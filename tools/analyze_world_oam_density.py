#!/usr/bin/env python3
"""Estimate native-resolution OBJ pressure for source-derived World 1 rooms."""
import argparse
import json
import math
from pathlib import Path
import numpy as np

OBJECT_SPRITES = {
    "obj_onion": "spr_onion",
    "obj_action_block": "spr_action_block",
    "obj_destructible_block": "spr_destructible_block",
    "obj_springboard": "spr_springboard",
    "obj_squirrel": "spr_squirrel_walk",
    "obj_hivefly": "spr_hivefly",
    "obj_plant": "spr_plants",
    "obj_game": "spr_worm",
    "obj_level_exit_hole": "spr_level_exit_hole",
}

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("room_data",type=Path)
    ap.add_argument("sprite_manifest",type=Path)
    ap.add_argument("--out",type=Path,required=True)
    ap.add_argument("--player-oam-reserve",type=int,default=8)
    args=ap.parse_args()

    room=json.loads(args.room_data.read_text())
    manifest=json.loads(args.sprite_manifest.read_text())
    sprites={s["name"]:s for s in manifest["sprites"]}
    width,height=room["width"],room["height"]
    max_x=max(0,width-240); max_y=max(0,height-160)
    diff=np.zeros((max_y+2,max_x+2),dtype=np.int32)
    tracked=0
    part_total=0

    for inst in room["instances"]:
        sprite_name=OBJECT_SPRITES.get(inst["object"])
        if not sprite_name or sprite_name not in sprites:
            continue
        sprite=sprites[sprite_name]
        part_count=max(len(frame["parts"]) for frame in sprite["frames"])
        part_count=min(part_count,8)
        if not part_count:
            continue
        left_min=top_min=1 << 30
        right_max=bottom_max=-(1 << 30)
        for frame in sprite["frames"]:
            for part in frame["parts"]:
                left=int(round(inst["x"]-frame["origin_x"]+part["x"]))
                top=int(round(inst["y"]-frame["origin_y"]+part["y"]))
                left_min=min(left_min,left)
                top_min=min(top_min,top)
                right_max=max(right_max,left+part["hw_w"])
                bottom_max=max(bottom_max,top+part["hw_h"])
        x0=max(0,left_min-239)
        x1=min(max_x,right_max-1)
        y0=max(0,top_min-159)
        y1=min(max_y,bottom_max-1)
        if x1<x0 or y1<y0:
            continue
        value=part_count
        diff[y0,x0]+=value
        diff[y0,x1+1]-=value
        diff[y1+1,x0]-=value
        diff[y1+1,x1+1]+=value
        tracked+=1
        part_total+=part_count

    visible=np.cumsum(np.cumsum(diff,axis=0),axis=1)
    max_actor_oam=int(visible.max())
    y,x=np.unravel_index(int(visible.argmax()),visible.shape)
    report={
        "source_room":room["name"],
        "coordinate_space":"native source pixels, one source pixel per GBA pixel",
        "viewport":[240,160],
        "camera_grid_positions_evaluated":int((max_x+1)*(max_y+1)),
        "source_objects_tracked":tracked,
        "total_source_actor_parts":part_total,
        "max_visible_actor_oam":max_actor_oam,
        "player_oam_reserve":args.player_oam_reserve,
        "max_hardware_oam_estimate_including_player":max_actor_oam+args.player_oam_reserve,
        "hardware_oam_limit":128,
        "max_density_camera_origin_native_px":[int(x),int(y)],
        "fits_128_obj_entries":max_actor_oam+args.player_oam_reserve<=128,
        "note":"Conservative rectangle/part-count estimate. Does not verify per-scanline 32-OBJ limit or runtime animation timing."
    }
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report,indent=2))

if __name__=="__main__":
    main()
