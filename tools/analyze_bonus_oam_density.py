#!/usr/bin/env python3
"""Evaluate worst-case OAM occupancy over all integer camera positions.

This counts the GBA OBJ rectangles that could be visible for every possible
240x160 camera origin in the 2656x560 bonus room. It uses original instance
placements and generated OBJ frame *hardware* dimensions, not opaque pixels.
Collected/destroyed actors are counted as still visible for a conservative cap.
It does not simulate mGBA's per-scanline OBJ rendering cycle limit.
"""
from __future__ import annotations
import argparse
from array import array
import json
from pathlib import Path

SOURCE_KIND_TO_SPRITE = {
    'obj_onion':'spr_onion', 'obj_action_block':'spr_action_block',
    'obj_destructible_block':'spr_destructible_block',
    'obj_destructible_block_oneway':'spr_destructible_block',
    'obj_destructible_block_swolvy':'spr_destructible_block_swolvy',
    'obj_springboard':'spr_springboard', 'obj_squirrel':'spr_squirrel_walk',
    'obj_bunny':'spr_bunny_hop', 'obj_bunny_ground':'spr_bunny_hop',
    'obj_bounce_block':'spr_bumper_sq','obj_corner_bumper':'spr_bumper_tri',
    'obj_hivefly':'spr_hivefly','obj_crowly':'spr_crowly_fly',
    'obj_vinechomp':'spr_vinechomp_base','obj_spikes_dave':'spr_spikes_dave',
    'obj_cannon':'spr_cannon','obj_button':'spr_button_off',
    'obj_rail_dave':'spr_rail_slice','obj_bkg_space_onion':'spr_bkg_space_onion',
    'obj_cloud_platform':'spr_cloud_platform',
}


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('room_json',type=Path)
    ap.add_argument('player_manifest',type=Path)
    ap.add_argument('--out',type=Path,required=True)
    args=ap.parse_args()
    room=json.loads(args.room_json.read_text())
    sprite_data=json.loads(args.player_manifest.read_text())
    sprites={sprite['name']:sprite for sprite in sprite_data['sprites']}
    camera_max_x=room['width']-240
    camera_max_y=room['height']-160
    stride=camera_max_x+2
    height=camera_max_y+2
    difference=array('i',[0])*(stride*height)
    actor_count=0
    total_parts=0
    counts={}
    for inst in room['instances']:
        obj=inst['object']
        sprite_name=SOURCE_KIND_TO_SPRITE.get(obj)
        if obj == 'obj_moving_platform':
            sprite_name = 'spr_moving_platform_wide' if inst.get('scale_x', 1.0) > 1.17 else 'spr_moving_platform'
        if not sprite_name:continue
        sprite=sprites[sprite_name]
        actor_count+=1
        part_count=max(len(frame['parts']) for frame in sprite['frames'])
        part_count=min(part_count,8)
        total_parts+=part_count
        counts[obj]=counts.get(obj,0)+1
        # Animating frames can shift bounds; take the union across all frames.
        left_min,top_min=1<<30,1<<30
        right_max,bottom_max=-(1<<30),-(1<<30)
        pivot_x=int(inst['x'])
        pivot_y=int(inst['y'])
        for frame in sprite['frames']:
            for part in frame['parts']:
                left=pivot_x-frame['origin_x']+part['x']
                top=pivot_y-frame['origin_y']+part['y']
                left_min=min(left_min,left)
                top_min=min(top_min,top)
                right_max=max(right_max,left+part['hw_w'])
                bottom_max=max(bottom_max,top+part['hw_h'])
        # Integer camera rectangle overlapping the union of sprite bounds.
        xa=max(0,left_min-239)
        xb=min(camera_max_x,right_max-1)
        ya=max(0,top_min-159)
        yb=min(camera_max_y,bottom_max-1)
        if xa>xb or ya>yb: continue
        difference[ya*stride+xa]+=part_count
        difference[ya*stride+xb+1]-=part_count
        difference[(yb+1)*stride+xa]-=part_count
        difference[(yb+1)*stride+xb+1]+=part_count

    max_actors=0
    max_camera=(0,0)
    for y in range(camera_max_y+1):
        acc=0
        for x in range(camera_max_x+1):
            idx=y*stride+x
            acc+=difference[idx]
            count=acc+(difference[idx-stride] if y else 0)
            difference[idx]=count
            if count>max_actors:
                max_actors=count
                max_camera=(x,y)
    report={
        'source_room':room['name'],'source_objects_tracked':actor_count,
        'source_object_counts':dict(sorted(counts.items())),
        'camera_grid_positions_evaluated':(camera_max_x+1)*(camera_max_y+1),
        'total_source_actor_parts':total_parts,
        'max_actor_oam_in_view':max_actors,
        'max_hardware_oam_estimate_including_8_player_parts':max_actors+8,
        'gba_oam_capacity':128,
        'max_density_camera_origin_native_px':list(max_camera),
        'note':'Exact integer-camera AABB union bound for source-derived visible actor rectangles in native 1:1 coordinates; no emulator verification or per-scanline OBJ-cycle assessment.'
    }
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))

if __name__=='__main__':main()
