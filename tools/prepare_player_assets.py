#!/usr/bin/env python3
"""Generate GBA 8bpp OBJ sprite C arrays from the private extracted manifest.

All selected player sprites share one exact 8bpp OBJ palette. Source pixel art
is downscaled with nearest-neighbor sampling (default 2:1). Each frame is
split into legal GBA OBJ pieces no larger than 32x32.
"""

from __future__ import annotations
import argparse
import collections
import json
import re
from pathlib import Path
from PIL import Image

ALLOWED = [
    (8, 8, 0, 0), (16, 16, 0, 1), (32, 32, 0, 2),
    (16, 8, 1, 0), (32, 8, 1, 1), (32, 16, 1, 2),
    (8, 16, 2, 0), (8, 32, 2, 1), (16, 32, 2, 2),
]

def ident(s: str) -> str:
    s = re.sub(r"\W+", "_", s)
    if not s or s[0].isdigit():
        s = "asset_" + s
    return s

def bgr555(rgba):
    r, g, b, _a = rgba
    r5 = (r * 31 + 127) // 255
    g5 = (g * 31 + 127) // 255
    b5 = (b * 31 + 127) // 255
    return r5 | (g5 << 5) | (b5 << 10)

def choose_hw_size(w, h):
    candidates = [x for x in ALLOWED if x[0] >= w and x[1] >= h]
    if not candidates:
        raise ValueError(f"Part {w}x{h} cannot fit a single GBA OBJ.")
    return min(candidates, key=lambda x: (x[0]*x[1] - w*h,
                                          abs(x[0]/x[1] - w/h)))

def tile_bytes(indices, hw_w, hw_h):
    out = bytearray()
    for ty in range(0, hw_h, 8):
        for tx in range(0, hw_w, 8):
            for py in range(8):
                for px in range(8):
                    x, y = tx + px, ty + py
                    out.append(indices[y * hw_w + x])
    return bytes(out)

def c_bytes(data):
    lines = []
    for i in range(0, len(data), 16):
        lines.append("    " + ", ".join(f"0x{v:02X}" for v in data[i:i+16]))
    return ",\n".join(lines)

