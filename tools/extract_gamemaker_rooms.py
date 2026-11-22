#!/usr/bin/env python3
"""Extract GameMaker ROOM tile layers and render source-resolution PNG previews.

This parser targets the observed 2024.2+ room layout in the supplied data.win:
25-word room header, 2022.1+ layer effect header, and RLE-compressed tile data.
It exports derived map data and visual previews for private reconstruction work.
"""

from __future__ import annotations
import argparse
import json
import struct
from collections import Counter
from pathlib import Path
from PIL import Image, ImageOps

def i32(data, off):
    return struct.unpack_from("<i", data, off)[0]

def u32(data, off):
    return struct.unpack_from("<I", data, off)[0]

def f32(data, off):
    return struct.unpack_from("<f", data, off)[0]

def read_ptrs(data, off):
    count = u32(data, off)
    if count > 100000:
        raise ValueError(f"implausible pointer count {count} at 0x{off:X}")
    return list(struct.unpack_from("<" + "I" * count, data, off + 4))

def decode_rle_tiles(data, offset, total):
    values = []
    pos = offset
    while len(values) < total:
        if pos >= len(data):
            raise ValueError("tile RLE stream ended early")
        length = data[pos]
        pos += 1
        if length >= 128:
            run = (length & 0x7F) + 1
            tile = u32(data, pos)
            pos += 4
            values.extend([tile] * min(run, total - len(values)))
        else:
            run = min(length, total - len(values))
            if run == 0:
                raise ValueError("zero-length literal run in tile RLE")
            for _ in range(run):
                values.append(u32(data, pos))
                pos += 4
    return values, pos

def parse_room_layers(g, room_ptr):
    data = g.data
    fields = struct.unpack_from("<25I", data, room_ptr)
    name = g.read_name(fields[0])
    width, height = fields[2], fields[3]
    layer_table = fields[23]
    layer_ptrs = read_ptrs(data, layer_table)
    layers = []

    for layer_ptr in layer_ptrs:
        name_ptr, layer_id, layer_type, depth = struct.unpack_from("<4I", data, layer_ptr)
        xoff, yoff, hspeed, vspeed = struct.unpack_from("<4f", data, layer_ptr + 16)
        visible, effect_enabled, effect_type, effect_count = struct.unpack_from("<4I", data, layer_ptr + 32)
        body = layer_ptr + 48 + effect_count * 12
        layer = {
            "name": g.read_name(name_ptr), "id": layer_id, "type": layer_type,
            "depth": depth, "x_offset": xoff, "y_offset": yoff,
            "h_speed": hspeed, "v_speed": vspeed, "visible": bool(visible),
            "effect_enabled": bool(effect_enabled), "body_offset": body,
        }

        if layer_type == 1:
            bg_visible, foreground, sprite_index, h_tiled, v_tiled, stretch, color = struct.unpack_from("<IIiIII I".replace(" ", ""), data, body)
            first_frame, anim_speed = struct.unpack_from("<2f", data, body + 28)
            anim_speed_type = u32(data, body + 36)
            layer.update({
                "background_visible": bool(bg_visible),
                "foreground": bool(foreground),
                "sprite_index": sprite_index,
                "h_tiled": bool(h_tiled), "v_tiled": bool(v_tiled),
                "stretch": bool(stretch), "color": color,
                "first_frame": first_frame, "anim_speed": anim_speed,
                "anim_speed_type": anim_speed_type,
            })
        elif layer_type == 2:
            instance_count = u32(data, body)
            instance_ids = list(struct.unpack_from("<" + "I" * instance_count,
                                                   data, body + 4)) if instance_count else []
            layer["instance_ids"] = instance_ids
        elif layer_type == 4:
            bg_index = i32(data, body)
            tiles_x = u32(data, body + 4)
            tiles_y = u32(data, body + 8)
            total = tiles_x * tiles_y
            tile_data, end_pos = decode_rle_tiles(data, body + 12, total)
            layer.update({
                "background_index": bg_index,
                "tiles_x": tiles_x, "tiles_y": tiles_y,
                "tile_data": tile_data,
                "tile_stream_end": end_pos,
            })
        layers.append(layer)

    # Resolve room instance IDs to their object definitions and placement.
    object_names = g.resource_names("OBJT")
    game_object_ptrs = read_ptrs(data, fields[12])
    instances_by_id = {}
    all_instances = []
    for object_ptr in game_object_ptrs:
        vals = struct.unpack_from("<iiiIi3fiIfi", data, object_ptr)
        x, y, object_index, instance_id, creation_code = vals[:5]
        scale_x, scale_y, image_speed = vals[5:8]
        image_index, color, rotation, pre_create_code = vals[8:]
        object_name = object_names[object_index] if 0 <= object_index < len(object_names) else f"object_{object_index}"
        record = {
            "instance_id": instance_id, "object_index": object_index,
            "object": object_name, "x": x, "y": y,
            "scale_x": scale_x, "scale_y": scale_y,
            "image_speed": image_speed, "image_index": image_index,
            "color": color, "rotation": rotation,
            "creation_code": creation_code, "pre_create_code": pre_create_code,
        }
        instances_by_id[instance_id] = record
        all_instances.append(record)

    for layer in layers:
        if "instance_ids" in layer:
            layer["instances"] = [
                instances_by_id[i] for i in layer["instance_ids"]
                if i in instances_by_id
            ]

    return {
        "name": name, "width": width, "height": height,
        "room_ptr": room_ptr, "layers": layers,
        "instances": all_instances,
    }

