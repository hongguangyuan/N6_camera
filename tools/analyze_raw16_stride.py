#!/usr/bin/env python3
"""Check candidate line strides for DCMIPP RAW16 dumps."""

from __future__ import annotations

import argparse
from pathlib import Path


def load_pixels(data: bytes, width: int, height: int, stride: int) -> list[int] | None:
    line_bytes = width * 2
    needed = (height - 1) * stride + line_bytes
    if stride < line_bytes or len(data) < needed:
        return None

    pixels: list[int] = []
    for y in range(height):
        row = data[y * stride : y * stride + line_bytes]
        for i in range(0, line_bytes, 2):
            pixels.append(row[i] | (row[i + 1] << 8))
    return pixels


def mean_abs_line_delta(pixels: list[int], width: int, height: int) -> int:
    total = 0
    count = 0
    for y in range(1, height):
        prev = pixels[(y - 1) * width : y * width]
        cur = pixels[y * width : (y + 1) * width]
        for a, b in zip(prev, cur):
            total += abs((a & 0x03FF) - (b & 0x03FF))
            count += 1
    return total // count if count else 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Analyze RAW16 line stride candidates.")
    parser.add_argument("input", type=Path)
    parser.add_argument("--width", type=int, default=640)
    parser.add_argument("--height", type=int, default=480)
    parser.add_argument(
        "--strides",
        default="1280,1296,1312,1344,1408,1536",
        help="Comma-separated stride candidates in bytes",
    )
    args = parser.parse_args()

    data = args.input.read_bytes()
    print(f"input={args.input}")
    print(f"input_bytes={len(data)}")
    print(f"line_bytes={args.width * 2}")
    for stride in [int(v.strip(), 0) for v in args.strides.split(",") if v.strip()]:
        pixels = load_pixels(data, args.width, args.height, stride)
        needed = (args.height - 1) * stride + args.width * 2
        if pixels is None:
            print(f"stride={stride} needed={needed} status=too_short")
            continue
        high6 = sum(1 for p in pixels if p & 0xFC00)
        low6 = sum(1 for p in pixels if p & 0x003F)
        raw10 = [p & 0x03FF for p in pixels]
        delta = mean_abs_line_delta(pixels, args.width, args.height)
        print(
            f"stride={stride} needed={needed} status=ok "
            f"min_raw16=0x{min(pixels):04X} max_raw16=0x{max(pixels):04X} "
            f"min_raw10=0x{min(raw10):03X} max_raw10=0x{max(raw10):03X} "
            f"high6_nonzero={high6} low6_nonzero={low6} mean_abs_line_delta={delta}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
