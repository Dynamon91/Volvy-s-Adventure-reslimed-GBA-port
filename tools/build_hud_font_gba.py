#!/usr/bin/env python3
"""Extract the exact 1:1 glyph pixels of the source GameMaker `fnt_small`.

The original font is stored in FONT+TPAG+TXTR chunks of data.win.
Export 8x8 4bpp GBA text glyphs: one source ink pixel per GBA pixel, no
resampling, thresholding or synthetic outlines. Glyphs fit in one 8x8 cell.
Output files are original-derived assets and must not be pushed publicly.
"""
from __future__ import annotations
import argparse
import json
import struct
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/vendor'))
from gm_extract.gamedata import GameData

CHARS='ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-: '

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('data_win',type=Path)
    ap.add_argument('--out',type=Path,required=True)
    ap.add_argument('--preview',type=Path)
    ap.add_argument('--font-name',default='fnt_small')
    args=ap.parse_args()
    game=GameData(str(args.data_win));data=game.data
    off,_=game.chunk('FONT');count=struct.unpack_from('<I',data,off)[0]
    found=None
    for index in range(count):
        ptr=struct.unpack_from('<I',data,off+4+index*4)[0]
        name=game.read_name(struct.unpack_from('<I',data,ptr)[0])
        if name==args.font_name: found=(index,ptr);break
    if not found:raise ValueError(f'Source font not found: {args.font_name}')
    index,ptr=found
    texture_ptr=struct.unpack_from('<I',data,ptr+28)[0]
    tpag=game.tpag(texture_ptr)
    if tpag is None:raise ValueError('Missing original TPAG record')
    page_w,page_h,rgba=game.decode_page(tpag[10])
    atlas=Image.frombytes('RGBA',(page_w,page_h),rgba)
    glyph_count=struct.unpack_from('<I',data,ptr+56)[0]
    glyphs={}
    for j in range(glyph_count):
        gp=struct.unpack_from('<I',data,ptr+60+j*4)[0]
        if gp<=0 or gp+18>len(data):raise ValueError('Invalid glyph pointer')
        code,x,y,w,h,advance,left,pad1,pad2=struct.unpack_from('<9H',data,gp)
        if code>127:continue
        glyphs[chr(code)]=(x,y,w,h,advance,left)

    source_tiles=[]
    metadata={}
    preview=Image.new('RGB',(480,120),(28,34,48));draw=ImageDraw.Draw(preview)
    font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf',10)
    # Tile 0 is transparent for empty map cells.
    source_tiles.extend([0]*32)
    for order,ch in enumerate(CHARS):
        if ch not in glyphs:raise ValueError(f'Glyph missing from original font: {ch!r}')
        x,y,w,h,advance,left=glyphs[ch]
        crop=atlas.crop((tpag[0]+x,tpag[1]+y,tpag[0]+x+w,tpag[1]+y+h))
        bbox=crop.getbbox()
        if bbox:
            cropped=crop.crop(bbox)
        else:cropped=Image.new('RGBA',(0,0))
        if cropped.width>8 or cropped.height>8:
            raise ValueError(f'Original {ch!r} ink {cropped.size} does not fit 8x8')
        bitmap=[0]*64
        cx=(8-cropped.width)//2;cy=(8-cropped.height)//2
        for py in range(cropped.height):
            for px in range(cropped.width):
                a=cropped.getpixel((px,py))[3]
                if a not in (0,255):
                    raise ValueError(f'Original source glyph {ch!r} has antialiased alpha {a}; cannot represent losslessly')
                bitmap[(cy+py)*8+(cx+px)]=1 if a==255 else 0
        for i in range(0,64,2):source_tiles.append(bitmap[i]|(bitmap[i+1]<<4))
        metadata[ch]={'glyph_size':[w,h],'opaque_bbox_size':list(cropped.size),
                      'source_tpag_offset':[x,y], 'preview_center':[cx,cy],
                      'opaque_pixels':sum(bitmap)}
        xx=(order%20)*24;yy=(order//20)*48
        draw.text((xx+3,yy),ch,font=font,fill=(240,240,245))
        large=Image.new('RGBA',(8,8),(0,0,0,0));pix=large.load()
        for row in range(8):
            for col in range(8):
                if bitmap[row*8+col]:pix[col,row]=(255,255,255,255)
        zoom=large.resize((16,16),Image.Resampling.NEAREST)
        preview.paste(zoom,(xx+4,yy+13),zoom)
    assert len(source_tiles)==40*32
    args.out.mkdir(parents=True,exist_ok=True)
    header='''#ifndef VOLVY_HUD_SOURCE_FONT_H
#define VOLVY_HUD_SOURCE_FONT_H
#include <stdint.h>
#define VOLVY_HUD_FONT_TILES 40u
#define VOLVY_HUD_FONT_BYTES (VOLVY_HUD_FONT_TILES * 32u)
extern const uint8_t g_hud_source_font_tiles[VOLVY_HUD_FONT_BYTES];
#endif
'''
    values=[f'0x{x:02X}' for x in source_tiles]
    c='''#include "hud_source_font.h"
const uint8_t g_hud_source_font_tiles[VOLVY_HUD_FONT_BYTES]
    __attribute__((aligned(4))) = {
'''
    c+='\n'.join('    '+', '.join(values[i:i+16])+',' for i in range(0,len(values),16))+'\n};\n'
    (args.out/'hud_source_font.h').write_text(header)
    (args.out/'hud_source_font.c').write_text(c)
    manifest={'source_font':args.font_name,'source_font_index':index,
              'source_file':args.data_win.name,
              'characters':CHARS,'glyph_count':len(CHARS),
              'tile_count_including_blank':40,'tile_bytes':len(source_tiles),
              'scale':1,'resampling':'none','bpp':4,
              'glyphs':metadata}
    (args.out/'hud_source_font_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    if args.preview:
        args.preview.parent.mkdir(parents=True,exist_ok=True)
        preview.save(args.preview)
    print(json.dumps({k:v for k,v in manifest.items() if k!='glyphs'},indent=2))

if __name__=='__main__':main()