def tile_source_image(tileset, tile_index, cache):
    name = tileset["name"]
    if name not in cache:
        image = Image.open(tileset["_path"]).convert("RGBA")
        tw, th = tileset["tile_size"]
        bx, by = tileset["border"]
        step_x, step_y = tw + bx * 2, th + by * 2
        cache[name] = (image, tw, th, bx, by, step_x, step_y)
    image, tw, th, bx, by, step_x, step_y = cache[name]
    cols = tileset["columns"]
    if tile_index < 0 or tile_index >= tileset["tile_count"]:
        return None
    tx, ty = tile_index % cols, tile_index // cols
    return image.crop((bx + tx * step_x, by + ty * step_y,
                       bx + tx * step_x + tw, by + ty * step_y + th))

def draw_layer(canvas, layer, tileset, cache):
    tw, th = tileset["tile_size"]
    data = layer["tile_data"]
    tiles_x, tiles_y = layer["tiles_x"], layer["tiles_y"]
    xoff, yoff = layer["x_offset"], layer["y_offset"]

    for idx, blob in enumerate(data):
        if blob == 0xFFFFFFFF:
            continue
        tile_index = blob & 0x7FFFF
        # In this game's maps, source tile index 0 is the editor's checkerboard
        # placeholder and is used as empty space, not visible level art.
        if tile_index == 0:
            continue
        tile = tile_source_image(tileset, tile_index, cache)
        if tile is None:
            continue
        # GameMaker tile blob: bit 28 mirror, bit 29 flip, bit 30 rotate.
        if blob & (1 << 30):
            tile = tile.transpose(Image.Transpose.ROTATE_90)
        if blob & (1 << 28):
            tile = ImageOps.mirror(tile)
        if blob & (1 << 29):
            tile = ImageOps.flip(tile)
        cx, cy = idx % tiles_x, idx // tiles_x
        px = round(xoff + cx * tw)
        py = round(yoff + cy * th)
        canvas.alpha_composite(tile, (px, py))

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("data_win", type=Path)
    ap.add_argument("--asset-manifest", type=Path, required=True)
    ap.add_argument("--assets-root", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--rooms", nargs="*", default=None)
    args = ap.parse_args()

    from pathlib import Path as _Path
    import sys
    sys.path.insert(0, str(_Path(__file__).resolve().parent / "vendor"))
    from gm_extract.gamedata import GameData

    g = GameData(str(args.data_win))
    manifest = json.loads(args.asset_manifest.read_text(encoding="utf-8"))
    tilesets = {}
    sprite_resources = {s["index"]: s for s in manifest["selected_sprites"] if "index" in s}
    for ts in manifest["tilesets"]:
        item = dict(ts)
        item["_path"] = args.assets_root / ts["file"]
        tilesets[ts["index"]] = item

    room_off, _ = g.chunk("ROOM")
    room_ptrs = read_ptrs(g.data, room_off)
    selected_names = set(args.rooms) if args.rooms else None
    out = args.out
    out.mkdir(parents=True, exist_ok=True)
    room_summaries = []
    image_cache = {}

    for room_ptr in room_ptrs:
        fields = struct.unpack_from("<25I", g.data, room_ptr)
        room_name = g.read_name(fields[0])
        if selected_names and room_name not in selected_names:
            continue

        room = parse_room_layers(g, room_ptr)
        canvas = Image.new("RGBA", (room["width"], room["height"]), (0, 0, 0, 0))
        layer_meta = []
        # Lower depth is in front in GameMaker; render from highest depth to lowest.
        drawable_layers = [
            l for l in room["layers"]
            if (l["type"] == 4 and l["visible"]) or
               (l["type"] == 1 and l.get("background_visible", False))
        ]
        for layer in sorted(drawable_layers, key=lambda x: x["depth"], reverse=True):
            if layer["type"] == 1:
                sprite = sprite_resources.get(layer["sprite_index"])
                if not sprite or not sprite["frames"]:
                    layer_meta.append({
                        "name": layer["name"], "depth": layer["depth"],
                        "error": f"missing background sprite index {layer['sprite_index']}"
                    })
                    continue
                sprite_path = args.assets_root / sprite["frames"][0]["file"]
                bg = Image.open(sprite_path).convert("RGBA")
                if layer["stretch"]:
                    bg = bg.resize((room["width"], room["height"]), Image.Resampling.NEAREST)
                x0, y0 = round(layer["x_offset"]), round(layer["y_offset"])
                xs = range(x0, room["width"], bg.width) if layer["h_tiled"] else [x0]
                ys = range(y0, room["height"], bg.height) if layer["v_tiled"] else [y0]
                for yy in ys:
                    for xx in xs:
                        canvas.alpha_composite(bg, (xx, yy))
                layer_meta.append({
                    "name": layer["name"], "depth": layer["depth"],
                    "sprite": sprite["name"], "sprite_size": [bg.width, bg.height],
                    "h_tiled": layer["h_tiled"], "v_tiled": layer["v_tiled"],
                    "parallax": [layer["h_speed"], layer["v_speed"]],
                    "visible": layer["background_visible"],
                })
                continue

            tileset = tilesets.get(layer["background_index"])
            if not tileset:
                layer_meta.append({
                    "name": layer["name"], "depth": layer["depth"],
                    "error": f"missing BGND resource index {layer['background_index']}"
                })
                continue
            draw_layer(canvas, layer, tileset, image_cache)
            valid = [v for v in layer["tile_data"] if v != 0xFFFFFFFF]
            indices = [v & 0x7FFFF for v in valid]
            layer_meta.append({
                "name": layer["name"], "depth": layer["depth"],
                "background_index": layer["background_index"],
                "tileset": tileset["name"],
                "grid": [layer["tiles_x"], layer["tiles_y"]],
                "nonempty_cells": len(valid),
                "tile_index_min": min(indices) if indices else None,
                "tile_index_max": max(indices) if indices else None,
                "invalid_tile_indices": sum(i >= tileset["tile_count"] for i in indices),
                "visible": layer["visible"],
            })

        room_path = out / room_name
        room_path.mkdir(parents=True, exist_ok=True)
        png_path = room_path / "tile_layers.png"
        canvas.save(png_path)
        summary = {
            "name": room_name, "size": [room["width"], room["height"]],
            "tile_layers": layer_meta,
            "preview": str(png_path),
        }
        (room_path / "room_summary.json").write_text(
            json.dumps(summary, indent=2), encoding="utf-8"
        )
        export_layers = []
        for layer in room["layers"]:
            entry = {k: v for k, v in layer.items()
                     if k not in ("body_offset", "tile_stream_end")}
            if "tile_data" in entry:
                entry["tile_data"] = [int(v) for v in entry["tile_data"]]
            export_layers.append(entry)
        room_data = {
            "name": room["name"], "width": room["width"], "height": room["height"],
            "tile_size_source": [16, 16], "layers": export_layers,
            "instances": room["instances"],
        }
        (room_path / "room_data.json").write_text(
            json.dumps(room_data, separators=(",", ":")), encoding="utf-8"
        )
        object_counts = Counter(i["object"] for i in room["instances"])
        summary["instance_count"] = len(room["instances"])
        summary["object_counts"] = dict(object_counts)
        room_summaries.append(summary)

    (out / "rooms_manifest.json").write_text(
        json.dumps(room_summaries, indent=2), encoding="utf-8"
    )
    print(f"Rooms exported: {len(room_summaries)}")
    print(f"Output: {out / 'rooms_manifest.json'}")
    for room in room_summaries:
        print(room["name"], room["size"], "tile layers:", len(room["tile_layers"]))

if __name__ == "__main__":
    main()
