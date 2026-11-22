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

# Keep the result-screen gate ahead of scene streaming and return before the
# normal actor/background renderer can overwrite the static bitmap.
draw_start = render.index("void render_draw")
vblank_start = render.index("void render_vblank_commit")
draw = render[draw_start:vblank_start]
gate = draw.index("if (mode == GAME_VICTORY")
scene_stream = draw.index("scene_stream_update")
gate_return = draw.index("return;", gate)
assert gate < gate_return < scene_stream
assert "hide_all_objects(oam);" in draw[gate:scene_stream]

# Leaving before the first commit cancels the queued result; leaving after the
# commit restores Mode 0 before normal scene uploads resume.
vblank = render[vblank_start:]
restore = vblank.index("if (s_stage_clear_restore_pending)")
pending = vblank.index("if (s_stage_clear_pending)")
scene_load = vblank.index("if (s_stage_loaded != s_stage_requested)")
assert restore < pending < scene_load
restore_block = vblank[restore:pending]
assert "s_stage_clear_pending = 0;" in restore_block
assert "stage_clear_screen_restore();" in restore_block
assert "s_stage_loaded = 0;" in vblank
assert "reset_native_scene_cache();" in vblank
assert "s_bitmap_ui_reload_needed = 1;" in vblank
assert "gba_ui_init();" in vblank
assert "SetMode(MODE_0 | OBJ_ON | OBJ_1D_MAP);" in vblank
commit_block = vblank[pending:scene_load]
assert "stage_clear_screen_commit();" in commit_block
assert "s_stage_clear_active = 1;" in commit_block
assert "gba_display_commit_oam();" in commit_block
assert "return;" in commit_block

arm_gba = (root / "experimental/clang-gba/include/gba.h").read_text()
assert "#define MODE_4 4" in arm_gba
assert 'swi 0x11' in arm_gba
print("Stage Clear renderer contract: PASS (stage-1 gate, queued-commit cancellation, VBlank commit and restore ordering)")
