#!/usr/bin/env python3
"""Build native-resolution GBA 4bpp scenes with per-tile palette banks.

No resampling is performed. Each source pixel maps to one GBA pixel.
Tiles are 8x8, deduplicated across H/V flips, and each tile selects one of
15 scene palette banks (bank 15 remains reserved for the HUD).
"""
from __future__ import annotations
import argparse
import collections
import json
import re
from pathlib import Path
from PIL import Image

def ident(s: str) -> str:
    s = re.sub(r"\W+", "_", s).strip("_")
    return s if s and not s[0].isdigit() else "scene_" + s

def bgr555(rgba):
    r, g, b, _a = rgba
    return ((r * 31 + 127) // 255) | (((g * 31 + 127) // 255) << 5) | (((b * 31 + 127) // 255) << 10)

def pack4(pixels):
    out = bytearray()
    for i in range(0, 64, 2):
        out.append((pixels[i] & 15) | ((pixels[i + 1] & 15) << 4))
    return bytes(out)

def c_bytes(values, width=16):
    return ",\n".join("    " + ", ".join(f"0x{v:02X}" for v in values[i:i+width])
                     for i in range(0, len(values), width))

def c_words(values, width=8):
    return ",\n".join("    " + ", ".join(f"0x{v:04X}" for v in values[i:i+width])
                     for i in range(0, len(values), width))

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("image", type=Path)
    ap.add_argument("--name", required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--palette-banks", type=int, default=15,
                    help="Scene banks; bank 15 is reserved for HUD by default")
    ap.add_argument("--preview-out", type=Path,
                    help="Optional PNG rendered from the generated 4bpp tile/map/palette data")
    args = ap.parse_args()

    if not 1 <= args.palette_banks <= 15:
        raise ValueError("palette-banks must be 1..15 (bank 15 is reserved for HUD)")
    image = Image.open(args.image).convert("RGBA")
    width, height = image.size
    map_w, map_h = (width + 7) // 8, (height + 7) // 8
    pixels = list(image.getdata())

    tile_rgba = []
    tile_sets = []
    frequencies = collections.Counter()
    for ty in range(map_h):
        for tx in range(map_w):
            tile = []
            colors = set()
            for py in range(8):
                for px in range(8):
                    sx, sy = tx * 8 + px, ty * 8 + py
                    if sx >= width or sy >= height:
                        color = (0, 0, 0, 0)
                    else:
                        color = pixels[sy * width + sx]
                        if color[3] < 128:
                            color = (0, 0, 0, 0)
                    tile.append(color)
                    if color[3]:
                        colors.add(color)
                        frequencies[color] += 1
            if len(colors) > 15:
                raise ValueError(f"tile ({tx},{ty}) has {len(colors)} opaque colors; 4bpp supports 15")
            tile_rgba.append(tile)
            tile_sets.append(frozenset(colors))

    # Group tile palettes into at most 15 banks, with each bank holding
    # transparent index 0 plus at most 15 opaque colors.
    unique_sets = sorted(set(tile_sets),
                         key=lambda s: (-len(s), tuple(sorted(s))))
    banks: list[set] = []
    set_to_bank = {}
    for colors in unique_sets:
        candidates = [
            (len(bank | set(colors)) - len(bank), len(bank), i)
            for i, bank in enumerate(banks)
            if len(bank | set(colors)) <= 15
        ]
        if candidates:
            _, _, bank_index = min(candidates)
            banks[bank_index].update(colors)
        elif len(banks) < args.palette_banks:
            banks.append(set(colors))
            bank_index = len(banks) - 1
        else:
            raise ValueError(f"tile palettes require more than {args.palette_banks} banks")
        set_to_bank[colors] = bank_index

    bank_colors = []
    bank_indices = []
    palette = [0] * 256
    for bank_index, colors in enumerate(banks):
        ordered = sorted(colors, key=lambda c: (-frequencies[c], c))
        bank_colors.append(ordered)
        mapping = {color: i + 1 for i, color in enumerate(ordered)}
        bank_indices.append(mapping)
        for i, color in enumerate(ordered, 1):
            palette[bank_index * 16 + i] = bgr555(color)

    # Tile zero is the transparent pattern. Store all other tiles in canonical
    # H/V-flipped orientation and preserve the inverse flip in each map cell.
    tile_lookup = {bytes(32): 0}
    tile_data = bytearray(32)
    map_cells = []
    for tile, colors in zip(tile_rgba, tile_sets):
        bank = set_to_bank[colors]
        mapping = bank_indices[bank]
        indexed = [0 if color[3] < 128 else mapping[color] for color in tile]
        grid = [indexed[y*8:(y+1)*8] for y in range(8)]
        variants = [
            (grid, 0),
            ([row[::-1] for row in grid], 1),
            (grid[::-1], 2),
            ([row[::-1] for row in grid[::-1]], 3),
        ]
        packed_variants = [(pack4([v for row in variant for v in row]), flags)
                           for variant, flags in variants]
        raw, flip_flags = min(packed_variants, key=lambda item: (item[0], item[1]))
        tile_id = tile_lookup.get(raw)
        if tile_id is None:
            tile_id = len(tile_lookup)
            tile_lookup[raw] = tile_id
            tile_data.extend(raw)
        flags = flip_flags | (bank << 2)
        map_cells.append((tile_id, flags))

    name = ident(args.name)
    out = args.out
    out.mkdir(parents=True, exist_ok=True)
    guard = name.upper() + "_NATIVE_H"
    header = f"""#ifndef {guard}
#define {guard}
#include "core/native_scene.h"
extern const NativeSceneAsset g_{name}_native_scene;
#endif
"""
    source = [
        f'#include "{name}_native.h"',
        f'const uint8_t {name}_native_tiles[{len(tile_data)}] __attribute__((aligned(4))) = {{',
        c_bytes(list(tile_data)), "};",
        f'const NativeSceneCell {name}_native_map[{len(map_cells)}] __attribute__((aligned(4))) = {{',
    ]
    for i in range(0, len(map_cells), 6):
        source.append("    " + ", ".join(
            "{%d, %d}" % map_cells[j] for j in range(i, min(i+6, len(map_cells)))
        ) + ",")
    source += [
        "};",
        f'const uint16_t {name}_native_palette[256] __attribute__((aligned(4))) = {{',
        c_words(palette), "};",
        f'const NativeSceneAsset g_{name}_native_scene = {{',
        f'    {name}_native_tiles, {name}_native_map, {name}_native_palette,',
        f'    {len(tile_lookup)}, {map_w}, {map_h}, {len(tile_data)}',
        "};",
    ]
    (out / f"{name}_native.h").write_text(header, encoding="utf-8")
    (out / f"{name}_native.c").write_text("\n".join(source) + "\n", encoding="utf-8")
    report = {
        "name": args.name,
        "source_size": [width, height],
        "map_tiles": [map_w, map_h],
        "native_scale": 1,
        "resampling": "none",
        "unique_tiles_after_hv_dedup": len(tile_lookup),
        "tile_bytes_4bpp": len(tile_data),
        "palette_banks_used": len(banks),
        "palette_bank_sizes": [len(b) for b in banks],
        "opaque_colors": len(set().union(*banks)) if banks else 0,
        "map_cells": len(map_cells),
        "map_bytes": len(map_cells) * 4,
        "max_source_tile_id": max(tile_id for tile_id, _ in map_cells),
        "fits_1024_tile_vram_cache": True,
        "fits_32k_vram_if_fully_loaded": len(tile_data) <= 32768,
    }
    (out / f"{name}_native_manifest.json").write_text(json.dumps(report, indent=2) + "\n")
    if args.preview_out:
        preview_image = Image.new("RGBA", (width, height), (0, 0, 0, 0))
        for cell_index, (tile_id, flags) in enumerate(map_cells):
            bank = (flags >> 2) & 15
            flip_h = flags & 1
            flip_v = flags & 2
            tx, ty = cell_index % map_w, cell_index // map_w
            raw = tile_data[tile_id * 32:(tile_id + 1) * 32]
            for py in range(8):
                for px in range(8):
                    sx = 7 - px if flip_h else px
                    sy = 7 - py if flip_v else py
                    packed = raw[sy * 4 + sx // 2]
                    color_index = (packed >> 4) & 15 if sx & 1 else packed & 15
                    if color_index == 0:
                        rgba = (0, 0, 0, 0)
                    else:
                        word = palette[bank * 16 + color_index]
                        r5, g5, b5 = word & 31, (word >> 5) & 31, (word >> 10) & 31
                        rgba = ((r5 * 255 + 15) // 31,
                                (g5 * 255 + 15) // 31,
                                (b5 * 255 + 15) // 31, 255)
                    dx, dy = tx * 8 + px, ty * 8 + py
                    if dx < width and dy < height:
                        preview_image.putpixel((dx, dy), rgba)
        args.preview_out.parent.mkdir(parents=True, exist_ok=True)
        preview_image.save(args.preview_out)

        source_pixels = list(image.getdata())
        preview_pixels = list(preview_image.getdata())
        differing = 0
        for source_rgba, preview_rgba in zip(source_pixels, preview_pixels):
            if source_rgba[3] < 128:
                expected = (0, 0, 0, 0)
            else:
                expected = tuple(
                    ((((channel * 31 + 127) // 255) * 255 + 15) // 31)
                    for channel in source_rgba[:3]
                ) + (255,)
            if expected != preview_rgba:
                differing += 1
        print(f"4bpp native round-trip differing pixels: {differing}")

    print(json.dumps(report, indent=2))

if __name__ == "__main__":
    main()
