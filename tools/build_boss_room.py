#!/usr/bin/env python3
"""Build source-derived Shrewy arena collision and full prop-transform tables.

The tool exports original room placements and sprite-AABB-derived collision.
It preserves editor/helper object transforms as metadata and does not infer
boss AI or claim pixel-mask accuracy.
"""
from __future__ import annotations
import argparse
import json
import math
import struct
import sys
from collections import Counter
from pathlib import Path

PROP_TYPES = {
    "obj_onion": "SHREWY_PROP_ONION",
    "obj_cage": "SHREWY_PROP_CAGE",
    "obj_autohole": "SHREWY_PROP_AUTOHOLE",
    "obj_cone_emitter": "SHREWY_PROP_EMITTER",
    "obj_adjustable_autodigger": "SHREWY_PROP_AUTODIGGER",
}


def transformed_aabb(inst, sprite, scale_divisor):
    """AABB of the scaled and rotated source sprite around its origin pivot."""
    sx = float(inst["scale_x"])
    sy = float(inst["scale_y"])
    angle = math.radians(float(inst["rotation"]))
    c, s = math.cos(angle), math.sin(angle)
    ox, oy = sprite["origin_x"], sprite["origin_y"]
    w, h = sprite["width"], sprite["height"]
    points = []
    for px, py in ((0, 0), (w, 0), (0, h), (w, h)):
        dx = (px - ox) * sx
        dy = (py - oy) * sy
        rx = dx * c - dy * s
        ry = dx * s + dy * c
        points.append(((float(inst["x"]) + rx) / scale_divisor,
                       (float(inst["y"]) + ry) / scale_divisor))
    left = math.floor(min(p[0] for p in points))
    top = math.floor(min(p[1] for p in points))
    right = math.ceil(max(p[0] for p in points))
    bottom = math.ceil(max(p[1] for p in points))
    return left, top, right, bottom


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("room_data", type=Path)
    ap.add_argument("--data-win", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--scale", type=int, default=1)
    args = ap.parse_args()
    if args.scale < 1:
        raise ValueError("scale must be a positive integer")

    sys.path.insert(0, str(Path(__file__).resolve().parent / "vendor"))
    from gm_extract.gamedata import GameData
    from gm_extract.resources import parse_sprites

    room = json.loads(args.room_data.read_text(encoding="utf-8"))
    if room["name"] != "rm_shrewy":
        raise ValueError("Expected dedicated source room rm_shrewy")
    game = GameData(str(args.data_win))
    object_names = game.resource_names("OBJT")
    sprites = parse_sprites(game)
    sprite_by_index = {s["index"]: s for s in sprites}
    obj_off, _ = game.chunk("OBJT")

    needed = {"obj_solid", "obj_owp", "obj_boss_shrewy", "obj_volvy", *PROP_TYPES}
    sprite_defs = {}
    for name in needed:
        idx = object_names.index(name)
        ptr = struct.unpack_from("<I", game.data, obj_off + 4 + 4 * idx)[0]
        sprite_idx = struct.unpack_from("<i", game.data, ptr + 4)[0]
        sprite_defs[name] = sprite_by_index.get(sprite_idx)
        if name in ("obj_solid", "obj_owp", "obj_boss_shrewy", "obj_volvy", *PROP_TYPES) and sprite_defs[name] is None:
            raise ValueError(f"Missing sprite for required room object {name}")

    width = (room["width"] + 7) // 8
    height = (room["height"] + 7) // 8
    collision = bytearray(width * height)
    props = []
    source_boss = None
    source_player = None
    floor_y = None
    solid_count = 0
    owp_count = 0
    source_instances = []

    for inst in room["instances"]:
        name = inst["object"]
        if name == "obj_boss_shrewy":
            source_boss = [int(inst["x"]), int(inst["y"])]
        elif name == "obj_volvy":
            source_player = [int(inst["x"]), int(inst["y"])]

        if name not in sprite_defs or sprite_defs[name] is None:
            continue
        spr = sprite_defs[name]
        left, top, right, bottom = transformed_aabb(inst, spr, args.scale)

        if name == "obj_solid":
            solid_count += 1
            tx0 = max(0, math.floor(left / 8))
            ty0 = max(0, math.floor(top / 8))
            tx1 = min(width, math.ceil(right / 8))
            ty1 = min(height, math.ceil(bottom / 8))
            for ty in range(ty0, ty1):
                for tx in range(tx0, tx1):
                    collision[ty * width + tx] = 1
        elif name == "obj_owp":
            owp_count += 1
            # Source OWP is a broad one-way floor. The top AABB edge is the
            # measured surface; full-width rasterization matches its extent.
            floor_y = round(top / 8) * 8
            row = max(0, min(height - 1, floor_y // 8))
            for tx in range(width):
                collision[row * width + tx] = 1

        if name in PROP_TYPES:
            w, h = right - left, bottom - top
            if w < 1 or h < 1 or w > 255 or h > 255:
                raise ValueError(f"{name} transformed AABB {w}x{h} exceeds prop dimensions")
            prop = {
                "kind": PROP_TYPES[name],
                "source_object": name,
                "source_id": int(inst["instance_id"]),
                "x": round(float(inst["x"]) / args.scale),
                "y": round(float(inst["y"]) / args.scale),
                "left": left, "top": top, "width": w, "height": h,
                "scale_x_q8": round(float(inst["scale_x"]) * 256),
                "scale_y_q8": round(float(inst["scale_y"]) * 256),
                "rotation_tenths": round(float(inst["rotation"]) * 10),
                "sprite": spr["name"],
                "sprite_size": [spr["width"], spr["height"]],
                "sprite_origin": [spr["origin_x"], spr["origin_y"]],
            }
            if not -32768 <= prop["scale_x_q8"] <= 32767 or not -32768 <= prop["scale_y_q8"] <= 32767:
                raise ValueError("Q8 scale exceeds int16")
            if not -32768 <= prop["rotation_tenths"] <= 32767:
                raise ValueError("rotation_tenths exceeds int16")
            props.append(prop)
            source_instances.append(prop)

    if source_boss is None or source_player is None or floor_y is None:
        raise ValueError("Boss, player or one-way floor placement missing")

    out = args.out
    out.mkdir(parents=True, exist_ok=True)
    header = f'''#ifndef VOLVY_SHREWY_ROOM_H
#define VOLVY_SHREWY_ROOM_H
#include <stdint.h>
#define SHREWY_ROOM_WIDTH {room['width'] // args.scale}
#define SHREWY_ROOM_HEIGHT {room['height'] // args.scale}
#define SHREWY_ROOM_GRID_WIDTH {width}
#define SHREWY_ROOM_GRID_HEIGHT {height}
#define SHREWY_ROOM_PROP_COUNT {len(props)}
#define SHREWY_ROOM_SOLID_OBJECTS {solid_count}
#define SHREWY_ROOM_ONEWAY_OBJECTS {owp_count}
#define SHREWY_BOSS_SOURCE_X {source_boss[0]}
#define SHREWY_BOSS_SOURCE_Y {source_boss[1]}
#define SHREWY_PLAYER_SOURCE_X {source_player[0]}
#define SHREWY_PLAYER_SOURCE_Y {source_player[1]}
#define SHREWY_ARENA_FLOOR_Y {floor_y}
typedef enum ShrewyPropKind {{
    SHREWY_PROP_ONION=1, SHREWY_PROP_CAGE, SHREWY_PROP_AUTOHOLE,
    SHREWY_PROP_EMITTER, SHREWY_PROP_AUTODIGGER
}} ShrewyPropKind;
typedef struct ShrewyProp {{
    uint16_t kind;
    uint16_t reserved;
    uint32_t source_id;
    int16_t x, y, left, top;
    uint8_t width, height;
    int16_t scale_x_q8, scale_y_q8, rotation_tenths;
}} ShrewyProp;
extern const uint8_t shrewy_collision[SHREWY_ROOM_GRID_WIDTH * SHREWY_ROOM_GRID_HEIGHT];
extern const ShrewyProp shrewy_props[SHREWY_ROOM_PROP_COUNT];
#endif
'''
    rows = ['#include "shrewy_room.h"',
            'const uint8_t shrewy_collision[SHREWY_ROOM_GRID_WIDTH * SHREWY_ROOM_GRID_HEIGHT] = {']
    for i in range(0, len(collision), 32):
        rows.append('    ' + ', '.join(str(v) for v in collision[i:i+32]) + ',')
    rows += ['};', 'const ShrewyProp shrewy_props[SHREWY_ROOM_PROP_COUNT] = {']
    for p in props:
        rows.append('    {%s,0,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d},' % (
            p['kind'], p['source_id'], p['x'], p['y'], p['left'], p['top'],
            p['width'], p['height'], p['scale_x_q8'], p['scale_y_q8'],
            p['rotation_tenths']))
    rows += ['};']
    (out / 'shrewy_room.h').write_text(header, encoding='utf-8')
    (out / 'shrewy_room.c').write_text('\n'.join(rows) + '\n', encoding='utf-8')

    report = {
        "source_room": room["name"], "source_size": [room["width"], room["height"]],
        "gba_scale": args.scale, "collision_grid": [width, height],
        "solid_objects": solid_count, "oneway_platforms": owp_count,
        "floor_y_derived_from_owp_aabb": floor_y,
        "boss_source_pivot": source_boss, "player_source_pivot": source_player,
        "boss_sprite": {"size": [sprite_defs['obj_boss_shrewy']['width'], sprite_defs['obj_boss_shrewy']['height']],
                        "origin": [sprite_defs['obj_boss_shrewy']['origin_x'], sprite_defs['obj_boss_shrewy']['origin_y']]},
        "player_sprite": {"size": [sprite_defs['obj_volvy']['width'], sprite_defs['obj_volvy']['height']],
                          "origin": [sprite_defs['obj_volvy']['origin_x'], sprite_defs['obj_volvy']['origin_y']]},
        "props": dict(Counter(p["kind"] for p in props)),
        "prop_count": len(props),
        "prop_transforms": source_instances,
        "collision_note": "AABB-derived solid objects and a rasterized one-way platform surface; not pixel-mask collision.",
        "spawn_note": "The source player pivot lies below the 144px room; runtime relocation is not decoded. A safe floor-aligned spawn must remain provisional.",
    }
    (out / 'shrewy_room_report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({k: report[k] for k in ("source_room", "source_size", "collision_grid", "solid_objects", "oneway_platforms", "props", "prop_count")}, indent=2))

if __name__ == '__main__':
    main()
