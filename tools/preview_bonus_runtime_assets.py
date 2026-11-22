#!/usr/bin/env python3
"""Reconstruct a 240x160 GBA asset viewport (NOT a real emulator capture).

The image uses the actual generated bonus_w1_composite C tile/map/palette
and original sprite PNGs extracted from the user-supplied GameMaker archive.
It intentionally does not emulate graphics registers, OAM timing or gameplay.
"""
from __future__ import annotations
import argparse
import json
import re
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOM_W, ROOM_H = 2656, 560
VIEW_W, VIEW_H = 240, 160
SPRITE_BY_OBJ = {
    'obj_onion':'spr_onion',
    'obj_action_block':'spr_action_block',
    'obj_destructible_block':'spr_destructible_block',
    'obj_destructible_block_oneway':'spr_destructible_block',
    'obj_destructible_block_swolvy':'spr_destructible_block_swolvy',
    'obj_springboard':'spr_springboard',
    'obj_squirrel':'spr_squirrel_walk',
    'obj_bunny':'spr_bunny_hop',
    'obj_bunny_ground':'spr_bunny_hop',
    'obj_bounce_block':'spr_bumper_sq',
    'obj_corner_bumper':'spr_bumper_tri',
}

def get_c_array(path:Path, name:str):
    text=path.read_text()
    pat=rf'\b{name}\s*\[\s*\d+\s*\][^=]*=\s*\{{([^}}]+)\}};'
    match=re.search(pat,text,re.S)
    if not match:
        raise ValueError(f'Cannot find {name} in {path}')
    return [int(x,0) for x in re.findall(r'0x[0-9a-fA-F]+|\d+',match.group(1))]

