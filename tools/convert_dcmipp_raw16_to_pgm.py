#!/usr/bin/env python3
"""Convert STM32 DCMIPP PIPE0 RAW16-unpacked RAW10 dumps to viewable files."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path

try:
    from PIL import Image, ImageOps
except ImportError:
    Image = None
    ImageOps = None


def write_pgm8(path: Path, pixels: list[int], width: int, height: int) -> None:
    with path.open("wb") as f:
        f.write(f"P5\n{width} {height}\n255\n".encode("ascii"))
        f.write(bytes((p >> 2) & 0xFF for p in pixels))


def write_pgm10(path: Path, pixels: list[int], width: int, height: int) -> None:
    with path.open("wb") as f:
        f.write(f"P5\n{width} {height}\n1023\n".encode("ascii"))
        for p in pixels:
            f.write(struct.pack(">H", p & 0x03FF))


def write_raw10le(path: Path, pixels: list[int]) -> None:
    with path.open("wb") as f:
        for p in pixels:
            f.write(struct.pack("<H", p & 0x03FF))


def write_png8_autocontrast(path: Path, pixels: list[int], width: int, height: int) -> bool:
    if Image is None or ImageOps is None:
        return False
    image = Image.frombytes("L", (width, height), bytes((p >> 2) & 0xFF for p in pixels))
    ImageOps.autocontrast(image).save(path)
    return True


def bayer_average_2x2(pixels: list[int], width: int, height: int) -> tuple[list[int], int, int]:
    out_width = width // 2
    out_height = height // 2
    averaged: list[int] = []
    for y in range(out_height):
        row0 = (y * 2) * width
        row1 = row0 + width
        for x in range(out_width):
            i = x * 2
            total = (
                pixels[row0 + i]
                + pixels[row0 + i + 1]
                + pixels[row1 + i]
                + pixels[row1 + i + 1]
            )
            averaged.append((total + 2) // 4)
    return averaged, out_width, out_height


def crop_left_half(pixels: list[int], width: int, height: int) -> tuple[list[int], int, int]:
    out_width = width // 2
    cropped: list[int] = []
    for y in range(height):
        start = y * width
        cropped.extend(pixels[start : start + out_width])
    return cropped, out_width, height


def write_pgm16(path: Path, pixels: list[int], width: int, height: int) -> None:
    with path.open("wb") as f:
        f.write(f"P5\n{width} {height}\n65535\n".encode("ascii"))
        for p in pixels:
            f.write(struct.pack(">H", p & 0xFFFF))


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


def unpack_packed10(data: bytes, width: int, height: int) -> list[int]:
    expected = (width * height * 10 + 7) // 8
    if len(data) < expected:
        raise SystemExit(f"packed input too short: got {len(data)} bytes, need {expected}")

    pixels: list[int] = []
    for i in range(0, expected, 5):
        b = data[i : i + 5]
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


def load_paged_packed10(data: bytes, width: int, height: int, page_size: int, payload_bytes: int) -> tuple[list[int], int]:
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
    return unpack_packed10(bytes(payload[:expected_payload]), width, height), pad_non_ff


def load_paged_raw16(data: bytes, width: int, height: int, page_size: int, payload_bytes: int) -> tuple[list[int], int]:
    line_bytes = width * 2
    if payload_bytes % line_bytes != 0:
        raise SystemExit(f"paged16 payload must contain whole lines: payload={payload_bytes}, line={line_bytes}")
    pixels: list[int] = []
    pad_non_ff = 0
    for start in range(0, len(data), page_size):
        page = data[start : start + page_size]
        if len(page) < page_size:
            break
        payload = page[:payload_bytes]
        for i in range(0, len(payload), 2):
            pixels.append(payload[i] | (payload[i + 1] << 8))
            if len(pixels) >= width * height:
                break
        pad_non_ff += sum(b != 0xFF for b in page[payload_bytes:])
        if len(pixels) >= width * height:
            break
    if len(pixels) < width * height:
        raise SystemExit(f"paged16 input too short: got {len(pixels)} pixels, need {width * height}")
    return pixels[: width * height], pad_non_ff


def load_raw16_rows(data: bytes, width: int, height: int, stride: int) -> tuple[list[int], int, int]:
    line_bytes = width * 2
    expected = (height - 1) * stride + line_bytes
    if len(data) < expected:
        raise SystemExit(f"input too short: got {len(data)} bytes, need {expected} for stride={stride}")

    raw16_pixels: list[int] = []
    high6_nonzero = 0
    low6_nonzero = 0
    for y in range(height):
        row = data[y * stride : y * stride + line_bytes]
        for i in range(0, line_bytes, 2):
            raw16 = row[i] | (row[i + 1] << 8)
            raw16_pixels.append(raw16)
            if raw16 & 0xFC00:
                high6_nonzero += 1
            if raw16 & 0x003F:
                low6_nonzero += 1

    return raw16_pixels, high6_nonzero, low6_nonzero


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Convert DCMIPP PIPE0 dump output. Input must be 16-bit little-endian "
            "unpacked RAW10, with valid data in bits[9:0]."
        )
    )
    parser.add_argument("input", type=Path, help="Input raw16le dump, 640*480*2 bytes by default")
    parser.add_argument("--width", type=int, default=640)
    parser.add_argument("--height", type=int, default=480)
    parser.add_argument("--stride-bytes", type=int, default=None, help="Bytes between consecutive lines")
    parser.add_argument("--format", choices=("raw16le", "paged10", "paged16"), default="raw16le")
    parser.add_argument("--page-size", type=int, default=8192, help="Bytes per page for --format paged10")
    parser.add_argument("--page-payload-bytes", type=int, default=5120, help="Packed RAW10 bytes per page for --format paged10")
    parser.add_argument("--prefix", type=Path, default=None, help="Output prefix without extension")
    parser.add_argument(
        "--mode",
        choices=("auto", "right", "left", "full16"),
        default="auto",
        help="How to convert 16-bit samples to RAW10. auto keeps <=0x03ff and rescales larger values.",
    )
    args = parser.parse_args()

    line_bytes = args.width * 2
    stride = args.stride_bytes if args.stride_bytes is not None else line_bytes
    if stride < line_bytes:
        raise SystemExit(f"stride too small: got {stride}, need at least {line_bytes}")

    expected = (args.height - 1) * stride + line_bytes
    data = args.input.read_bytes()
    if len(data) < expected:
        raise SystemExit(f"input too short: got {len(data)} bytes, need {expected}")

    if args.format == "paged10":
        pixels, pad_non_ff = load_paged_packed10(data, args.width, args.height, args.page_size, args.page_payload_bytes)
        raw16_pixels = pixels
        high6_nonzero = 0
        low6_nonzero = sum(1 for p in pixels if p & 0x003F)
        high6_by_line = [0 for _ in range(args.height)]
    elif args.format == "paged16":
        raw16_pixels, pad_non_ff = load_paged_raw16(data, args.width, args.height, args.page_size, args.page_payload_bytes)
        pixels = [raw16_to_raw10(raw16, args.mode) for raw16 in raw16_pixels]
        high6_by_line = []
        high6_nonzero = 0
        low6_nonzero = 0
        for y in range(args.height):
            line_high6 = 0
            for raw16 in raw16_pixels[y * args.width : (y + 1) * args.width]:
                if raw16 & 0xFC00:
                    line_high6 += 1
                if raw16 & 0x003F:
                    low6_nonzero += 1
            high6_by_line.append(line_high6)
            high6_nonzero += line_high6
    else:
        raw16_pixels, high6_nonzero, low6_nonzero = load_raw16_rows(data, args.width, args.height, stride)
        pixels = []
        high6_by_line = []
        for y in range(args.height):
            line_high6 = 0
            for raw16 in raw16_pixels[y * args.width : (y + 1) * args.width]:
                if raw16 & 0xFC00:
                    line_high6 += 1
            high6_by_line.append(line_high6)

        for raw16 in raw16_pixels:
            pixels.append(raw16_to_raw10(raw16, args.mode))
        pad_non_ff = 0

    prefix = args.prefix if args.prefix is not None else args.input.with_suffix("")
    pgm8 = prefix.with_name(prefix.name + "_8bit.pgm")
    pgm10 = prefix.with_name(prefix.name + "_10bit.pgm")
    pgm16 = prefix.with_name(prefix.name + "_raw16_16bit.pgm")
    raw10le = prefix.with_name(prefix.name + "_raw10le.bin")
    avg_pixels, avg_width, avg_height = bayer_average_2x2(pixels, args.width, args.height)
    avg_pgm8 = prefix.with_name(prefix.name + "_bayer2x2avg_8bit.pgm")
    avg_pgm10 = prefix.with_name(prefix.name + "_bayer2x2avg_10bit.pgm")
    png8 = prefix.with_name(prefix.name + "_raw10_autocontrast.png")
    avg_png8 = prefix.with_name(prefix.name + "_bayer2x2avg_autocontrast.png")
    left_pixels, left_width, left_height = crop_left_half(pixels, args.width, args.height)
    left_pgm8 = prefix.with_name(prefix.name + "_left_half_8bit.pgm")
    left_png8 = prefix.with_name(prefix.name + "_left_half_autocontrast.png")
    left_avg_pixels, left_avg_width, left_avg_height = bayer_average_2x2(left_pixels, left_width, left_height)
    left_avg_pgm8 = prefix.with_name(prefix.name + "_left_half_bayer2x2avg_8bit.pgm")
    left_avg_png8 = prefix.with_name(prefix.name + "_left_half_bayer2x2avg_autocontrast.png")
    stats = prefix.with_name(prefix.name + "_stats.txt")

    write_pgm8(pgm8, pixels, args.width, args.height)
    write_pgm10(pgm10, pixels, args.width, args.height)
    write_pgm16(pgm16, raw16_pixels, args.width, args.height)
    write_raw10le(raw10le, pixels)
    write_pgm8(avg_pgm8, avg_pixels, avg_width, avg_height)
    write_pgm10(avg_pgm10, avg_pixels, avg_width, avg_height)
    wrote_png8 = write_png8_autocontrast(png8, pixels, args.width, args.height)
    wrote_avg_png8 = write_png8_autocontrast(avg_png8, avg_pixels, avg_width, avg_height)
    write_pgm8(left_pgm8, left_pixels, left_width, left_height)
    write_pgm8(left_avg_pgm8, left_avg_pixels, left_avg_width, left_avg_height)
    wrote_left_png8 = write_png8_autocontrast(left_png8, left_pixels, left_width, left_height)
    wrote_left_avg_png8 = write_png8_autocontrast(left_avg_png8, left_avg_pixels, left_avg_width, left_avg_height)

    lines = [
        f"input={args.input}",
        f"width={args.width}",
        f"height={args.height}",
        f"line_bytes={line_bytes}",
        f"stride_bytes={stride}",
        f"format={args.format}",
        f"page_size={args.page_size}",
        f"page_payload_bytes={args.page_payload_bytes}",
        f"page_pad_non_ff={pad_non_ff}",
        f"mode={args.mode}",
        f"input_bytes={len(data)}",
        f"used_bytes={expected}",
        f"min_raw16={min(raw16_pixels)}",
        f"max_raw16={max(raw16_pixels)}",
        f"min_raw10={min(pixels)}",
        f"max_raw10={max(pixels)}",
        f"high6_nonzero={high6_nonzero}",
        f"low6_nonzero={low6_nonzero}",
        "high6_by_line_first16=" + " ".join(str(v) for v in high6_by_line[:16]),
        f"pgm8={pgm8}",
        f"pgm10={pgm10}",
        f"pgm16={pgm16}",
        f"raw10le={raw10le}",
        f"bayer2x2avg_width={avg_width}",
        f"bayer2x2avg_height={avg_height}",
        f"bayer2x2avg_pgm8={avg_pgm8}",
        f"bayer2x2avg_pgm10={avg_pgm10}",
        f"png8={png8 if wrote_png8 else 'not_written_pillow_missing'}",
        f"bayer2x2avg_png8={avg_png8 if wrote_avg_png8 else 'not_written_pillow_missing'}",
        f"left_half_width={left_width}",
        f"left_half_height={left_height}",
        f"left_half_pgm8={left_pgm8}",
        f"left_half_png8={left_png8 if wrote_left_png8 else 'not_written_pillow_missing'}",
        f"left_half_bayer2x2avg_width={left_avg_width}",
        f"left_half_bayer2x2avg_height={left_avg_height}",
        f"left_half_bayer2x2avg_pgm8={left_avg_pgm8}",
        f"left_half_bayer2x2avg_png8={left_avg_png8 if wrote_left_avg_png8 else 'not_written_pillow_missing'}",
    ]
    stats.write_text("\n".join(lines) + "\n", encoding="ascii")
    print("\n".join(lines))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
