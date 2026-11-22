#!/usr/bin/env python3
"""Private-source preview of the compiled C prototype's moving platforms.

Reads positions from trace_bonus_platforms.c CSV; does not generate gameplay
poses from an invented route or claim to be an emulator screenshot.
"""
from pathlib import Path
import argparse, csv, json
from PIL import Image, ImageDraw, ImageFont


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('scene_png',type=Path)
    parser.add_argument('sprite_manifest',type=Path)
    parser.add_argument('assets_root',type=Path)
    parser.add_argument('trace_csv',type=Path)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args()
    args.out.mkdir(parents=True,exist_ok=True)
    manifest=json.loads(args.sprite_manifest.read_text())
    sprites={s['name']:s for s in manifest['selected_sprites']}
    frames={}
    for name in ['spr_moving_platform','spr_moving_platform_wide']:
        sp=sprites[name]
        frame=sp['frames'][0]
        src=Image.open(args.assets_root/frame['file']).convert('RGBA')
        canvas=Image.new('RGBA',(sp['width'],sp['height']),(0,0,0,0))
        canvas.alpha_composite(src,tuple(frame['destination']))
        frames[name]=canvas.resize((sp['width']//2,sp['height']//2),Image.Resampling.NEAREST)
    raw_scene=Image.open(args.scene_png).convert('RGBA')
    scene=raw_scene.resize((raw_scene.width//2,raw_scene.height//2),Image.Resampling.NEAREST)
    with args.trace_csv.open() as f: positions=list(csv.DictReader(f))
    camera_x,camera_y=1168,155
    panels=[]
    font=ImageFont.load_default()
    for frame_id in (0,48):
        view=scene.crop((camera_x,camera_y,camera_x+240,camera_y+160))
        one=next(r for r in positions if int(r['frame'])==frame_id and int(r['platform'])==0)
        w=int(one['width']);h=int(one['height']);x=round(float(one['x_pivot']))-w//2-camera_x
        y=round(float(one['y_pivot']))-h-camera_y
        img=frames['spr_moving_platform_wide']
        assert img.size==(32,4),(img.size,w,h)
        view.alpha_composite(img,(x,y))
        draw=ImageDraw.Draw(view)
        draw.rectangle((0,0,239,14),fill=(8,13,33,228))
        draw.text((5,2),f'MOVING PLATFORM | FRAME {frame_id}',font=font,fill=(255,255,255,255))
        draw.rectangle((0,145,239,159),fill=(8,13,33,228))
        draw.text((5,147),f'X:{float(one["x_pivot"]):.1f}  Y:{float(one["y_pivot"]):.1f}',font=font,fill=(255,235,140,255))
        p=args.out/f'bonus_platform_frame_{frame_id}_v14.png'
        view.save(p)
        panels.append(view)
    composite=Image.new('RGBA',(480,160),(0,0,0,255))
    composite.alpha_composite(panels[0],(0,0))
    composite.alpha_composite(panels[1],(240,0))
    composite.save(args.out/'bonus_platform_motion_comparison_v14.png')
    print('Saved 240x160 camera samples and 480x160 comparison using compiled C trace')

if __name__=='__main__':main()
