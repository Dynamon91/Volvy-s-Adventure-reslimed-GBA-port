#!/usr/bin/env python3
"""Dev-only handheld card preview and native BG restoration source contract."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
game = (root / "source/core/game.c").read_text()
render = (root / "source/platform/render.c").read_text()
make = (root / "Makefile").read_text()
screen = (root / "source/platform/handheld_card_screen.c").read_text()

assert "DEV_CARD_PREVIEW ?= 0" in make
assert "-DVOLVY_DEV_CARD_PREVIEW=$(DEV_CARD_PREVIEW)" in make
assert "#if VOLVY_DEV_CARD_PREVIEW" in game
assert "if (input_was_pressed(KEY_L))" in game
assert "s_mode = GAME_CARD_PREVIEW;" in game
assert "case GAME_CARD_PREVIEW:" in game
assert "input_was_pressed(KEY_B)" in game
assert '#include "fuzz_forest_bin.h"' in render
assert 's_fuzz_card.lz77_bytes = fuzz_forest_bin_size;' in render
assert 'handheld_card_screen_prepare(&s_fuzz_card)' in render
assert 'handheld_card_screen_commit();' in render
assert 'handheld_card_screen_restore();' in render
assert 's_card_restore_pending = 1;' in render
assert 's_bitmap_ui_reload_needed = 1;' in render
assert 's_stage_loaded = 0;' in render
assert 'reset_native_scene_cache();' in render
assert 'gba_ui_init();' in render
assert 'CARD_CONTENT_BYTES' in screen
assert 's_card_frame + CARD_WIDTH * CARD_TOP_MARGIN' in screen
print("Handheld card integration contract: PASS (dev-gated, private asset, VBlank, native BG rebuild)")
