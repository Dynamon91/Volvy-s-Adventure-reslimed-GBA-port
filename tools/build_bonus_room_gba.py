#!/usr/bin/env python3
"""Convert rm_w1_b1_00 source tile layers and instances into GBA data.

This is a source-derived asset exporter. It crops the original room tile grid
 to the room bounds, applies GameMaker tile transform flags offline, deduplicates
8x8 tiles, and emits an 8bpp BG map plus a compact instance table.
"""
from __future__ import annotations
import argparse
import collections
import json
import re
from pathlib import Path
from PIL import Image, ImageOps


def ident(value: str) -> str:
    value = re.sub(r"\W+", "_", value).strip("_").upper()
    if value.startswith("OBJ_"):
        value = value[4:]
    if not value or value[0].isdigit():
        value = "OBJ_" + value
    return value


def bgr555(rgba):
    r, g, b, _a = rgba
    return ((r * 31 + 127) // 255) | (((g * 31 + 127) // 255) << 5) | (((b * 31 + 127) // 255) << 10)


def c_array(values, width, fmt):
    return ",\n".join(
        "    " + ", ".join(fmt(v) for v in values[i:i + width])
        for i in range(0, len(values), width)
    )


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("room_json", type=Path)
    ap.add_argument("tileset_png", type=Path)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--name", default="bonus_w1_room")
    args = ap.parse_args()

    room = json.loads(args.room_json.read_text(encoding="utf-8"))
    tileset = Image.open(args.tileset_png).convert("RGBA")
    room_w, room_h = room["width"], room["height"]
    src_tile_w, src_tile_h = room.get("tile_size_source", [16, 16])
    if src_tile_w != 16 or src_tile_h != 16:
        raise ValueError("Expected the source room's 16x16 tile grid")
    map_w, map_h = room_w // src_tile_w, room_h // src_tile_h
    if map_w * src_tile_w != room_w or map_h * src_tile_h != room_h:
        raise ValueError("Room dimensions must be exact multiples of 16")

    # TileSet_bonus source metadata: 16x16 tiles, 2px gutters, 24 columns.
    tile_w = tile_h = 16
    border_x = border_y = 2
    columns = 24
    tile_count = 576
    step_x = tile_w + border_x * 2
    step_y = tile_h + border_y * 2

    def make_tile(blob):
        source_index = blob & 0x7FFFF
        if source_index <= 0 or source_index >= tile_count:
            return None
        tx, ty = source_index % columns, source_index // columns
        tile = tileset.crop((
            border_x + tx * step_x, border_y + ty * step_y,
            border_x + tx * step_x + tile_w,
            border_y + ty * step_y + tile_h
        )).resize((8, 8), Image.Resampling.NEAREST)
        # GameMaker tile blob flags: bit 30 rotate 90, bit 28 mirror, bit 29 flip.
        if blob & (1 << 30):
            tile = tile.transpose(Image.Transpose.ROTATE_90)
        if blob & (1 << 28):
            tile = ImageOps.mirror(tile)
        if blob & (1 << 29):
            tile = ImageOps.flip(tile)
        return tuple(tile.get_flattened_data())

    # Higher depth is behind; lower depth is in front.
    visible_layers = [
        layer for layer in room["layers"]
        if layer.get("type") == 4 and layer.get("visible")
        and layer.get("background_index") == 2
        and any(value != 0xFFFFFFFF and (value & 0x7FFFF) != 0
                for value in layer["tile_data"])
    ]
    visible_layers.sort(key=lambda layer: layer["depth"], reverse=True)
    if not visible_layers:
        raise ValueError("No visible TileSet_bonus layers found")

    cell_keys = [None] * (map_w * map_h)
    used_transform_pairs = set()
    for layer in visible_layers:
        stride = layer["tiles_x"]
        data = layer["tile_data"]
        for y in range(map_h):
            for x in range(map_w):
                blob = data[y * stride + x]
                if blob == 0xFFFFFFFF or (blob & 0x7FFFF) == 0:
                    continue
                key = (blob & 0x7FFFF, bool(blob & (1 << 30)),
                       bool(blob & (1 << 28)), bool(blob & (1 << 29)))
                tile_pixels = make_tile(blob)
                if tile_pixels is None:
                    raise ValueError(f"invalid source tile index in {layer['name']}: {blob:#x}")
                cell_keys[y * map_w + x] = (key, tile_pixels)
                used_transform_pairs.add(key)

    transparent = tuple([(0, 0, 0, 0)] * 64)
    unique_tiles = [transparent]
    tile_lookup = {transparent: 0}
    cell_map = [0] * (map_w * map_h)
    color_counts = collections.Counter()
    for cell_index, cell in enumerate(cell_keys):
        if cell is None:
            continue
        _key, pixels = cell
        tile_id = tile_lookup.get(pixels)
        if tile_id is None:
            tile_id = len(unique_tiles)
            tile_lookup[pixels] = tile_id
            unique_tiles.append(pixels)
        cell_map[cell_index] = tile_id
        color_counts.update(pixel for pixel in pixels if pixel[3] >= 128)

    if len(unique_tiles) > 1024:
        raise ValueError(f"{len(unique_tiles)} tiles exceed the GBA 10-bit BG tile index")
    colors = [color for color, _count in color_counts.most_common()]
    if len(colors) > 255:
        raise ValueError(f"{len(colors)} opaque colors exceed 8bpp palette capacity")
    color_index = {color: index + 1 for index, color in enumerate(colors)}
    palette = [0] + [bgr555(color) for color in colors]
    palette += [0] * (256 - len(palette))
    tile_bytes = []
    for pixels in unique_tiles:
        tile_bytes.extend(0 if pixel[3] < 128 else color_index[pixel] for pixel in pixels)

    # Preserve all original room placements, including decorations and helpers.
    instances = room["instances"]
    names = sorted({instance["object"] for instance in instances})
    if len(names) > 255:
        raise ValueError("Object-kind enum exceeds uint8 capacity")
    name_to_kind = {name: index for index, name in enumerate(names)}
    counts = [0] * len(names)
    records = []
    for instance in instances:
        kind = name_to_kind[instance["object"]]
        counts[kind] += 1
        sx_raw = int(round(instance["scale_x"] * 256))
        sy_raw = int(round(instance["scale_y"] * 256))
        flip_flags = (1 if sx_raw < 0 else 0) | (2 if sy_raw < 0 else 0)
        sx, sy = abs(sx_raw), abs(sy_raw)
        rot = int(round(instance["rotation"] * 10))
        x, y = int(instance["x"]), int(instance["y"])
        if not (-32768 <= x <= 32767 and -32768 <= y <= 32767):
            raise ValueError(f"instance position outside int16: {instance}")
        if not (0 <= sx <= 65535 and 0 <= sy <= 65535 and -32768 <= rot <= 32767):
            raise ValueError(f"instance transform outside compact record range: {instance}")
        object_index = int(instance["object_index"])
        if not 0 <= object_index <= 255:
            raise ValueError(f"object index outside uint8: {instance}")
        records.append((int(instance["instance_id"]), x, y, sx, sy,
                        rot, object_index, kind, flip_flags, 0))

    name = re.sub(r"\W+", "_", args.name)
    guard = name.upper() + "_H"
    ctype = name.title().replace("_", "")
    out = args.out
    out.mkdir(parents=True, exist_ok=True)

    header = f"""#ifndef {guard}
#define {guard}
#include <stdint.h>
#define {name.upper()}_SOURCE_WIDTH {room_w}
#define {name.upper()}_SOURCE_HEIGHT {room_h}
#define {name.upper()}_MAP_WIDTH {map_w}
#define {name.upper()}_MAP_HEIGHT {map_h}
#define {name.upper()}_TILE_COUNT {len(unique_tiles)}
#define {name.upper()}_TILE_BYTES {len(tile_bytes)}
#define {name.upper()}_PALETTE_COLORS {len(colors) + 1}
#define {name.upper()}_INSTANCE_COUNT {len(records)}
#define {name.upper()}_OBJECT_KIND_COUNT {len(names)}
typedef enum {ctype}ObjectKind {{
"""
    for index, object_name in enumerate(names):
        header += f"    {name.upper()}_OBJ_{ident(object_name)} = {index},\n"
    header += f"""}} {ctype}ObjectKind;
typedef struct {ctype}Instance {{
    uint32_t source_id;
    int16_t x, y;
    uint16_t scale_x_q8, scale_y_q8;
    int16_t rotation_tenths;
    uint8_t object_index, kind, flip_flags, reserved;
}} {ctype}Instance;
extern const uint8_t {name}_tiles[{len(tile_bytes)}];
extern const uint16_t {name}_palette[256];
extern const uint16_t {name}_map[{len(cell_map)}];
extern const {ctype}Instance {name}_instances[{len(records)}];
extern const uint16_t {name}_object_counts[{len(names)}];
#endif
"""
    source = [f'#include "{name}.h"', ""]
    source += [
        f"const uint8_t {name}_tiles[{len(tile_bytes)}] __attribute__((aligned(4))) = {{",
        c_array(tile_bytes, 16, lambda value: f"0x{value:02X}"),
        "};", "",
        f"const uint16_t {name}_palette[256] __attribute__((aligned(4))) = {{",
        c_array(palette, 8, lambda value: f"0x{value:04X}"),
        "};", "",
        f"const uint16_t {name}_map[{len(cell_map)}] __attribute__((aligned(4))) = {{",
        c_array(cell_map, 12, str),
        "};", "",
        f"const {ctype}Instance {name}_instances[{len(records)}] = {{"
    ]
    for record in records:
        source.append("    {" + ", ".join(str(value) for value in record) + "},")
    source += [
        "};", "",
        f"const uint16_t {name}_object_counts[{len(counts)}] = {{",
        c_array(counts, 12, str),
        "};", ""
    ]
    (out / f"{name}.h").write_text(header, encoding="utf-8")
    (out / f"{name}.c").write_text("\n".join(source), encoding="utf-8")

    object_counts = {object_name: counts[index] for index, object_name in enumerate(names)}
    bg_layers = []
    for layer in room["layers"]:
        if layer.get("type") == 1 and layer.get("visible") and layer.get("background_visible"):
            bg_layers.append({
                "name": layer["name"], "sprite_index": layer["sprite_index"],
                "x_offset": layer["x_offset"], "y_offset": layer["y_offset"],
                "parallax_x": layer["h_speed"], "parallax_y": layer["v_speed"],
                "h_tiled": layer["h_tiled"], "v_tiled": layer["v_tiled"]
            })
    manifest = {
        "room": room["name"], "source_size": [room_w, room_h],
        "gba_size": [room_w // 2, room_h // 2],
        "map_size": [map_w, map_h],
        "visible_tile_layers": [layer["name"] for layer in visible_layers],
        "used_source_tile_transforms": len(used_transform_pairs),
        "unique_gba_tiles": len(unique_tiles),
        "tile_bytes": len(tile_bytes), "palette_opaque_colors": len(colors),
        "palette_entries_including_transparent": len(colors) + 1,
        "map_entries": len(cell_map),
        "instance_count": len(records), "object_kind_count": len(names),
        "object_counts": object_counts, "background_layers": bg_layers,
        "source_tile_index_zero_is_placeholder": True,
        "transforms_baked_into_tile_art": True,
        "notes": [
            "Room tile grid is cropped to the 5312x1120 room bounds.",
            "Tiles_1 is composited behind TilesFront; empty front cells preserve the underlying tile.",
            "GameMaker rotate/mirror/flip flags are baked into deduplicated GBA tile art.",
            "All source instances are preserved as compact C records; extraction does not imply behavior."
        ]
    }
    (out / f"{name}_manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({
        "room": room["name"], "map": [map_w, map_h],
        "unique_tiles": len(unique_tiles), "tile_bytes": len(tile_bytes),
        "opaque_colors": len(colors), "instances": len(records),
        "object_kinds": len(names), "output": str(out)
    }, indent=2))


if __name__ == "__main__":
    main()
