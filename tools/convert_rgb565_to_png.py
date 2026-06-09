#!/usr/bin/env python3
import argparse
from pathlib import Path

from PIL import Image


def expand_rgb565(pixel: int) -> tuple[int, int, int]:
    r5 = (pixel >> 11) & 0x1F
    g6 = (pixel >> 5) & 0x3F
    b5 = pixel & 0x1F
    r8 = (r5 << 3) | (r5 >> 2)
    g8 = (g6 << 2) | (g6 >> 4)
    b8 = (b5 << 3) | (b5 >> 2)
    return r8, g8, b8


def main() -> int:
    parser = argparse.ArgumentParser(description="Convert a little-endian RGB565 frame dump to PNG.")
    parser.add_argument("input", type=Path, help="Raw RGB565LE input file")
    parser.add_argument("output", type=Path, help="Output PNG file")
    parser.add_argument("--width", type=int, default=640)
    parser.add_argument("--height", type=int, default=480)
    args = parser.parse_args()

    expected = args.width * args.height * 2
    data = args.input.read_bytes()
    if len(data) < expected:
        raise SystemExit(f"input too small: got {len(data)} bytes, expected at least {expected}")
    if len(data) > expected:
        data = data[:expected]

    pixels = bytearray(args.width * args.height * 3)
    for i in range(args.width * args.height):
        lo = data[i * 2]
        hi = data[(i * 2) + 1]
        r, g, b = expand_rgb565(lo | (hi << 8))
        j = i * 3
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
