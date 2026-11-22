#!/usr/bin/env python3
"""Convert a PNG into GBA 8x8 tiles, a tilemap, and a BGR555 palette.

Requires Pillow: python -m pip install Pillow

Example:
  python tools/png2gba.py input.png --bpp 4 --name level_tiles --out build/level_tiles

The PNG dimensions must be multiples of 8. Alpha < 128 maps to palette index 0.
For 4bpp, the image may use at most 15 opaque colors plus transparency.
For 8bpp, the image may use at most 255 opaque colors plus transparency.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path
from PIL import Image


def c_identifier(value: str) -> str:
    value = re.sub(r"\W+", "_", value)
    if not value or value[0].isdigit():
        value = "asset_" + value
    return value


def bgr555(rgba: tuple[int, int, int, int]) -> int:
    r, g, b, _a = rgba
    r5 = (r * 31 + 127) // 255
    g5 = (g * 31 + 127) // 255
    b5 = (b * 31 + 127) // 255
    return r5 | (g5 << 5) | (b5 << 10)


def format_array(values: list[int], width: int = 12) -> str:
    lines = []
    for start in range(0, len(values), width):
        lines.append("    " + ", ".join(
            f"0x{v:02X}" if v <= 0xFF else f"0x{v:04X}"
            for v in values[start:start + width]
        ))
    return ",\n".join(lines)


def convert(input_path: Path, output_prefix: Path, name: str, bpp: int,
            dedupe: bool) -> tuple[int, int, int]:
    image = Image.open(input_path).convert("RGBA")
    width, height = image.size

    if width % 8 or height % 8:
        raise ValueError(f"Image size {width}x{height} must be a multiple of 8.")

    max_colors = 16 if bpp == 4 else 256
    pixels = list(image.getdata())

    # Index 0 is always transparent.
    palette: list[tuple[int, int, int, int]] = [(0, 0, 0, 0)]
    palette_lookup: dict[tuple[int, int, int, int], int] = {}

    for pixel in pixels:
        if pixel[3] < 128:
            continue
        if pixel not in palette_lookup:
            if len(palette) >= max_colors:
                raise ValueError(
                    f"Image has more than {max_colors - 1} opaque colors "
                    f"for {bpp}bpp. Reduce the palette first."
                )
            palette_lookup[pixel] = len(palette)
            palette.append(pixel)

    def index_of(pixel: tuple[int, int, int, int]) -> int:
        if pixel[3] < 128:
            return 0
        return palette_lookup[pixel]

    tile_data = bytearray()
    tile_map: list[int] = []
    tile_lookup: dict[bytes, int] = {}

    tiles_x = width // 8
    tiles_y = height // 8

    for ty in range(tiles_y):
        for tx in range(tiles_x):
            indices = []
            for py in range(8):
                for px in range(8):
                    indices.append(index_of(image.getpixel((tx * 8 + px,
                                                            ty * 8 + py))))

            if bpp == 4:
                packed = bytes(
                    indices[i] | (indices[i + 1] << 4)
                    for i in range(0, 64, 2)
                )
            else:
                packed = bytes(indices)

            tile_id = tile_lookup.get(packed) if dedupe else None
            if tile_id is None:
                tile_id = len(tile_data) // (32 if bpp == 4 else 64)
                tile_lookup[packed] = tile_id
                tile_data.extend(packed)

            if tile_id > 1023:
                raise ValueError("Tile count exceeds the 10-bit GBA tile index.")
            tile_map.append(tile_id)

    palette_words = [bgr555(color) for color in palette]
    ident = c_identifier(name)
    header_path = output_prefix.with_suffix(".h")
    source_path = output_prefix.with_suffix(".c")
    header_path.parent.mkdir(parents=True, exist_ok=True)

    header = f"""#ifndef {ident.upper()}_H
#define {ident.upper()}_H

#include <stdint.h>

extern const uint8_t {ident}_tiles[];
extern const uint16_t {ident}_palette[];
extern const uint16_t {ident}_map[];

#define {ident.upper()}_TILE_COUNT {len(tile_data) // (32 if bpp == 4 else 64)}
#define {ident.upper()}_MAP_WIDTH {tiles_x}
#define {ident.upper()}_MAP_HEIGHT {tiles_y}
#define {ident.upper()}_PALETTE_COUNT {len(palette_words)}
#define {ident.upper()}_BPP {bpp}

#endif
"""

    source = f"""#include <stdint.h>
#include "{header_path.name}"

const uint8_t {ident}_tiles[] = {{
{format_array(list(tile_data))}
}};

const uint16_t {ident}_palette[] = {{
{format_array(palette_words)}
}};

const uint16_t {ident}_map[] = {{
{format_array(tile_map)}
}};
"""

    header_path.write_text(header, encoding="utf-8")
    source_path.write_text(source, encoding="utf-8")

    return (len(tile_data) // (32 if bpp == 4 else 64),
            len(palette_words), len(tile_map))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("--bpp", type=int, choices=(4, 8), default=4)
    parser.add_argument("--name", default=None)
    parser.add_argument("--out", type=Path, required=True,
                        help="Output path prefix, without extension.")
    parser.add_argument("--no-dedupe", action="store_true",
                        help="Keep duplicate tiles instead of deduplicating.")
    args = parser.parse_args()

    name = args.name or args.input.stem
    tile_count, palette_count, map_entries = convert(
        args.input, args.out, name, args.bpp, not args.no_dedupe
    )
    print(f"Generated {args.out}.c and {args.out}.h")
    print(f"Tiles: {tile_count}; palette entries: {palette_count}; "
          f"tilemap entries: {map_entries}")


if __name__ == "__main__":
    main()
