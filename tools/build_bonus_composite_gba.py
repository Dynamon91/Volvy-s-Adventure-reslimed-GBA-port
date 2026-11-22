#!/usr/bin/env python3
"""Convert the complete visible GameMaker room composite to a GBA 8bpp map.

The input is the room extractor's `tile_layers.png`, which composites all
visible background-sprite layers and visible tile layers in GameMaker depth
order. The output is a static 2:1 scene; original live parallax is not retained.
"""
from __future__ import annotations
import argparse, collections, json, re
from pathlib import Path
from PIL import Image

def ident(value):
    value=re.sub(r"\W+","_",value).strip("_")
    return value if value and not value[0].isdigit() else "scene_"+value

def bgr555(rgba):
    r,g,b,_a=rgba
    return ((r*31+127)//255) | (((g*31+127)//255)<<5) | (((b*31+127)//255)<<10)

def c_array(values,width,fmt):
    return ",\n".join(
        "    "+", ".join(fmt(v) for v in values[i:i+width])
        for i in range(0,len(values),width)
    )

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("image",type=Path)
    ap.add_argument("--name",default="bonus_w1_composite")
    ap.add_argument("--scale",type=int,default=2)
    ap.add_argument("--out",type=Path,required=True)
    ap.add_argument("--max-tiles",type=int,default=512)
    args=ap.parse_args()

    image=Image.open(args.image).convert("RGBA")
    w=max(1,(image.width+args.scale-1)//args.scale)
    h=max(1,(image.height+args.scale-1)//args.scale)
    image=image.resize((w,h),Image.Resampling.NEAREST)
    map_w=(w+7)//8
    map_h=(h+7)//8

    opaque=sorted({pixel for pixel in image.getdata() if pixel[3]>=128})
    if len(opaque)>255:
        raise ValueError(f"scene needs {len(opaque)} opaque colors; 8bpp max is 255")
    color_index={pixel:i+1 for i,pixel in enumerate(opaque)}
    palette=[0]+[bgr555(pixel) for pixel in opaque]
    palette += [0]*(256-len(palette))

    transparent=bytes(64)
    tile_lookup={transparent:0}
    tile_data=bytearray(64)
    tile_map=[]
    for ty in range(map_h):
        for tx in range(map_w):
            pixels=[]
            for py in range(8):
                for px in range(8):
                    sx,sy=tx*8+px,ty*8+py
                    if sx>=w or sy>=h:
                        pixel=(0,0,0,0)
                    else:
                        pixel=image.getpixel((sx,sy))
                    pixels.append(0 if pixel[3]<128 else color_index[pixel])
            raw=bytes(pixels)
            tile_id=tile_lookup.get(raw)
            if tile_id is None:
                tile_id=len(tile_lookup)
                tile_lookup[raw]=tile_id
                tile_data.extend(raw)
            tile_map.append(tile_id)

    tile_count=len(tile_lookup)
    if tile_count>args.max_tiles:
        raise ValueError(f"{tile_count} unique tiles exceed configured {args.max_tiles}")
    if tile_count>512:
        raise ValueError(f"{tile_count} tiles exceed the 32 KiB two-charblock 8bpp budget")

    name=ident(args.name)
    guard=name.upper()+"_H"
    out=args.out
    out.mkdir(parents=True,exist_ok=True)
    header=f"""#ifndef {guard}
#define {guard}
#include <stdint.h>
#define {name.upper()}_MAP_WIDTH {map_w}
#define {name.upper()}_MAP_HEIGHT {map_h}
#define {name.upper()}_TILE_COUNT {tile_count}
#define {name.upper()}_TILE_BYTES {len(tile_data)}
#define {name.upper()}_OPAQUE_COLORS {len(opaque)}
#define {name.upper()}_SCENE_WIDTH {w}
#define {name.upper()}_SCENE_HEIGHT {h}
extern const uint8_t {name}_tiles[{len(tile_data)}];
extern const uint16_t {name}_palette[256];
extern const uint16_t {name}_map[{len(tile_map)}];
#endif
"""
    source=(
        f'#include "{name}.h"\n'
        f'const uint8_t {name}_tiles[{len(tile_data)}] __attribute__((aligned(4))) = {{\n'
        +c_array(list(tile_data),16,lambda v:f"0x{v:02X}")+"\n};\n"
        +f'const uint16_t {name}_palette[256] __attribute__((aligned(4))) = {{\n'
        +c_array(palette,8,lambda v:f"0x{v:04X}")+"\n};\n"
        +f'const uint16_t {name}_map[{len(tile_map)}] __attribute__((aligned(4))) = {{\n'
        +c_array(tile_map,12,str)+"\n};\n"
    )
    (out/f"{name}.h").write_text(header,encoding="utf-8")
    (out/f"{name}.c").write_text(source,encoding="utf-8")
    report={
        "name":args.name,"source_image":[image.width*args.scale,image.height*args.scale],
        "gba_scene":[w,h],"map":[map_w,map_h],"unique_tiles":tile_count,
        "tile_bytes":len(tile_data),"opaque_colors":len(opaque),
        "palette_entries_including_transparent":len(opaque)+1,
        "map_entries":len(tile_map),"map_bytes":len(tile_map)*2,
        "source_includes_visible_background_sprites":True,
        "static_parallax_snapshot":True,
        "fits_two_8bpp_bg_charblocks":len(tile_data)<=32768,
    }
    (out/f"{name}_manifest.json").write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    print(json.dumps(report,indent=2))

if __name__=="__main__":
    main()
