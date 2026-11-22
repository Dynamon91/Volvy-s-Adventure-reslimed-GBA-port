#!/usr/bin/env python3
"""Check the first-stage result screen's renderer transition contract."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
render = (root / "source/platform/render.c").read_text()
screen = (root / "source/platform/stage_clear_screen.c").read_text()

assert '#include "platform/stage_clear_screen.h"' in render
assert 'mode == GAME_VICTORY && level->active_stage == 1u' in render
assert '!level->boss_room && !level->bonus_room' in render
assert 's_stage_clear_pending = 1;' in render
assert 's_stage_clear_restore_pending = 1;' in render
assert 'stage_clear_screen_commit();' in render
assert 'stage_clear_screen_restore();' in render
assert 'gba_display_commit_oam();' in render
assert 'LZ77UnCompWram(stage_clear_v198_bin, s_stage_clear_packed);' in screen
assert 'SetMode(MODE_4 | BG2_ON);' in screen
assert 'dmaCopy(s_stage_clear_frame, STAGE_CLEAR_FRAMEBUFFER,' in screen
assert 'SetMode(MODE_0 | BG0_ON | OBJ_ON | OBJ_1D_MAP);' in screen
print("Stage Clear renderer contract: PASS (stage-1 gate, VBlank commit, restore path)")
