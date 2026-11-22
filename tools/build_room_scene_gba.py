#!/usr/bin/env python3
"""Convert a flattened source-room composite PNG into a GBA 8bpp tiled scene.

The input composite is downscaled 2:1 with nearest-neighbor sampling. The
result must fit one 256-tile, 16 KiB 8bpp BG character block.
"""

from __future__ import annotations
import argparse
import json
import re
from pathlib import Path
from collections import Counter
from PIL import Image

def ident(s):
    s=re.sub(r"\W+","_",s)
    return s if s and not s[0].isdigit() else "scene_"+s

def bgr555(rgba):
    r,g,b,_a=rgba
    return ((r*31+127)//255) | (((g*31+127)//255)<<5) | (((b*31+127)//255)<<10)

def arr(values,width,fmt):
    return ",\n".join(
        "    "+", ".join(fmt(v) for v in values[i:i+width])
        for i in range(0,len(values),width)
    )

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("image",type=Path)
    ap.add_argument("--name",default="rm_w1_l1_scene")
    ap.add_argument("--scale",type=int,default=2)
    ap.add_argument("--out",type=Path,required=True)
    args=ap.parse_args()

    im=Image.open(args.image).convert("RGBA")
    w=max(1,(im.width+args.scale-1)//args.scale)
    h=max(1,(im.height+args.scale-1)//args.scale)
    im=im.resize((w,h),Image.Resampling.NEAREST)

    map_w=(w+7)//8
    map_h=(h+7)//8
    palette_colors=sorted({p for p in im.getdata() if p[3]>=128})
    if len(palette_colors)>255:
        raise ValueError(f"scene needs {len(palette_colors)} opaque colors")
    color_index={p:i+1 for i,p in enumerate(palette_colors)}
    palette=[0]+[bgr555(p) for p in palette_colors]
    palette += [0]*(256-len(palette))

    tile_lookup={bytes(64):0}
    tile_data=bytearray(64)
    tile_map=[]
    for ty in range(map_h):
        for tx in range(map_w):
            indices=[]
            for py in range(8):
                for px in range(8):
                    sx,sy=tx*8+px,ty*8+py
                    if sx>=w or sy>=h:
                        pixel=(0,0,0,0)
                    else:
                        pixel=im.getpixel((sx,sy))
                    indices.append(0 if pixel[3]<128 else color_index[pixel])
            raw=bytes(indices)
            tile_id=tile_lookup.get(raw)
            if tile_id is None:
                tile_id=len(tile_lookup)
                tile_lookup[raw]=tile_id
                tile_data.extend(raw)
            tile_map.append(tile_id)

    tile_count=len(tile_lookup)
    if tile_count>512:
        raise ValueError(f"{tile_count} unique tiles exceed two 8bpp BG character blocks (32KiB)")

    name=ident(args.name)
    out=args.out
    out.mkdir(parents=True,exist_ok=True)
    header=f"""#ifndef {name.upper()}_H
#define {name.upper()}_H
#include <stdint.h>
#define {name.upper()}_MAP_WIDTH {map_w}
#define {name.upper()}_MAP_HEIGHT {map_h}
#define {name.upper()}_TILE_COUNT {tile_count}
#define {name.upper()}_PALETTE_COLORS {len(palette_colors)+1}
#define {name.upper()}_SCENE_WIDTH {w}
#define {name.upper()}_SCENE_HEIGHT {h}
extern const uint8_t {name}_tiles[];
extern const uint16_t {name}_palette[256];
extern const uint16_t {name}_map[{len(tile_map)}];
#endif
"""
    source=f"""#include <stdint.h>
#include "{name}.h"
const uint8_t {name}_tiles[] __attribute__((aligned(4))) = {{
{arr(list(tile_data),16,lambda v:f"0x{v:02X}")}
}};
const uint16_t {name}_palette[256] __attribute__((aligned(4))) = {{
{arr(palette,8,lambda v:f"0x{v:04X}")}
}};
const uint16_t {name}_map[{len(tile_map)}] = {{
{arr(tile_map,12,str)}
}};
"""
    (out/f"{name}.h").write_text(header,encoding="utf-8")
    (out/f"{name}.c").write_text(source,encoding="utf-8")
    report={
        "name":args.name,"source_image":[im.width*args.scale,im.height*args.scale],
        "gba_scene":[w,h],"map":[map_w,map_h],
        "unique_tiles":tile_count,"opaque_colors":len(palette_colors),
        "tile_bytes":len(tile_data),"map_entries":len(tile_map),
        "static_parallax_snapshot":True,
    }
    (out/f"{name}_manifest.json").write_text(json.dumps(report,indent=2),encoding="utf-8")
    print(json.dumps(report,indent=2))

if __name__=="__main__":
    main()
