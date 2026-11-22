#!/usr/bin/env python3
"""Regression checks for HUD-over-gameplay OAM priority ordering."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
render = (root / "source/platform/render.c").read_text()
display = (root / "source/platform/gba_display.c").read_text()
obj_source = (root / "source/platform/obj_parts.c").read_text()

assert "#define OBJ_PRIORITY_SHIFT 10u" in render
assert "#define OBJ_GAMEPLAY_PRIORITY (1u << OBJ_PRIORITY_SHIFT)" in render
assert "gfx->part_tile_base32[part_index] | OBJ_GAMEPLAY_PRIORITY" in render
assert "((priority & 3u) << OBJ_PRIORITY_SHIFT)" in obj_source
assert "mode == GAME_TITLE ? 0u : 1u" in render
assert "boss->facing == 0, 128, 1u)" in render
assert "rock_x, rock_y, 0, 384, 1u)" in render
assert "s_boss_onion_base32[frame_index] |" in render
assert "OBJ_GAMEPLAY_PRIORITY);" in render
assert "BGCTRL[0] = BG_SIZE_0 | CHAR_BASE(0) | SCREEN_BASE(29) | 0;" in display
assert "BGCTRL[1] = BG_SIZE_1 | CHAR_BASE(0) | SCREEN_BASE(27) | 1;" in display
assert "BG0_ON | BG1_ON | OBJ_ON" in render

print("HUD/OAM priority regression checks PASS")