def decode_background(root:Path):
    path=root/'assets/converted/rooms/bonus_w1_composite.c'
    tile_data=bytes(get_c_array(path,'bonus_w1_composite_tiles'))
    palette=get_c_array(path,'bonus_w1_composite_palette')
    tilemap=get_c_array(path,'bonus_w1_composite_map')
    assert len(tile_data)==31104 and len(palette)==256 and len(tilemap)==332*70
    pal=[]
    for color in palette:
        pal.extend([((color & 31)*255)//31, ((color>>5 & 31)*255)//31,
                    ((color>>10 & 31)*255)//31])
    out=Image.new('P',(332*8,70*8),0)
    out.putpalette(pal)
    cache={}
    for index,tileno in enumerate(tilemap):
        tile=cache.get(tileno)
        if tile is None:
            tile=Image.frombytes('P',(8,8),tile_data[tileno*64:(tileno+1)*64])
            tile.putpalette(pal)
            cache[tileno]=tile
        out.paste(tile,((index % 332)*8, (index // 332)*8))
    return out.convert('RGBA')

def sprite_frame(sprite, root:Path, frame_idx):
    f=sprite['frames'][frame_idx % len(sprite['frames'])]
    src=Image.open(root/f['file']).convert('RGBA')
    # Manifest PNGs are full original source sprite canvases.
    width=max(1,(src.width+1)//2)
    height=max(1,(src.height+1)//2)
    return src.resize((width,height),Image.Resampling.NEAREST)

def compose(scene:Image.Image,room,manifest,asset_root:Path, cx:int,cy:int, player=None):
    view=scene.crop((cx,cy,cx+VIEW_W,cy+VIEW_H))
    sprites={s['name']:s for s in manifest['selected_sprites']}
    total=0
    for inst in room['instances']:
        name=SPRITE_BY_OBJ.get(inst['object'])
        if not name or name not in sprites: continue
        if inst['object'] in ('obj_bunny','obj_bunny_ground'):
            frame_idx=1
        else:
            frame_idx=0
        sp=sprites[name]
        sx,sy=float(inst['scale_x']),float(inst['scale_y'])
        # Source art is already converted 2:1 above; instances mostly unit scale.
        frame=sprite_frame(sp,asset_root,frame_idx)
        w=max(1,round(frame.width * abs(sx)))
        h=max(1,round(frame.height * abs(sy)))
        x=round(float(inst['x'])/2 - sp['origin'][0]*sx/2)-cx
        y=round(float(inst['y'])/2 - sp['origin'][1]*sy/2)-cy
        if x+w<=0 or y+h<=0 or x>=VIEW_W or y>=VIEW_H: continue
        if (w,h)!=frame.size:
            frame=frame.resize((w,h),Image.Resampling.NEAREST)
        if sx<0:frame=frame.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
        if sy<0:frame=frame.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
        view.alpha_composite(frame,(x,y))
        total+=1
    if player:
        # Original source pivot, converted to 2:1 GBA world pixels.
        sp=sprites[player['sprite']]
        frame=sprite_frame(sp,asset_root,int(player['frame']))
        x=round(player['x']/2-sp['origin'][0]/2)-cx
        y=round(player['y']/2-sp['origin'][1]/2)-cy
        view.alpha_composite(frame,(x,y))
    return view,total

def label_preview(view,label,subtitle):
    # Screen interior stays native 240x160; magnification is nearest-neighbor.
    zoom=view.resize((720,480),Image.Resampling.NEAREST)
    d=ImageDraw.Draw(zoom)
    d.rectangle((0,0,719,35),fill=(7,13,34,230))
    d.text((11,9),label,fill='white')
    d.rectangle((0,442,719,479),fill=(7,13,34,230))
    d.text((11,453),subtitle,fill=(243,225,129))
    return zoom

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--project-root',type=Path,default=Path(__file__).resolve().parents[1])
    ap.add_argument('--room-json',type=Path,required=True)
    ap.add_argument('--asset-manifest',type=Path,required=True)
    ap.add_argument('--assets-root',type=Path,required=True)
    ap.add_argument('--out',type=Path,required=True)
    args=ap.parse_args()
    room=json.loads(args.room_json.read_text())
    manifest=json.loads(args.asset_manifest.read_text())
    scene=decode_background(args.project_root)
    args.out.mkdir(exist_ok=True,parents=True)
    samples=[
        ('bonus_bunny_source_v12.png', 890,285, None, 'Original bunny placement and animation frame'),
        ('bonus_track_source_v12.png', 489,193,
         {'sprite':'spr_ground_pound','frame':4,'x':1185.66,'y':546.35},
         'Mid-course: source trace frame 494, GBA-scale view'),
        ('bonus_swolvy_block_v12.png', 1100,45, None,
         'Swolvy-only source blocks / track layout'),
    ]
    for name,cx,cy,player,desc in samples:
        view,n=compose(scene,room,manifest,args.assets_root,cx,cy,player)
        labeled=label_preview(view,'VOLVY GBA - SOURCE ASSET COMPOSITION',
                              f'{desc} / visible actors: {n} / NOT EMULATOR')
        dest=args.out/name
        labeled.save(dest)
        print(f'{dest}  actors={n}')
    # Dedicated bunny frame and other source sprite reference, also pixel-perfect.
    sprites={s['name']:s for s in manifest['selected_sprites']}
    sheet=Image.new('RGBA',(600,210),(15,21,43,255))
    d=ImageDraw.Draw(sheet)
    d.text((12,10),'Original sprite conversion - 2:1 GBA scale (4x zoom)',fill=(255,255,255))
    groups=[('spr_bunny_hop',4),('spr_destructible_block_swolvy',1),('spr_bumper_sq',1),('spr_bumper_tri',1)]
    x=14
    for name,num in groups:
        sp=sprites[name]
        frame=sprite_frame(sp,args.assets_root,0)
        display=frame.resize((frame.width*4,frame.height*4),Image.Resampling.NEAREST)
        if x+display.width>sheet.width:
            break
        sheet.alpha_composite(display,(x,50))
        d.text((x,170),name.replace('spr_','')[:16],fill=(248,211,132))
        x+=max(display.width,145)+4
    sheet.save(args.out/'bonus_original_sprite_reference_v12.png')
    print(args.out/'bonus_original_sprite_reference_v12.png')

if __name__=='__main__': main()
