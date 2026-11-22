#!/usr/bin/env python3
"""Host-side validation of source GameMaker 4bpp HUD font tiles."""
import json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
manifest=json.loads((ROOT/'assets/converted/hud_source_font_manifest.json').read_text())
assert manifest['source_font']=='fnt_small'
assert manifest['glyph_count']==39
assert manifest['tile_count_including_blank']==40
assert manifest['scale']==1 and manifest['resampling']=='none'
assert manifest['tile_bytes']==1280
assert len(manifest['characters'])==39
src=(ROOT/'assets/converted/hud_source_font.c').read_text()
bytes_=list(map(lambda v:int(v,16),re.findall(r'0x[0-9A-Fa-f]{2}',src)))
assert len(bytes_)==40*32,len(bytes_)
assert not any(bytes_[:32])
assert set(bytes_)<=set((0,1,16,17)),set(bytes_)
assert any(bytes_[32:])
for glyph_index,ch in enumerate(manifest['characters']):
    src_tile=bytes_[(glyph_index+1)*32:(glyph_index+2)*32]
    filled=sum(((v&15)==1)+(((v>>4)&15)==1) for v in src_tile)
    assert filled==manifest['glyphs'][ch]['opaque_pixels']
    assert max(manifest['glyphs'][ch]['opaque_bbox_size'])<=8
assert manifest['glyphs']['W']['opaque_pixels']>0
print('Source HUD font tiles: PASS (39 original glyphs, 1280 bytes, native 1:1, 4bpp)')
