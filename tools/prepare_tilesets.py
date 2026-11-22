#!/usr/bin/env python3
"""Convert extracted 16x16 GameMaker tilesets into deduplicated GBA 8bpp tiles.

The source tiles use 2px gutters. Each 16x16 source tile is downscaled to 8x8,
then deduplicated. The 8bpp output fits in one 16 KiB GBA BG character block
when the unique tile count is <=256.
"""

from __future__ import annotations
import argparse
import collections
import json
import re
from pathlib import Path
from PIL import Image

def ident(s):
    s = re.sub(r"\W+", "_", s)
    if not s or s[0].isdigit():
        s = "tileset_" + s
    return s

def bgr555(rgba):
    r, g, b, _a = rgba
    r5 = (r * 31 + 127) // 255
    g5 = (g * 31 + 127) // 255
    b5 = (b * 31 + 127) // 255
    return r5 | (g5 << 5) | (b5 << 10)

def c_array(values, width=16, fmt=None):
    lines = []
    for i in range(0, len(values), width):
        chunk = values[i:i+width]
        if fmt:
            line = ", ".join(fmt(v) for v in chunk)
        else:
            line = ", ".join(f"0x{v:02X}" for v in chunk)
        lines.append("    " + line)
    return ",\n".join(lines)

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("manifest", type=Path)
    ap.add_argument("--assets-root", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--scale", type=int, default=2)
    args = ap.parse_args()

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    out = args.out
    out.mkdir(parents=True, exist_ok=True)

    header = """#ifndef VOLVY_TILESETS_8BPP_H
#define VOLVY_TILESETS_8BPP_H

#include <stdint.h>

typedef struct GbaTileset8bpp {
    const uint8_t *tiles;
    const uint16_t *palette;
    const uint16_t *map;
    uint16_t tile_count;
    uint16_t palette_count;
    uint8_t map_width;
    uint8_t map_height;
} GbaTileset8bpp;

"""
    source = ['#include "tilesets_8bpp.h"', ""]

    result = []
    total_tiles = 0
    total_bytes = 0

    for ts in manifest["tilesets"]:
        name = ts["name"]
        cname = ident(name)
        image = Image.open(args.assets_root / ts["file"]).convert("RGBA")
        tile_w, tile_h = ts["tile_size"]
        border_x, border_y = ts["border"]
        cols = ts["columns"]
        rows = (ts["tile_count"] + cols - 1) // cols
        step_x = tile_w + border_x * 2
        step_y = tile_h + border_y * 2

        tile_images = []
        color_counts = collections.Counter()
        for tile_index in range(ts["tile_count"]):
                ty, tx = divmod(tile_index, cols)
                if tile_index == 0:
                    tile_images.append([(0, 0, 0, 0)] * 64)
                    continue
                crop = image.crop((
                    border_x + tx * step_x,
                    border_y + ty * step_y,
                    border_x + tx * step_x + tile_w,
                    border_y + ty * step_y + tile_h
                ))
                sw = max(1, (tile_w + args.scale - 1) // args.scale)
                sh = max(1, (tile_h + args.scale - 1) // args.scale)
                crop = crop.resize((sw, sh), Image.Resampling.NEAREST)
                if (sw, sh) != (8, 8):
                    raise ValueError(f"{name}: scale {args.scale} did not yield 8x8 tiles")
                pixels = list(crop.getdata())
                tile_images.append(pixels)
                color_counts.update(px for px in pixels if px[3] >= 128)

        palette_colors = [c for c, _ in color_counts.most_common()]
        if len(palette_colors) > 255:
            raise ValueError(f"{name}: {len(palette_colors)} opaque colors exceed 8bpp")
        palette_words = [0] + [bgr555(c) for c in palette_colors]
        palette_words += [0] * (256 - len(palette_words))
        color_index = {c: i + 1 for i, c in enumerate(palette_colors)}

        tile_data = bytearray()
        tile_lookup = {}
        tile_map = [0] * (cols * rows)
        for source_index, pixels in enumerate(tile_images):
            raw = bytes(0 if p[3] < 128 else color_index[p] for p in pixels)
            tile_id = tile_lookup.get(raw)
            if tile_id is None:
                tile_id = len(tile_lookup)
                tile_lookup[raw] = tile_id
                tile_data.extend(raw)
            tile_map[source_index] = tile_id

        unique_count = len(tile_lookup)
        if unique_count > 256:
            raise ValueError(f"{name}: {unique_count} unique tiles exceed one 8bpp char block")

        tile_name = f"{cname}_tiles"
        palette_name = f"{cname}_palette"
        map_name = f"{cname}_map"

        source.extend([
            f"static const uint8_t {tile_name}[] __attribute__((aligned(4))) = {{",
            c_array(list(tile_data)),
            "};",
            "",
            f"static const uint16_t {palette_name}[256] = {{",
            c_array(palette_words, 8, lambda v: f"0x{v:04X}"),
            "};",
            "",
            f"static const uint16_t {map_name}[{len(tile_map)}] = {{",
            c_array(tile_map, 12, lambda v: str(v)),
            "};",
            "",
            f"const GbaTileset8bpp g_{cname} = {{",
            f"    {tile_name}, {palette_name}, {map_name},",
            f"    {unique_count}, {len(palette_colors)+1}, {cols}, {rows}",
            "};",
            "",
        ])
        header += f"extern const GbaTileset8bpp g_{cname};\n"
        total_tiles += unique_count
        total_bytes += len(tile_data)
        result.append({
            "name": name, "map_size": [cols, rows],
            "source_tile_size": [tile_w, tile_h],
            "gba_tile_size": [8, 8],
            "source_tile_count": ts["tile_count"], "map_entries": len(tile_map),
            "unique_gba_tiles": unique_count,
            "opaque_colors": len(palette_colors),
            "gba_tile_bytes": len(tile_data),
        })

    header += "\n#endif\n"
    (out / "tilesets_8bpp.h").write_text(header, encoding="utf-8")
    (out / "tilesets_8bpp.c").write_text("\n".join(source), encoding="utf-8")
    (out / "tilesets_8bpp_manifest.json").write_text(
        json.dumps({
            "scale_divisor": args.scale,
            "tilesets": result,
            "total_unique_tiles_across_worlds": total_tiles,
            "total_tile_bytes_if_all_linked": total_bytes,
        }, indent=2), encoding="utf-8"
    )

    print(f"Tilesets converted: {len(result)}")
    print(f"Unique tiles (sum across sets): {total_tiles}")
    print(f"Tile data bytes (sum across sets): {total_bytes}")
    print("Every tileset fits a single 256-tile 8bpp BG character block.")

if __name__ == "__main__":
    main()
