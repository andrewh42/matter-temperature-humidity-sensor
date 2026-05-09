#!/usr/bin/env python3
# /// script
# requires-python = ">=3.11"
# dependencies = ["pillow"]
# ///
"""Convert a captured LVGL/SSD16xx mono framebuffer dump to PNG.

The buffer is captured after `lv_timer_handler()` has flushed; at that point
Zephyr's mono flush callback has overwritten the LVGL I1 pixel data in place
with the converted output it sent to the SSD1681. With the panel in default
orientation the output is in MONO10 VTILED format:

  - 8-byte LVGL I1 palette header at the start of the buffer (skipped).
  - 200 columns × 25 vertical tiles, one byte per tile per column.
  - byte_idx = x + (y // 8) * width
  - bit 7 (MSB) is the topmost row of the tile (SCREEN_INFO_MONO_MSB_FIRST).
  - MONO10: bit=1 → white pixel on display, bit=0 → black pixel on display.
"""

import sys
from pathlib import Path
from PIL import Image

WIDTH = 200
HEIGHT = 200
TILE_HEIGHT = 8
PALETTE_HEADER_SIZE = 8


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <framebuf.bin> [output.png]", file=sys.stderr)
        sys.exit(1)

    input_path = Path(sys.argv[1])
    output_path = Path(sys.argv[2]) if len(sys.argv) > 2 else input_path.with_suffix(".png")

    raw = input_path.read_bytes()
    pixel_bytes = WIDTH * HEIGHT // TILE_HEIGHT
    data = raw[PALETTE_HEADER_SIZE : PALETTE_HEADER_SIZE + pixel_bytes]

    img = Image.new("L", (WIDTH, HEIGHT))
    pixels = img.load()

    for y in range(HEIGHT):
        tile = y // TILE_HEIGHT
        bit_pos = 7 - (y % TILE_HEIGHT)
        row_base = tile * WIDTH
        for x in range(WIDTH):
            bit = (data[row_base + x] >> bit_pos) & 1
            pixels[x, y] = 255 if bit else 0

    img.save(output_path)
    print(f"Saved {output_path}")


if __name__ == "__main__":
    main()