def c_words(data):
    lines = []
    for i in range(0, len(data), 8):
        lines.append("    " + ", ".join(f"0x{v:04X}" for v in data[i:i+8]))
    return ",\n".join(lines)

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("manifest", type=Path)
    ap.add_argument("--assets-root", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--scale", type=int, default=2)
    ap.add_argument("--frame-duration", type=int, default=6)
    args = ap.parse_args()

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    player_render_sprites = {
        "spr_idle", "spr_walk", "spr_jump", "spr_float", "spr_dash", "spr_dash_up",
        "spr_grind", "spr_ground_pound", "spr_dig", "spr_punch", "spr_punch_up",
        "spr_swim", "spr_swim_dash", "spr_volvy_hurt", "spr_volvy_collect",
        "spr_volvy_windup", "spr_volvy_clear", "spr_volvy_clear_2", "spr_volvy_title",
        "spr_swolvy_idle", "spr_swolvy_walk", "spr_swolvy_jump", "spr_swolvy_float",
        "spr_swolvy_grind", "spr_swolvy_ground_pound", "spr_swolvy_punch",
        "spr_swolvy_punch_up", "spr_swolvy_swim", "spr_swolvy_transform", "spr_dig_swole",
        "spr_onion", "spr_destructible_block", "spr_destructible_block_swolvy",
        "spr_bunny_hop", "spr_bumper_sq", "spr_bumper_tri",
        "spr_moving_platform", "spr_moving_platform_wide", "spr_cloud_platform",
        "spr_action_block", "spr_action_block_spent",
        "spr_squirrel_idle", "spr_squirrel_walk", "spr_squirrel_hurt", "spr_springboard"
    }
    sprites = [s for s in manifest["selected_sprites"] if s["name"] in player_render_sprites]
    # Reproduce the source room's 4/3 X-scale instance without shipping an
    # extra source PNG. The original 48x8 moving-platform art becomes 64x8
    # (32x4 at 2:1) before the normal OBJ converter runs.
    if not any(s["name"] == "spr_moving_platform_wide" for s in sprites):
        original = next((s for s in sprites if s["name"] == "spr_moving_platform"), None)
        if original is not None:
            wide = dict(original)
            wide["name"] = "spr_moving_platform_wide"
            wide["width"] = 64
            wide["origin"] = [32, 8]
            sprites.append(wide)
    images = {}
    colors = collections.Counter()

    for sprite in sprites:
        name = sprite["name"]
        images[name] = []
        for frame in sprite["frames"]:
            path = args.assets_root / frame["file"]
            im = Image.open(path).convert("RGBA")
            if name == "spr_moving_platform_wide":
                im = im.resize((64, im.height), Image.Resampling.NEAREST)
            w = max(1, (im.width + args.scale - 1) // args.scale)
            h = max(1, (im.height + args.scale - 1) // args.scale)
            im = im.resize((w, h), Image.Resampling.NEAREST)
            pixels = list(im.getdata())
            colors.update(px for px in pixels if px[3] >= 128)
            images[name].append((im, frame))

    palette_colors = [c for c, _n in colors.most_common()]
    if len(palette_colors) > 255:
        raise ValueError(f"{len(palette_colors)} opaque colors exceed the 8bpp palette.")
    palette = [(0, 0, 0, 0)] + palette_colors
    color_index = {c: i + 1 for i, c in enumerate(palette_colors)}
    palette_words = [0] + [bgr555(c) for c in palette_colors]
    palette_words += [0] * (256 - len(palette_words))

    out_dir = args.out
    out_dir.mkdir(parents=True, exist_ok=True)
    header_path = out_dir / "player_sprites.h"
    source_path = out_dir / "player_sprites.c"

    header = """#ifndef VOLVY_PLAYER_SPRITES_H
#define VOLVY_PLAYER_SPRITES_H

#include <stdint.h>

typedef struct GbaSpritePart {
    const uint8_t *tiles;
    uint16_t tile_bytes;
    uint8_t offset_x;
    uint8_t offset_y;
    uint8_t hw_w;
    uint8_t hw_h;
    uint8_t shape;
    uint8_t size_code;
} GbaSpritePart;

typedef struct GbaSpriteFrame {
    const GbaSpritePart *parts;
    uint8_t part_count;
    uint8_t width;
    uint8_t height;
    uint8_t origin_x;
    uint8_t origin_y;
} GbaSpriteFrame;

typedef struct GbaSpriteAnim {
    const GbaSpriteFrame *frames;
    uint8_t frame_count;
    uint8_t frame_duration;
} GbaSpriteAnim;

extern const uint16_t g_player_obj_palette[256];
"""
    for sprite in sprites:
        header += f"extern const GbaSpriteAnim g_{ident(sprite['name'])};\n"
    header += "\n#endif\n"

    source_lines = [
        '#include "player_sprites.h"',
        "",
        "const uint16_t g_player_obj_palette[256] = {",
        c_words(palette_words),
        "};",
        "",
    ]

    total_parts = 0
    total_tile_bytes = 0
    meta_sprites = []

    for sprite in sprites:
        name = sprite["name"]
        cname = ident(name)
        frame_meta = []
        frame_array_names = []

        for frame_idx, (im, original_frame) in enumerate(images[name]):
            w, h = im.size
            pxs = list(im.getdata())
            indices = [
                0 if px[3] < 128 else color_index[px]
                for px in pxs
            ]

            parts = []
            part_array = f"s_{cname}_f{frame_idx}_parts"
            # Split into 32x32 source regions; each region becomes a legal OBJ.
            for py in range(0, h, 32):
                for px in range(0, w, 32):
                    part_w = min(32, w - px)
                    part_h = min(32, h - py)
                    hw_w, hw_h, shape, size_code = choose_hw_size(part_w, part_h)
                    padded = [0] * (hw_w * hw_h)
                    for yy in range(part_h):
                        src_start = (py + yy) * w + px
                        dst_start = yy * hw_w
                        padded[dst_start:dst_start + part_w] = indices[src_start:src_start + part_w]
                    raw = tile_bytes(padded, hw_w, hw_h)
                    tile_name = f"s_{cname}_f{frame_idx}_p{len(parts)}_tiles"
                    source_lines.extend([
                        f"static const uint8_t {tile_name}[] __attribute__((aligned(4))) = {{",
                        c_bytes(raw),
                        "};",
                        "",
                    ])
                    parts.append({
                        "tile_name": tile_name, "tile_bytes": len(raw),
                        "x": px, "y": py, "hw_w": hw_w, "hw_h": hw_h,
                        "shape": shape, "size_code": size_code,
                    })
                    total_parts += 1
                    total_tile_bytes += len(raw)

            source_lines.append(f"static const GbaSpritePart {part_array}[] = {{")
            for part in parts:
                source_lines.append(
                    "    {%s, %d, %d, %d, %d, %d, %d, %d}," % (
                        part["tile_name"], part["tile_bytes"], part["x"], part["y"],
                        part["hw_w"], part["hw_h"], part["shape"], part["size_code"]
                    )
                )
            source_lines.extend(["};", ""])
            frame_array_names.append(part_array)
            frame_meta.append({
                "frame": frame_idx, "width": w, "height": h,
                "origin_x": round(sprite["origin"][0] / args.scale) if "origin" in sprite else round(sprite["origin"][0] / args.scale),
                "origin_y": round(sprite["origin"][1] / args.scale) if "origin" in sprite else round(sprite["origin"][1] / args.scale),
                "parts": parts,
            })

        frame_array = f"s_{cname}_frames"
        source_lines.append(f"static const GbaSpriteFrame {frame_array}[] = {{")
        for fm, part_array in zip(frame_meta, frame_array_names):
            source_lines.append(
                "    {%s, %d, %d, %d, %d, %d}," % (
                    part_array, len(fm["parts"]), fm["width"], fm["height"],
                    fm["origin_x"], fm["origin_y"]
                )
            )
        source_lines.extend(["};", ""])
        duration = 10 if "idle" in name else args.frame_duration
        source_lines.append(
            f"const GbaSpriteAnim g_{cname} = "
            f"{{{frame_array}, {len(frame_meta)}, {duration}}};"
        )
        source_lines.append("")
        meta_sprites.append({
            "name": name,
            "scaled_frame_count": len(frame_meta),
            "frames": frame_meta,
        })

    header_path.write_text(header, encoding="utf-8")
    source_path.write_text("\n".join(source_lines), encoding="utf-8")
    out_manifest = {
        "scale_divisor": args.scale,
        "opaque_palette_colors": len(palette_colors),
        "palette_bgr555": palette_words[:len(palette_colors)+1],
        "sprites": meta_sprites,
        "total_parts": total_parts,
        "total_tile_bytes": total_tile_bytes,
    }
    (out_dir / "player_sprites_manifest.json").write_text(
        json.dumps(out_manifest, indent=2), encoding="utf-8"
    )
    print(f"Sprites: {len(sprites)}")
    print(f"Shared 8bpp OBJ palette: {len(palette_colors)} opaque colors")
    print(f"OBJ parts: {total_parts}; tile data: {total_tile_bytes} bytes")
    print(f"Generated: {source_path} and {header_path}")

if __name__ == "__main__":
    main()
