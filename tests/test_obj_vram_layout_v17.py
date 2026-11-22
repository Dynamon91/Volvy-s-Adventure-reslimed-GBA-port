#!/usr/bin/env python3
"""Verify the actual 8bpp OBJ tile indices used by the GBA renderer.

GBA OBJ attribute 2 uses 32-byte character units even for 64-byte 8bpp
8x8 tiles; every 8bpp sprite starts at an even tile index. The renderer's
player/boss frame DMA uses 64-byte units with optional 64-byte alignment,
while static actors use fixed even 32-byte indices.
"""
import json
import re
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/'source/platform/render.c').read_text(encoding='utf-8')
assert '#define STATIC_OBJ_BASE_32 160' in source
assert '#define BOSS_ONION_OBJ_BASE32 192u' in source
assert 'u16 base32 = BOSS_ONION_OBJ_BASE32' in source
assert '((base_tile64 + tile_index) * 2) |' in source
assert '((priority & 3u) << OBJ_PRIORITY_SHIFT)' in source

player=json.loads((ROOT/'assets/converted/player_sprites_manifest.json').read_text())
boss=json.loads((ROOT/'assets/converted/boss_sprites_manifest.json').read_text())

def frame_span_8bpp(frame):
    cursor_tiles_64=0
    for part in frame['parts']:
        assert part['tile_bytes'] % 64 == 0
        if cursor_tiles_64 & 1:
            cursor_tiles_64+=1
        # Attr2 addresses 32B units, always even for 8bpp sprites.
        assert (cursor_tiles_64*2) & 1 == 0
        cursor_tiles_64+=part['tile_bytes']//64
    return cursor_tiles_64*64

max_player=max(frame_span_8bpp(frame) for sprite in player['sprites']
               for frame in sprite['frames'])
assert max_player <= 5120, max_player
max_boss_player=max(frame_span_8bpp(frame) for sprite in boss['sprites']
               if not sprite['name'].startswith('spr_shrewy')
               for frame in sprite['frames'])
# Some non-player boss art is in this broad subset; the normal player slot
# only needs to fit the runtime's selected Volvy/Swolvy states.
selected_names={
    'spr_idle','spr_walk','spr_jump','spr_float','spr_dash','spr_dash_up',
    'spr_grind','spr_ground_pound','spr_dig','spr_punch','spr_punch_up',
    'spr_volvy_hurt','spr_swolvy_idle','spr_swolvy_walk','spr_swolvy_jump',
    'spr_swolvy_float','spr_swolvy_grind','spr_swolvy_ground_pound',
    'spr_swolvy_punch','spr_swolvy_punch_up','spr_dig_swole',
}
max_selected=max(frame_span_8bpp(frame) for sprite in boss['sprites']
                 if sprite['name'] in selected_names for frame in sprite['frames'])
assert max_selected <= 5120, max_selected

ONION_BASE32=192
BOSS_BODY_BASE32=256
ROCK_BASE32=768
assert 5120 <= ONION_BASE32*32
onion=next(s for s in boss['sprites'] if s['name']=='spr_onion')
onion_bytes=sum(frame_span_8bpp(f) for f in onion['frames'])
assert ONION_BASE32*32+onion_bytes <= BOSS_BODY_BASE32*32
boss_states=[s for s in boss['sprites'] if s['name'].startswith('spr_shrewy_')
             and s['name'] != 'spr_shrewy_rock']
max_boss_body=max(frame_span_8bpp(f) for s in boss_states for f in s['frames'])
assert BOSS_BODY_BASE32*32 + max_boss_body <= ROCK_BASE32*32
rock=next(s for s in boss['sprites'] if s['name']=='spr_shrewy_rock')
max_rock=max(frame_span_8bpp(f) for f in rock['frames'])
assert ROCK_BASE32*32+max_rock <= 32768

print('OBJ 8bpp VRAM layout: PASS')
print(f'  player aligned max {max_player} B / 5120 B; boss player {max_selected} B')
print(f'  boss onion [{ONION_BASE32*32}, {ONION_BASE32*32+onion_bytes}), '
      f'boss body [{BOSS_BODY_BASE32*32}, {BOSS_BODY_BASE32*32+max_boss_body}), '
      f'rock [{ROCK_BASE32*32}, {ROCK_BASE32*32+max_rock})')
