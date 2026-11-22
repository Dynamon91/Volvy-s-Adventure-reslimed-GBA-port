#!/usr/bin/env python3
"""Build a source-sprite-AABB collision grid for rm_w1_b1_00.

The output is a 332x70 grid at 8 GBA pixels per cell. This is derived from
GameMaker object sprite bounds and instance transforms, not pixel masks or
decompiled collision scripts.
"""
from __future__ import annotations
import argparse, collections, json, math, struct, sys
from pathlib import Path

TILE_EMPTY = 0
TILE_GROUND = 1
TILE_BREAKABLE = 2
TILE_RAIL = 3
TILE_HAZARD = 5
TILE_ONEWAY = 7
TILE_BREAKABLE_ONEWAY = 8
TILE_SWOLVY_BLOCK = 9

COLLISION_TYPES = {
    "obj_solid": TILE_GROUND,
    "obj_destructible_block": TILE_BREAKABLE,
    "obj_destructible_block_swolvy": TILE_SWOLVY_BLOCK,
    "obj_destructible_block_oneway": TILE_BREAKABLE_ONEWAY,
    "obj_cloud_platform": TILE_ONEWAY,
    "obj_spikes_dave": TILE_HAZARD,
    "obj_rail_dave": TILE_RAIL,
}
PRIORITY = {TILE_EMPTY: 0, TILE_RAIL: 1, TILE_ONEWAY: 1,
            TILE_BREAKABLE: 2, TILE_BREAKABLE_ONEWAY: 2,
            TILE_SWOLVY_BLOCK: 2,
            TILE_GROUND: 3, TILE_HAZARD: 4}

def c_array(values, width=24):
    return ",\n".join("    " + ", ".join(str(v) for v in values[i:i+width])
                     for i in range(0, len(values), width))

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("room_json", type=Path)
    ap.add_argument("data_win", type=Path)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--scale", type=int, default=2)
    args = ap.parse_args()

    room = json.loads(args.room_json.read_text(encoding="utf-8"))
    sys.path.insert(0, str(Path(__file__).resolve().parent / "vendor"))
    from gm_extract.gamedata import GameData
    from gm_extract.resources import parse_sprites

    game = GameData(str(args.data_win))
    object_names = game.resource_names("OBJT")
    sprites = parse_sprites(game)
    obj_off, _ = game.chunk("OBJT")
    sprite_defs = {}
    for object_name in COLLISION_TYPES:
        object_index = object_names.index(object_name)
        object_ptr = struct.unpack_from("<I", game.data,
                                        obj_off + 4 + 4 * object_index)[0]
        sprite_index = struct.unpack_from("<i", game.data, object_ptr + 4)[0]
        if not 0 <= sprite_index < len(sprites):
            raise ValueError(f"{object_name} has no valid sprite")
        sprite = sprites[sprite_index]
        sprite_defs[object_name] = (
            sprite["width"], sprite["height"],
            sprite["origin_x"], sprite["origin_y"]
        )

    scale = args.scale
    room_width = (room["width"] + scale * 8 - 1) // (scale * 8)
    room_height = (room["height"] + scale * 8 - 1) // (scale * 8)
    grid = [TILE_EMPTY] * (room_width * room_height)
    placed_counts = collections.Counter()
    cell_counts = collections.Counter()
    skipped = collections.Counter()

    for instance in room["instances"]:
        object_name = instance["object"]
        if object_name not in COLLISION_TYPES:
            continue
        sw, sh, ox, oy = sprite_defs[object_name]
        sx, sy = instance["scale_x"], instance["scale_y"]
        angle = math.radians(instance["rotation"])
        cos_a, sin_a = math.cos(angle), math.sin(angle)
        px, py = instance["x"], instance["y"]

        corners = []
        for lx, ly in ((-ox*sx, -oy*sy),
                       ((sw-ox)*sx, -oy*sy),
                       (-ox*sx, (sh-oy)*sy),
                       ((sw-ox)*sx, (sh-oy)*sy)):
            rx = px + lx*cos_a - ly*sin_a
            ry = py + lx*sin_a + ly*cos_a
            corners.append((rx / scale, ry / scale))
        min_x = min(p[0] for p in corners)
        max_x = max(p[0] for p in corners)
        min_y = min(p[1] for p in corners)
        max_y = max(p[1] for p in corners)
        tx0 = max(0, math.floor(min_x / 8))
        ty0 = max(0, math.floor(min_y / 8))
        tx1 = min(room_width, math.ceil(max_x / 8))
        ty1 = min(room_height, math.ceil(max_y / 8))
        if tx1 <= tx0 or ty1 <= ty0:
            skipped[object_name] += 1
            continue

        tile = COLLISION_TYPES[object_name]
        placed_counts[object_name] += 1
        for ty in range(ty0, ty1):
            for tx in range(tx0, tx1):
                index = ty * room_width + tx
                if PRIORITY[tile] >= PRIORITY[grid[index]]:
                    grid[index] = tile
                    cell_counts[tile] += 1

    args.out.mkdir(parents=True, exist_ok=True)
    header = f"""#ifndef BONUS_W1_COLLISION_H
#define BONUS_W1_COLLISION_H
#include <stdint.h>
#define BONUS_W1_COLLISION_WIDTH {room_width}
#define BONUS_W1_COLLISION_HEIGHT {room_height}
#define BONUS_W1_COLLISION_CELLS {len(grid)}
#define BONUS_W1_COLLISION_SOURCE_SCALE {scale}
extern const uint8_t bonus_w1_collision[BONUS_W1_COLLISION_CELLS];
#endif
"""
    source = (
        '#include "bonus_w1_collision.h"\n'
        'const uint8_t bonus_w1_collision[BONUS_W1_COLLISION_CELLS] = {\n'
        + c_array(grid) + '\n};\n'
    )
    (args.out / "bonus_w1_collision.h").write_text(header, encoding="utf-8")
    (args.out / "bonus_w1_collision.c").write_text(source, encoding="utf-8")
    final_cell_counts = collections.Counter(grid)
    report = {
        "room": room["name"], "source_dimensions": [room["width"], room["height"]],
        "collision_grid": [room_width, room_height], "source_scale": scale,
        "sprite_aabb_types": sprite_defs,
        "object_placements_used": dict(placed_counts),
        "collision_cell_counts": {str(k): v for k, v in final_cell_counts.items()},
        "unsupported_collision_types": {
            "obj_moving_platform": "two source instances handled by dynamic platform controller; intentionally not in static grid",
            "obj_hole": "visual/hazard semantics not confirmed",
            "obj_hole_dir": "visual/hazard semantics not confirmed"
        },
        "notes": [
            "AABBs are derived from source sprite dimensions, origins, scales and rotations.",
            "These are not pixel-perfect collision masks.",
            "Only cloud platforms are static one-way cells; moving platforms use runtime AABBs and provisional horizontal motion.",
            "One-way destructible blocks break on downward ground-pound/Swolvy contact only.",
            "Swolvy-only source blocks have distinct tile 9; normal Volvy cannot destroy them.",
            "Spikes are represented as hazardous cells; exact hurt masks remain provisional."
        ]
    }
    (args.out / "bonus_w1_collision_report.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({
        "grid": [room_width, room_height],
        "placements": dict(placed_counts),
        "cells": {str(k): v for k, v in final_cell_counts.items()},
        "output": str(args.out)
    }, indent=2))

if __name__ == "__main__":
    main()
