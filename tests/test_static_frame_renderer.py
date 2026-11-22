#!/usr/bin/env python3
"""Check OAM/render integration uses the frame reserved for the next VBlank.

A standalone scheduler test cannot catch a caller accidentally drawing
`loaded_frame` (the old OBJ graphics) immediately before uploading the new
frame during VBlank.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
render = (root / 'source/platform/render.c').read_text()
assert '#include "core/static_frame_scheduler.h"' in render
assert 'static_frame_scheduler_begin(&s_static_frame_scheduler,' in render
assert 'STATIC_OBJ_UPLOAD_BUDGET);' in render
assert render.count('frame_index = request_static_frame(gfx_id, frame_index);') == 2
assert 'request_static_frame(gfx_id, frame_index);\n        frame_index = gfx->loaded_frame;' not in render
assert 's_static_gfx[a].pending = 0;' in render
assert 'gfx->loaded_frame = gfx->pending_frame;' in render
assert 'budget -= bytes;' in render
assert render.index('static_frame_scheduler_begin(&s_static_frame_scheduler,') < render.index('next_oam = draw_world_actors(')
print('Renderer static-frame schedule/OAM regression: PASS')
