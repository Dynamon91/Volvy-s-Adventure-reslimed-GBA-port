#!/usr/bin/env python3
"""Export data.win room instances as GBA collision and interaction tables.

The 8px collision grid conservatively rasterizes scaled GameMaker sprite AABBs.
These are original placements, NOT verified original pixel collision masks.
"""
import argparse
import json
import math
import re
import struct
import sys
from pathlib import Path

OBJ_TYPES = {
    'obj_onion': 'WORLD_ONION',
    'obj_destructible_block': 'WORLD_BREAKABLE',
    'obj_action_block': 'WORLD_ACTION_BLOCK',
    'obj_squirrel': 'WORLD_SQUIRREL',
    'obj_springboard': 'WORLD_SPRINGBOARD',
    'obj_level_transition_right': 'WORLD_EXIT_RIGHT',
    'obj_level_transition_up': 'WORLD_EXIT_UP',
    'obj_level_transition_down': 'WORLD_EXIT_DOWN',
}

WORLD_OBJECT_HEADER = '''#ifndef VOLVY_WORLD_OBJECTS_H
#define VOLVY_WORLD_OBJECTS_H
#include <stdint.h>
typedef enum WorldObjectType {
    WORLD_ONION=1, WORLD_BREAKABLE, WORLD_ACTION_BLOCK, WORLD_SQUIRREL,
    WORLD_SPRINGBOARD, WORLD_EXIT_RIGHT, WORLD_EXIT_UP, WORLD_EXIT_DOWN
} WorldObjectType;
typedef struct WorldObject {
    uint16_t kind;
    int16_t x, y; /* original source instance pivot, divided by 2 */
    int16_t left, top; /* sprite AABB, divided by 2 */
    uint8_t width, height;
} WorldObject;
#endif
'''

