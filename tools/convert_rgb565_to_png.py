#!/usr/bin/env python3
import argparse
from pathlib import Path

from PIL import Image, ImageOps


def expand_rgb565(pixel: int) -> tuple[int, int, int]:
    r5 = (pixel >> 11) & 0x1F
    g6 = (pixel >> 5) & 0x3F
    b5 = pixel & 0x1F
    r8 = (r5 << 3) | (r5 >> 2)
    g8 = (g6 << 2) | (g6 >> 4)
    b8 = (b5 << 3) | (b5 >> 2)
    return r8, g8, b8


def main() -> int:
    parser = argparse.ArgumentParser(description="Convert a DCMIPP UART frame dump to PNG.")
    parser.add_argument("input", type=Path, help="Raw input frame file")
    parser.add_argument("output", type=Path, help="Output PNG file")
    parser.add_argument("--width", type=int, default=640)
    parser.add_argument("--height", type=int, default=480)
    parser.add_argument("--stride", type=int, default=0, help="Input bytes per line; defaults to width * bytes-per-pixel")
    parser.add_argument("--format", choices=("rgb565", "gray8", "raw16le", "raw10p"), default="rgb565")
    args = parser.parse_args()

    if args.format == "gray8":
        min_stride = args.width
    elif args.format == "raw10p":
        min_stride = (args.width * 10 + 7) // 8
    else:
        min_stride = args.width * 2
    stride = args.stride if args.stride else min_stride
    if stride < min_stride:
        raise SystemExit(f"stride too small: got {stride}, expected at least {min_stride}")

    expected = stride * args.height
    data = args.input.read_bytes()
    if len(data) < expected:
        raise SystemExit(f"input too small: got {len(data)} bytes, expected at least {expected}")
    if len(data) > expected:
        data = data[:expected]

    if args.format == "gray8":
        pixels = bytearray(args.width * args.height)
        for y in range(args.height):
            start = y * stride
            row = data[start : start + args.width]
            pixels[y * args.width : (y + 1) * args.width] = row
        image = Image.frombytes("L", (args.width, args.height), bytes(pixels))
    elif args.format == "raw10p":
        samples10 = []
        for y in range(args.height):
            row = data[y * stride : y * stride + min_stride]
            x = 0
            for i in range(0, len(row) - 4, 5):
                b0, b1, b2, b3, b4 = row[i : i + 5]
                samples10.extend([
                    b0 | ((b4 & 0x03) << 8),
                    b1 | (((b4 >> 2) & 0x03) << 8),
                    b2 | (((b4 >> 4) & 0x03) << 8),
                    b3 | (((b4 >> 6) & 0x03) << 8),
                ])
                x += 4
                if x >= args.width:
                    break
        samples10 = samples10[: args.width * args.height]
        pixels = bytearray((v >> 2) & 0xFF for v in samples10)
        image = ImageOps.autocontrast(Image.frombytes("L", (args.width, args.height), bytes(pixels)))
        print(f"raw10p min=0x{min(samples10) if samples10 else 0:03X} max=0x{max(samples10) if samples10 else 0:03X}")
    elif args.format == "raw16le":
        raw = []
        high6_nonzero = 0
        low6_nonzero = 0
        for y in range(args.height):
            for x in range(args.width):
                i = (y * stride) + (x * 2)
                v = data[i] | (data[i + 1] << 8)
                raw.append(v)
                high6_nonzero += 1 if (v & 0xFC00) else 0
                low6_nonzero += 1 if (v & 0x003F) else 0

        if raw and max(raw) <= 0x03FF:
            samples10 = raw
            mode = "low10"
        elif high6_nonzero and low6_nonzero == 0:
            samples10 = [(v >> 6) & 0x03FF for v in raw]
            mode = "high10"
        else:
            maxv = max(raw) if raw else 0
            samples10 = [((v * 1023 + (maxv // 2)) // maxv) if maxv else 0 for v in raw]
            mode = "scaled16"

        pixels = bytearray((v >> 2) & 0xFF for v in samples10)
        image = Image.frombytes("L", (args.width, args.height), bytes(pixels))
        image = ImageOps.autocontrast(image)
        print(
            f"raw16le mode={mode} min=0x{min(raw) if raw else 0:04X} "
            f"max=0x{max(raw) if raw else 0:04X} high6_nonzero={high6_nonzero} low6_nonzero={low6_nonzero}"
        )
    else:
        pixels = bytearray(args.width * args.height * 3)
        for y in range(args.height):
            for x in range(args.width):
                i = (y * stride) + (x * 2)
                lo = data[i]
                hi = data[i + 1]
                r, g, b = expand_rgb565(lo | (hi << 8))
                j = ((y * args.width) + x) * 3
                pixels[j] = r
                pixels[j + 1] = g
                pixels[j + 2] = b
        image = Image.frombytes("RGB", (args.width, args.height), bytes(pixels))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    image.save(args.output)
    print(f"wrote {args.output} ({args.width}x{args.height})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

