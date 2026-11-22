#!/usr/bin/env python3
"""Ensure the source-only green hole marker is never submitted as a GBA OBJ."""
from pathlib import Path
import json
import re

ROOT = Path(__file__).resolve().parents[1]
render = (ROOT / "source/platform/render.c").read_text()
manifest = json.loads((ROOT / "assets/converted/player_sprites_manifest.json").read_text())

assert "STATIC_HOLE" not in render
assert "&g_spr_hole" not in render
assert "case WORLD_HOLE:" not in render
assert "case BONUS_W1_ROOM_OBJ_HOLE" not in render

sprites = {sprite["name"]: sprite for sprite in manifest["sprites"]}
hole = sprites["spr_hole"]
assert len(hole["frames"]) == 1
frame = hole["frames"][0]
assert (frame["width"], frame["height"]) == (16, 16)
assert len(frame["parts"]) == 1
array_name = frame["parts"][0]["tile_name"]
source = (ROOT / "assets/converted/player_sprites.c").read_text()
match = re.search(rf"{re.escape(array_name)}\[\][^=]*=\s*\{{(.*?)\}};", source, re.S)
assert match, "source marker tile array is missing"
values = [int(value, 0) for value in re.findall(r"0x[0-9A-Fa-f]+|\b\d+\b", match.group(1))]
assert len(values) == 256
assert len(set(values)) == 1, "spr_hole unexpectedly contains visible pixel art"
palette_index = values[0]
color = int(manifest["palette_bgr555"][palette_index])
r = (color & 31) * 255 // 31
g = ((color >> 5) & 31) * 255 // 31
b = ((color >> 10) & 31) * 255 // 31
assert g >= 220 and g >= r * 3 and g >= b * 2, (r, g, b)
print(f"Invisible hole marker check PASS (16x16 solid palette index {palette_index}, RGB {r},{g},{b}; excluded from OBJ renderer)")
