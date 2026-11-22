#!/usr/bin/env python3
"""Export 2:1 GameMaker bumper AABBs, including original per-instance
scaling and rotation. These are conservative bounds, not mask collisions.
"""
from __future__ import annotations
import argparse, json, math
from pathlib import Path

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('room_data',type=Path)
    ap.add_argument('--out',type=Path,required=True)
    args=ap.parse_args()
    room=json.loads(args.room_data.read_text())
    lines=[]; report=[]
    for idx,instance in enumerate(room['instances']):
        if instance['object'] not in ('obj_bounce_block','obj_corner_bumper'):
            continue
        radians=math.radians(instance['rotation'])
        angle_sin,angle_cos=math.sin(radians),math.cos(radians)
        x0,y0=instance['x'],instance['y']
        sx,sy=instance['scale_x'],instance['scale_y']
        points=[]
        for px,py in ((-8,-8),(8,-8),(8,8),(-8,8)):
            local_x,local_y=px*sx,py*sy
            points.append(((x0+local_x*angle_cos-local_y*angle_sin)/2,
                           (y0+local_x*angle_sin+local_y*angle_cos)/2))
        left=math.floor(min(x for x,y in points))
        top=math.floor(min(y for x,y in points))
        right=math.ceil(max(x for x,y in points))
        bottom=math.ceil(max(y for x,y in points))
        kind=1 if instance['object']=='obj_corner_bumper' else 0
        lines.append((idx,left,top,right-left,bottom-top,kind))
        report.append({'source_index':idx,'instance_id':instance['instance_id'],
                       'object':instance['object'], 'original_rotation_degrees':instance['rotation'],
                       'gba_aabb':[left,top,right-left,bottom-top]})
    assert len(lines)==10, len(lines)
    args.out.mkdir(parents=True,exist_ok=True)
    (args.out/'bonus_w1_bumper_bounds.h').write_text('''#ifndef BONUS_W1_BUMPER_BOUNDS_H
#define BONUS_W1_BUMPER_BOUNDS_H
#include <stdint.h>
#define BONUS_W1_BUMPER_COUNT 10
/* Offline AABBs from original 16x16 sprites, origins, instance transforms. */
typedef struct BonusBumperHitbox {
    uint16_t instance_index;
    int16_t x, y;
    uint8_t width, height, corner;
} BonusBumperHitbox;
extern const BonusBumperHitbox bonus_w1_bumpers[BONUS_W1_BUMPER_COUNT];
#endif
''')
    c=['#include "bonus_w1_bumper_bounds.h"',
       'const BonusBumperHitbox bonus_w1_bumpers[BONUS_W1_BUMPER_COUNT] = {']
    c += ['    {'+', '.join(map(str,row))+'},' for row in lines]
    c.append('};')
    (args.out/'bonus_w1_bumper_bounds.c').write_text('\n'.join(c)+'\n')
    (args.out/'bonus_w1_bumper_bounds_report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'Generated {len(lines)} source-rotated bumper AABBs')

if __name__=='__main__': main()
