#!/usr/bin/env python3
"""Build a merged GBA 8bpp room map from extracted GameMaker tile layers.

This handles the supplied game's observed room tilemap RLE export and merges
the visible tile layers into one map, with frontmost non-empty tiles winning.
"""

from __future__ import annotations
import argparse
import json
import re
import struct
from pathlib import Path
from PIL import Image, ImageOps

def ident(s):
    s = re.sub(r"\W+", "_", s)
    return s if s and not s[0].isdigit() else "room_" + s

def bgr555(rgba):
    r,g,b,_a = rgba
    return ((r*31+127)//255) | (((g*31+127)//255)<<5) | (((b*31+127)//255)<<10)

def c_array(values, width, fmt):
    return ",\n".join(
        "    " + ", ".join(fmt(v) for v in values[i:i+width])
        for i in range(0, len(values), width)
    )

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("room_data", type=Path)
    ap.add_argument("--asset-manifest", type=Path, required=True)
    ap.add_argument("--assets-root", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--scale", type=int, default=2)
    args=ap.parse_args()

    room=json.loads(args.room_data.read_text(encoding="utf-8"))
    manifest=json.loads(args.asset_manifest.read_text(encoding="utf-8"))
    tilesets={t["index"]:t for t in manifest["tilesets"]}
    cache={}

    def source_tile(bg_index, tile_index):
        key=(bg_index,tile_index)
        if key in cache:
            return cache[key]
        ts=tilesets[bg_index]
        image=Image.open(args.assets_root/ts["file"]).convert("RGBA")
        tw,th=ts["tile_size"]; bx,by=ts["border"]
        step_x,step_y=tw+2*bx,th+2*by
        tx,ty=tile_index%ts["columns"],tile_index//ts["columns"]
        tile=image.crop((bx+tx*step_x,by+ty*step_y,
                         bx+tx*step_x+tw,by+ty*step_y+th))
        out_w=(tw+args.scale-1)//args.scale
        out_h=(th+args.scale-1)//args.scale
        tile=tile.resize((out_w,out_h),Image.Resampling.NEAREST)
        cache[key]=tile
        return tile

    visible=[l for l in room["layers"] if l["type"]==4 and l["visible"]]
    # Source tile 0 is the checkerboard placeholder used as empty space in this game.
    used_colors=set()
    tile_cache={}
    for layer in visible:
        bg=layer["background_index"]
        ts=tilesets[bg]
        for blob in layer["tile_data"]:
            if blob==0xFFFFFFFF:
                continue
            tile_id=blob&0x7FFFF
            if tile_id==0:
                continue
            if tile_id>=ts["tile_count"]:
                raise ValueError(f"{layer['name']}: tile {tile_id} exceeds {ts['tile_count']}")
            key=(bg,tile_id,blob&0x70000000)
            if key not in tile_cache:
                tile=source_tile(bg,tile_id)
                if blob&(1<<30): tile=tile.transpose(Image.Transpose.ROTATE_270)
                if blob&(1<<28): tile=ImageOps.mirror(tile)
                if blob&(1<<29): tile=ImageOps.flip(tile)
                tile_cache[key]=list(tile.getdata())
            used_colors.update(px for px in tile_cache[key] if px[3]>=128)

    palette_colors=sorted(used_colors)
    if len(palette_colors)>255:
        raise ValueError(f"room needs {len(palette_colors)} colors; 8bpp supports 255 plus transparency")
    color_index={c:i+1 for i,c in enumerate(palette_colors)}
    palette=[0]+[bgr555(c) for c in palette_colors]
    palette += [0]*(256-len(palette))

    tile_lookup={bytes(64):0}
    tile_data=bytearray(64)  # GBA tile 0 is transparent.
    map_width=room["width"]//16
    map_height=room["height"]//16
    # The room tile layers declare their own grid; use that exact map size.
    map_width=visible[0]["tiles_x"]
    map_height=visible[0]["tiles_y"]
    full_map=[0]*(map_width*map_height)

    for layer in sorted(visible,key=lambda l:l["depth"],reverse=True):
        bg=layer["background_index"]
        ts=tilesets[bg]
        for cell,blob in enumerate(layer["tile_data"]):
            if blob==0xFFFFFFFF:
                continue
            tile_id=blob&0x7FFFF
            if tile_id==0:
                continue
            key=(bg,tile_id,blob&0x70000000)
            pixels=tile_cache[key]
            indices=bytes(0 if p[3]<128 else color_index[p] for p in pixels)
            gba_id=tile_lookup.get(indices)
            if gba_id is None:
                gba_id=len(tile_lookup)
                tile_lookup[indices]=gba_id
                tile_data.extend(indices)
            full_map[cell]=gba_id

    unique_count=len(tile_lookup)
    if unique_count>256:
        raise ValueError(f"room requires {unique_count} unique tiles; one 8bpp char block supports 256")

    out=args.out
    out.mkdir(parents=True,exist_ok=True)
    name=ident(room["name"])
    h=f"""#ifndef {name.upper()}_GBA_H
#define {name.upper()}_GBA_H
#include <stdint.h>
#define {name.upper()}_MAP_WIDTH {map_width}
#define {name.upper()}_MAP_HEIGHT {map_height}
#define {name.upper()}_TILE_COUNT {unique_count}
#define {name.upper()}_PALETTE_COLORS {len(palette_colors)+1}
#define {name.upper()}_ROOM_WIDTH_SOURCE {room['width']}
#define {name.upper()}_ROOM_HEIGHT_SOURCE {room['height']}
extern const uint8_t {name}_tiles[];
extern const uint16_t {name}_palette[256];
extern const uint16_t {name}_map[{map_width*map_height}];
#endif
"""
    c=f"""#include <stdint.h>
#include "{name}.h"

const uint8_t {name}_tiles[] __attribute__((aligned(4))) = {{
{c_array(list(tile_data),16,lambda v:f"0x{v:02X}")}
}};

const uint16_t {name}_palette[256] = {{
{c_array(palette,8,lambda v:f"0x{v:04X}")}
}};

const uint16_t {name}_map[{len(full_map)}] = {{
{c_array(full_map,12,str)}
}};
"""
    (out/f"{name}.h").write_text(h,encoding="utf-8")
    (out/f"{name}.c").write_text(c,encoding="utf-8")
    report={
        "room":room["name"],"source_size":[room["width"],room["height"]],
        "gba_tilemap_size":[map_width,map_height],
        "scale_divisor":args.scale,
        "visible_tile_layers":[{"name":l["name"],"depth":l["depth"],
                                "tileset":tilesets[l["background_index"]]["name"]}
                               for l in visible],
        "unique_tiles":unique_count,"opaque_colors":len(palette_colors),
        "tile_bytes":len(tile_data),
        "map_entries":len(full_map),
        "empty_cells":sum(v==0 for v in full_map),
        "max_tile_id":max(full_map) if full_map else 0,
    }
    (out/f"{name}_manifest.json").write_text(json.dumps(report,indent=2),encoding="utf-8")
    print(json.dumps(report,indent=2))

if __name__=="__main__":
    main()
