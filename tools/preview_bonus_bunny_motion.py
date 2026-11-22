#!/usr/bin/env python3
"""Build a side-by-side 240x160 art preview from source PNGs and a real
host-side bunny-motion diagnostic CSV. Not a running GBA emulator capture.
"""
from __future__ import annotations
import argparse, csv, json
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('room_png', type=Path)
    p.add_argument('room_json', type=Path)
    p.add_argument('sprites_manifest', type=Path)
    p.add_argument('sprites_root', type=Path)
    p.add_argument('trace_csv', type=Path)
    p.add_argument('--out', type=Path, required=True)
    args = p.parse_args()
    manifest = json.loads(args.sprites_manifest.read_text())
    sprites = {x['name']:x for x in manifest['selected_sprites']}
    room = json.loads(args.room_json.read_text())
    records = list(csv.DictReader(args.trace_csv.open()))
    rows = {(int(x['frame']),int(x['opponent'])):x for x in records}
    raw = Image.open(args.room_png).convert('RGBA')
    scene = raw.resize((raw.width//2,raw.height//2),Image.Resampling.NEAREST)
    camera_x,camera_y = 914, 310
    font = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf',9)
    source_cache = {}

    def sprite_frame(name, frame_index=0):
        key=(name,frame_index)
        if key not in source_cache:
            s=sprites[name]; fr=s['frames'][frame_index % len(s['frames'])]
            cropped=Image.open(args.sprites_root/fr['file']).convert('RGBA')
            full=Image.new('RGBA',(s['width'],s['height']))
            full.alpha_composite(cropped,tuple(fr['destination']))
            size=(max(1,s['width']//2),max(1,s['height']//2))
            source_cache[key]=full.resize(size,Image.Resampling.NEAREST)
        return source_cache[key]

    def make_panel(t):
        panel=scene.crop((camera_x,camera_y,camera_x+240,camera_y+160))
        art={
            'obj_onion':'spr_onion',
            'obj_destructible_block':'spr_destructible_block',
            'obj_action_block':'spr_action_block',
            'obj_springboard':'spr_springboard',
            'obj_bounce_block':'spr_bumper_sq',
            'obj_corner_bumper':'spr_bumper_tri',
        }
        for instance in room['instances']:
            name=art.get(instance['object'])
            if not name: continue
            s=sprites[name]; im=sprite_frame(name,0)
            iw=max(1,round(im.width*instance['scale_x']))
            ih=max(1,round(im.height*instance['scale_y']))
            if iw!=im.width or ih!=im.height:
                im=im.resize((iw,ih),Image.Resampling.NEAREST)
            ox=round(s['origin'][0]*instance['scale_x']/2)
            oy=round(s['origin'][1]*instance['scale_y']/2)
            x=instance['x']//2-ox-camera_x
            y=instance['y']//2-oy-camera_y
            if -iw<x<240 and -ih<y<160:
                panel.alpha_composite(im,(x,y))
        for opponent in range(2):
            record=rows[(t,opponent)]
            s=sprites['spr_bunny_hop']
            im=sprite_frame('spr_bunny_hop',(t//6)%s['frame_count'])
            x=int(record['x_gba'])-round(s['origin'][0]/2)-camera_x
            y=int(record['y_gba'])-round(s['origin'][1]/2)-camera_y
            panel.alpha_composite(im,(x,y))
        d=ImageDraw.Draw(panel)
        d.rectangle((0,0,239,14),fill=(8,15,34,230))
        d.text((5,2),f'BONUS RACE  |  HOST FRAME {t:03d}',font=font,fill='white')
        d.rectangle((0,147,239,159),fill=(8,15,34,230))
        d.text((5,148),'2 active bunnies  |  9 static ground bunnies',
               font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf',7),fill='white')
        return panel

    left,right=make_panel(0),make_panel(80)
    # Keep both 240x160 outputs available individually.
    args.out.mkdir(parents=True,exist_ok=True)
    left.save(args.out/'bonus_bunnies_frame000_v13.png')
    right.save(args.out/'bonus_bunnies_frame080_v13.png')
    combined=Image.new('RGB',(980,375),(24,31,44))
    d=ImageDraw.Draw(combined)
    titlefont=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf',15)
    d.text((18,12),'Original graphics + provisional source-anchored bunny motion',font=titlefont,fill=(245,248,251))
    combined.paste(left.resize((480,320),Image.Resampling.NEAREST).convert('RGB'),(6,43))
    combined.paste(right.resize((480,320),Image.Resampling.NEAREST).convert('RGB'),(493,43))
    combined.save(args.out/'bonus_bunny_movement_comparison_v13.png')
    print('Preview generated: '+str(args.out/'bonus_bunny_movement_comparison_v13.png'))

if __name__=='__main__':
    main()
