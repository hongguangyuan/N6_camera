#!/usr/bin/env python3
"""Convert IMX219 Bayer RAW10 dumps to RGB images."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path

try:
    from PIL import Image
except Exception:  # pragma: no cover - optional dependency fallback
    Image = None


def raw16_to_raw10(raw16: int, mode: str) -> int:
    if mode == "right":
        return raw16 & 0x03FF
    if mode == "left":
        return (raw16 >> 6) & 0x03FF
    if mode == "full16":
        return (raw16 * 1023 + 32767) // 65535
    if raw16 <= 0x03FF:
        return raw16
    return (raw16 * 1023 + 32767) // 65535


def unpack_packed10(data: bytes, width: int, height: int, stride: int | None) -> list[int]:
    packed_line_bytes = (width * 10 + 7) // 8
    packed_stride = stride if stride is not None else packed_line_bytes
    expected_packed = (height - 1) * packed_stride + packed_line_bytes
    if packed_stride < packed_line_bytes:
        raise SystemExit(f"stride too small: got {packed_stride}, need at least {packed_line_bytes}")
    if len(data) < expected_packed:
        raise SystemExit(f"packed input too short: got {len(data)} bytes, need {expected_packed} for stride={packed_stride}")

    pixels = []
    for y in range(height):
        line = data[y * packed_stride : y * packed_stride + packed_line_bytes]
        for x in range(0, width, 4):
            b = line[(x // 4) * 5 : (x // 4) * 5 + 5]
            if len(b) < 5:
                break
            pixels.extend(
                [
                    ((b[0] << 2) | (b[4] & 0x03)) & 0x03FF,
                    ((b[1] << 2) | ((b[4] >> 2) & 0x03)) & 0x03FF,
                    ((b[2] << 2) | ((b[4] >> 4) & 0x03)) & 0x03FF,
                    ((b[3] << 2) | ((b[4] >> 6) & 0x03)) & 0x03FF,
                ]
            )
    return pixels[: width * height]


def unpack_paged_packed10(data: bytes, width: int, height: int, page_size: int, payload_bytes: int) -> list[int]:
    expected_payload = (width * height * 10 + 7) // 8
    payload = bytearray()
    pad_non_ff = 0
    for start in range(0, len(data), page_size):
        page = data[start : start + page_size]
        if len(page) < page_size:
            break
        payload.extend(page[:payload_bytes])
        pad_non_ff += sum(b != 0xFF for b in page[payload_bytes:])
        if len(payload) >= expected_payload:
            break
    if len(payload) < expected_payload:
        raise SystemExit(f"paged input too short: got {len(payload)} payload bytes, need {expected_payload}")
    return unpack_packed10(bytes(payload[:expected_payload]), width, height, None)


def unpack_paged_raw16(data: bytes, width: int, height: int, page_size: int, payload_bytes: int, mode: str) -> list[int]:
    line_bytes = width * 2
    if payload_bytes % line_bytes != 0:
        raise SystemExit(f"paged16 payload must contain whole lines: payload={payload_bytes}, line={line_bytes}")
    pixels: list[int] = []
    for start in range(0, len(data), page_size):
        page = data[start : start + page_size]
        if len(page) < page_size:
            break
        payload = page[:payload_bytes]
        for i in range(0, len(payload), 2):
            raw16 = payload[i] | (payload[i + 1] << 8)
            pixels.append(raw16_to_raw10(raw16, mode))
            if len(pixels) >= width * height:
                break
        if len(pixels) >= width * height:
            break
    if len(pixels) < width * height:
        raise SystemExit(f"paged16 input too short: got {len(pixels)} pixels, need {width * height}")
    return pixels[: width * height]


def load_pixels(
    path: Path,
    width: int,
    height: int,
    input_format: str,
    mode: str,
    stride: int | None,
    page_size: int,
    page_payload_bytes: int,
) -> list[int]:
    data = path.read_bytes()
    raw16_line_bytes = width * 2
    raw16_stride = stride if stride is not None else raw16_line_bytes
    expected_16 = (height - 1) * raw16_stride + raw16_line_bytes
    if input_format == "auto":
        input_format = "raw16le" if len(data) >= expected_16 else "packed10"

    if input_format == "paged10":
        return unpack_paged_packed10(data, width, height, page_size, page_payload_bytes)
    if input_format == "paged16":
        return unpack_paged_raw16(data, width, height, page_size, page_payload_bytes, mode)

    if input_format in ("raw16le", "raw10le"):
        if raw16_stride < raw16_line_bytes:
            raise SystemExit(f"stride too small: got {raw16_stride}, need at least {raw16_line_bytes}")
        if len(data) < expected_16:
            raise SystemExit(f"input too short: got {len(data)} bytes, need {expected_16} for stride={raw16_stride}")
        pixels = []
        for y in range(height):
            row = data[y * raw16_stride : y * raw16_stride + raw16_line_bytes]
            for i in range(0, raw16_line_bytes, 2):
                raw16 = row[i] | (row[i + 1] << 8)
                pixels.append(raw16_to_raw10(raw16, mode) if input_format == "raw16le" else (raw16 & 0x03FF))
        return pixels

    return unpack_packed10(data, width, height, stride)


def color_at(pattern: str, x: int, y: int) -> str:
    pattern = pattern.upper()
    if pattern == "RGGB":
        return ("R", "G", "G", "B")[(y & 1) * 2 + (x & 1)]
    if pattern == "BGGR":
        return ("B", "G", "G", "R")[(y & 1) * 2 + (x & 1)]
    if pattern == "GRBG":
        return ("G", "R", "B", "G")[(y & 1) * 2 + (x & 1)]
    if pattern == "GBRG":
        return ("G", "B", "R", "G")[(y & 1) * 2 + (x & 1)]
    raise ValueError(pattern)


def sample(pixels: list[int], width: int, height: int, x: int, y: int) -> int:
    x = 0 if x < 0 else (width - 1 if x >= width else x)
    y = 0 if y < 0 else (height - 1 if y >= height else y)
    return pixels[y * width + x]


def avg_values(pixels: list[int], width: int, height: int, coords: list[tuple[int, int]]) -> int:
    return sum(sample(pixels, width, height, x, y) for x, y in coords) // len(coords)


def demosaic_bilinear(pixels: list[int], width: int, height: int, pattern: str) -> list[tuple[int, int, int]]:
    rgb: list[tuple[int, int, int]] = []
    for y in range(height):
        for x in range(width):
            c = color_at(pattern, x, y)
            v = sample(pixels, width, height, x, y)
            if c == "R":
                r = v
                g = avg_values(pixels, width, height, [(x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)])
                b = avg_values(pixels, width, height, [(x - 1, y - 1), (x + 1, y - 1), (x - 1, y + 1), (x + 1, y + 1)])
            elif c == "B":
                b = v
                g = avg_values(pixels, width, height, [(x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)])
                r = avg_values(pixels, width, height, [(x - 1, y - 1), (x + 1, y - 1), (x - 1, y + 1), (x + 1, y + 1)])
            else:
                g = v
                left_right_are_red = color_at(pattern, x - 1, y) == "R" or color_at(pattern, x + 1, y) == "R"
                if left_right_are_red:
                    r = avg_values(pixels, width, height, [(x - 1, y), (x + 1, y)])
                    b = avg_values(pixels, width, height, [(x, y - 1), (x, y + 1)])
                else:
                    r = avg_values(pixels, width, height, [(x, y - 1), (x, y + 1)])
                    b = avg_values(pixels, width, height, [(x - 1, y), (x + 1, y)])
            rgb.append((r, g, b))
    return rgb


def percentile(values: list[int], num: int, den: int) -> int:
    if not values:
        return 0
    return sorted(values)[(len(values) - 1) * num // den]


def scale_rgb(
    rgb10: list[tuple[int, int, int]],
    black: int | None,
    white: int | None,
    wb: tuple[float, float, float],
    gamma: float,
    brightness: float,
) -> bytes:
    if gamma <= 0.0:
        raise SystemExit(f"gamma must be > 0, got {gamma}")
    if brightness <= 0.0:
        raise SystemExit(f"brightness must be > 0, got {brightness}")
    if any(gain <= 0.0 for gain in wb):
        raise SystemExit(f"white-balance gains must be > 0, got {wb}")

    all_values = [v for px in rgb10 for v in px]
    black_level = percentile(all_values, 1, 100) if black is None else black

    corrected: list[tuple[float, float, float]] = []
    for r, g, b in rgb10:
        corrected.append(
            (
                max(0.0, float(r - black_level)) * wb[0],
                max(0.0, float(g - black_level)) * wb[1],
                max(0.0, float(b - black_level)) * wb[2],
            )
        )

    if white is None:
        hi = float(percentile([int(v) for px in corrected for v in px], 99, 100))
    else:
        hi = max(1.0, float(white - black_level) * max(wb))
    if hi <= 0.0:
        hi = 1.0

    out = bytearray()
    for px in corrected:
        for v in px:
            n = (v / hi) * brightness
            if n < 0.0:
                n = 0.0
            elif n > 1.0:
                n = 1.0
            s = int((n ** gamma) * 255.0 + 0.5)
            out.append(0 if s < 0 else (255 if s > 255 else s))
    return bytes(out)


def write_ppm(path: Path, rgb8: bytes, width: int, height: int) -> None:
    with path.open("wb") as f:
        f.write(f"P6\n{width} {height}\n255\n".encode("ascii"))
        f.write(rgb8)


def main() -> int:
    parser = argparse.ArgumentParser(description="Demosaic Bayer RAW10 to RGB PNG/PPM.")
    parser.add_argument("input", type=Path)
    parser.add_argument("--width", type=int, default=640)
    parser.add_argument("--height", type=int, default=480)
    parser.add_argument("--format", choices=("auto", "raw16le", "raw10le", "packed10", "paged10", "paged16"), default="auto")
    parser.add_argument("--raw16-mode", choices=("auto", "right", "left", "full16"), default="auto")
    parser.add_argument("--stride-bytes", type=int, default=None, help="Bytes between consecutive lines")
    parser.add_argument("--page-size", type=int, default=8192, help="Bytes per page for --format paged10")
    parser.add_argument("--page-payload-bytes", type=int, default=5120, help="Packed RAW10 bytes per page for --format paged10")
    parser.add_argument("--bayer", choices=("RGGB", "BGGR", "GRBG", "GBRG"), default="RGGB")
    parser.add_argument("--black", type=int, default=None, help="RAW10 black level to subtract before color processing")
    parser.add_argument("--white", type=int, default=None, help="RAW10 white level before black subtraction; default uses 99th percentile")
    parser.add_argument("--wb", type=float, nargs=3, metavar=("R", "G", "B"), default=None, help="White-balance gains for R G B")
    parser.add_argument("--wb-r", type=float, default=None, help="White-balance gain for red")
    parser.add_argument("--wb-g", type=float, default=None, help="White-balance gain for green")
    parser.add_argument("--wb-b", type=float, default=None, help="White-balance gain for blue")
    parser.add_argument("--gamma", type=float, default=1.0, help="Gamma exponent applied after scaling; 0.45 brightens shadows")
    parser.add_argument("--brightness", type=float, default=1.0, help="Linear brightness multiplier before gamma")
    parser.add_argument("--prefix", type=Path, default=None)
    args = parser.parse_args()

    wb_values = args.wb if args.wb is not None else (1.0, 1.0, 1.0)
    wb = (
        args.wb_r if args.wb_r is not None else wb_values[0],
        args.wb_g if args.wb_g is not None else wb_values[1],
        args.wb_b if args.wb_b is not None else wb_values[2],
    )

    pixels = load_pixels(
        args.input,
        args.width,
        args.height,
        args.format,
        args.raw16_mode,
        args.stride_bytes,
        args.page_size,
        args.page_payload_bytes,
    )
    rgb10 = demosaic_bilinear(pixels, args.width, args.height, args.bayer)
    rgb8 = scale_rgb(rgb10, args.black, args.white, wb, args.gamma, args.brightness)

    prefix = args.prefix if args.prefix is not None else args.input.with_suffix("")
    ppm = prefix.with_name(prefix.name + f"_{args.bayer}_rgb.ppm")
    png = prefix.with_name(prefix.name + f"_{args.bayer}_rgb.png")
    stats = prefix.with_name(prefix.name + f"_{args.bayer}_rgb_stats.txt")

    write_ppm(ppm, rgb8, args.width, args.height)
    if Image is not None:
        Image.frombytes("RGB", (args.width, args.height), rgb8).save(png)

    channels = list(zip(*rgb10))
    rgb8_channels = [rgb8[i::3] for i in range(3)]
    lines = [
        f"input={args.input}",
        f"width={args.width}",
        f"height={args.height}",
        f"format={args.format}",
        f"raw16_mode={args.raw16_mode}",
        f"stride_bytes={args.stride_bytes if args.stride_bytes is not None else 'default'}",
        f"page_size={args.page_size}",
        f"page_payload_bytes={args.page_payload_bytes}",
        f"bayer={args.bayer}",
        f"black={args.black if args.black is not None else 'auto_p1'}",
        f"white={args.white if args.white is not None else 'auto_p99'}",
        f"wb_r={wb[0]}",
        f"wb_g={wb[1]}",
        f"wb_b={wb[2]}",
        f"gamma={args.gamma}",
        f"brightness={args.brightness}",
        f"min_raw10={min(pixels)}",
        f"max_raw10={max(pixels)}",
        f"mean_r={sum(channels[0]) // len(channels[0])}",
        f"mean_g={sum(channels[1]) // len(channels[1])}",
        f"mean_b={sum(channels[2]) // len(channels[2])}",
        f"mean_rgb8_r={sum(rgb8_channels[0]) // len(rgb8_channels[0])}",
        f"mean_rgb8_g={sum(rgb8_channels[1]) // len(rgb8_channels[1])}",
        f"mean_rgb8_b={sum(rgb8_channels[2]) // len(rgb8_channels[2])}",
        f"ppm={ppm}",
    ]
    if Image is not None:
        lines.append(f"png={png}")
    stats.write_text("\n".join(lines) + "\n", encoding="ascii")
    print("\n".join(lines))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
