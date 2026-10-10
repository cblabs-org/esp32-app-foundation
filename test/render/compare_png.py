#!/usr/bin/env python3
# ESP32 App Foundation (c) 2026 CB Labs
#
# https://github.com/matthewmcneill/esp32-app-foundation
#
# SPDX-License-Identifier: Apache-2.0
#
# Module: test/render/compare_png.py
# Description: Compare a harness render (a P4 PBM, 1 = lit) with a frame captured
#              from the device (a PNG, converted to 1 bit, white = lit), pixel for
#              pixel. Prints the differing-pixel count and, when any pixel
#              differs, an ASCII diff of the rows that hold a difference.
#
# Exported Functions:
# - read_pbm():     Read a binary PBM into a set of lit (x, y) pixels.
# - read_png():     Read a PNG, convert to 1 bit, into a set of lit (x, y) pixels.
# - main():         Compare the two files and print the result.
#
# Usage:
#   python3 test/render/compare_png.py <render.pbm> <device.png>
#
# Exit status: 0 identical, 1 differ, 2 usage or unreadable input.

import sys

from PIL import Image


def read_pbm(path):
    """Read a binary PBM (P4) into (width, height, set of lit pixels).

    The harness writes "P4\\n256 64\\n" and then 8 columns per byte, MSB first,
    1 = lit, so the header is exactly two lines.
    """
    with open(path, "rb") as handle:
        data = handle.read()
    magic, size, body = data.split(b"\n", 2)
    if magic != b"P4":
        raise ValueError(f"{path}: not a binary PBM")
    width, height = (int(token) for token in size.split())
    row_bytes = (width + 7) // 8
    lit = set()
    for y in range(height):
        for x in range(width):
            if body[y * row_bytes + x // 8] & (0x80 >> (x % 8)):
                lit.add((x, y))
    return width, height, lit


def read_png(path):
    """Read a PNG, convert it to 1 bit, and return (width, height, set of lit pixels).

    White (non-zero after conversion) is lit, as the device's capture writes it.
    """
    image = Image.open(path).convert("1")
    lit = {(x, y) for y in range(image.height) for x in range(image.width) if image.getpixel((x, y))}
    return image.width, image.height, lit


def main(argv):
    """Compare a render with a device capture; print the count and an ASCII diff."""
    if len(argv) != 3:
        print(__doc__ if __doc__ else "usage: compare_png.py <render.pbm> <device.png>", file=sys.stderr)
        return 2
    render_w, render_h, render = read_pbm(argv[1])
    device_w, device_h, device = read_png(argv[2])
    if (render_w, render_h) != (device_w, device_h):
        print(f"size differs: render {render_w}x{render_h}, device {device_w}x{device_h}")
        return 1

    differing = render ^ device
    print(f"render lit={len(render)} device lit={len(device)} "
          f"differing_pixels={len(differing)} "
          f"(render-only {len(render - device)}, device-only {len(device - render)})")
    if not differing:
        return 0

    # '#' lit in both, '+' lit in the render only, '-' lit on the device only.
    rows = sorted({y for _, y in differing})
    print("ASCII diff of rows with a difference ('#' both, '+' render only, '-' device only):")
    for y in range(max(0, rows[0] - 1), min(render_h, rows[-1] + 2)):
        line = ""
        for x in range(render_w):
            in_render, in_device = (x, y) in render, (x, y) in device
            line += "#" if in_render and in_device else "+" if in_render else "-" if in_device else "."
        print(f"{y:3d} {line}")
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