def build(room_path: Path, data_win: Path, out: Path, scale: int = 2):
    room = json.loads(room_path.read_text(encoding='utf-8'))
    match = re.fullmatch(r'rm_(w\d+_l\d+)_\d+', room['name'])
    if not match:
        raise ValueError('Expected a numbered World level, e.g. rm_w1_l2_00')
    prefix = match.group(1)
    macro = prefix.upper()

    sys.path.insert(0, str(Path(__file__).resolve().parent/'vendor'))
    from gm_extract.gamedata import GameData
    from gm_extract.resources import parse_sprites
    game = GameData(str(data_win))
    names = game.resource_names('OBJT')
    sprites = parse_sprites(game)
    obj_off, _ = game.chunk('OBJT')
    sprite_defs = {}
    for kind in ('obj_solid', *OBJ_TYPES.keys()):
        obj_index = names.index(kind)
        obj_ptr = struct.unpack_from('<I', game.data, obj_off + 4 + 4*obj_index)[0]
        sprite_index = struct.unpack_from('<i', game.data, obj_ptr + 4)[0]
        if not 0 <= sprite_index < len(sprites):
            raise ValueError(f'No source sprite for {kind}')
        spr = sprites[sprite_index]
        sprite_defs[kind] = (spr['width'], spr['height'],
                             spr['origin_x'], spr['origin_y'])

    width = (room['width'] + 15)//16
    height = 32
    if width != 142:
        raise ValueError(f'The initial engine supports 142-tile rooms, got {width}')
    grid = bytearray(width*height)
    actors = []
    solids = []
    for inst in room['instances']:
        kind = inst['object']
        if kind not in sprite_defs:
            continue
        iw, ih, ox, oy = sprite_defs[kind]
        sx, sy = inst['scale_x'], inst['scale_y']
        x0 = (inst['x'] - ox*sx)/scale
        y0 = (inst['y'] - oy*sy)/scale
        x1 = x0 + iw*sx/scale
        y1 = y0 + ih*sy/scale
        if x1 <= x0 or y1 <= y0:
            continue
        tx0, tx1 = max(0, math.floor(x0/8)), min(width, math.ceil(x1/8))
        ty0, ty1 = max(0, math.floor(y0/8)), min(height, math.ceil(y1/8))
        if kind == 'obj_solid':
            for ty in range(ty0, ty1):
                for tx in range(tx0, tx1):
                    grid[ty*width+tx] = 1
            solids.append((x0,y0,x1,y1))
            continue
        bw, bh = max(1,round(x1-x0)), max(1,round(y1-y0))
        if bw>255 or bh>255:
            raise ValueError(f'{kind} bounds exceed uint8 dimensions')
        actors.append((OBJ_TYPES[kind], round(inst['x']/scale), round(inst['y']/scale),
                       round(x0), round(y0), bw, bh))
        if kind == 'obj_destructible_block':
            for ty in range(ty0, ty1):
                for tx in range(tx0, tx1):
                    # A solid collision surface always wins over a breakable.
                    # Room-instance enumeration order must not change collision.
                    if grid[ty*width+tx] != 1:
                        grid[ty*width+tx] = 2

    out.mkdir(parents=True, exist_ok=True)
    # Preserve the original placements of unsupported behaviors for future
    # reverse-engineering; do not silently drop them or pretend they are active.
    from collections import Counter
    unsupported = [inst for inst in room['instances']
                   if inst['object'] not in sprite_defs]
    unhandled = {
        'source_room': room['name'],
        'unimplemented_objects': len(unsupported),
        'object_types': dict(Counter(inst['object'] for inst in unsupported)),
        'placements': [
            {'name': inst['object'], 'id': inst['instance_id'],
             'x': inst['x'], 'y': inst['y'],
             'scale_x': inst['scale_x'], 'scale_y': inst['scale_y']}
            for inst in unsupported
        ],
        'note': 'Source placements preserved; no gameplay or collision logic is claimed for these objects.'
    }
    (out/f'{prefix}_unimplemented.json').write_text(
        json.dumps(unhandled, ensure_ascii=False, indent=2) + '\n',
        encoding='utf-8')
    (out/'world_objects.h').write_text(WORLD_OBJECT_HEADER,encoding='utf-8')
    header = f'''#ifndef VOLVY_{macro}_OBJECTS_H
#define VOLVY_{macro}_OBJECTS_H
#include "world_objects.h"
#define {macro}_COLLISION_WIDTH {width}
#define {macro}_COLLISION_HEIGHT {height}
#define {macro}_OBJECT_COUNT {len(actors)}
#define {macro}_ONION_COUNT {sum(a[0]=='WORLD_ONION' for a in actors)}
extern const uint8_t {prefix}_collision[{macro}_COLLISION_WIDTH * {macro}_COLLISION_HEIGHT];
extern const WorldObject {prefix}_objects[{macro}_OBJECT_COUNT];
#endif
'''
    rows = ['#include "'+prefix+'_objects.h"',
            f'const uint8_t {prefix}_collision[{macro}_COLLISION_WIDTH * {macro}_COLLISION_HEIGHT] = {{']
    for i in range(0,len(grid),32):
        rows.append('    '+', '.join(str(t) for t in grid[i:i+32])+',')
    rows += ['};', f'const WorldObject {prefix}_objects[{macro}_OBJECT_COUNT] = {{']
    rows += ['    {%s,%d,%d,%d,%d,%d,%d},' % a for a in actors]
    rows += ['};']
    (out/f'{prefix}_objects.h').write_text(header,encoding='utf-8')
    (out/f'{prefix}_objects.c').write_text('\n'.join(rows)+'\n',encoding='utf-8')
    spawn = next(([i['x'],i['y']] for i in room['instances'] if i['object']=='obj_volvy'),None)
    report = {
        'source': room['name'], 'scale': scale,
        'world_grid': [width,height],
        'solid_instances': len(solids), 'solid_cells':grid.count(1),
        'breakable_cells': grid.count(2), 'object_count': len(actors),
        'actors':{t:sum(a[0]==t for a in actors) for t in sorted(set(a[0] for a in actors))},
        'spawn_source_pivot':spawn,
        'spawn_note':f'Original sprite pivot {spawn!r}, nominal room height {room["height"]}; provisional GBA spawn is selected using collision geometry.',
        'solid_mask_note':'Sprite bounding boxes, not extracted pixel-perfect collision masks.'
    }
    (out/f'{prefix}_objects_report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report,indent=2))

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('room_data',type=Path)
    parser.add_argument('--data-win',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args()
    build(args.room_data,args.data_win,args.out)
