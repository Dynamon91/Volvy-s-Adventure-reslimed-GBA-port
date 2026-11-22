#!/usr/bin/env python3
"""Protect GBA stack: BG scene uploads must use the persistent staging buffer."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
r = (root / 'source/platform/render.c').read_text()
block = r.split('static void scene_stream_update(', 1)[1].split('static int clamp_camera(', 1)[0]
assert 'NativeSceneUpload uploads[NATIVE_SCENE_CACHE_TILES]' not in block
assert 's_scene_map, s_scene_uploads, &upload_count' in block
assert 'memcpy(s_scene_uploads, uploads,' not in block
assert 's_scene_upload_pos < s_scene_upload_count' in block
assert 's_scene_upload_count = upload_count;' in block
print('Renderer BG tile stream: PASS (persistent staging, no 4 KiB stack upload buffer)')
