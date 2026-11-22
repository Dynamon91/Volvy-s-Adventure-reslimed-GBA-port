#!/usr/bin/env python3
"""Guard release builds against shipping the walkthrough/debug stage selector."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
game = (root / "source/core/game.c").read_text()
makefile = (root / "Makefile").read_text()

assert "#ifndef VOLVY_DEV_STAGE_SELECT\n#define VOLVY_DEV_STAGE_SELECT 0\n#endif" in game
assert "DEV_STAGE_SELECT ?= 0" in makefile
assert "-DVOLVY_DEV_STAGE_SELECT=$(DEV_STAGE_SELECT)" in makefile

helper_start = game.index("static uint8_t next_title_stage")
helper_guard = game.rfind("#if VOLVY_DEV_STAGE_SELECT", 0, helper_start)
helper_end = game.index("#endif", helper_start)
assert helper_guard >= 0 and helper_end > helper_start

selector = game.index("if (input_was_pressed(KEY_SELECT))")
selector_guard = game.rfind("#if VOLVY_DEV_STAGE_SELECT", 0, selector)
selector_end = game.index("#endif", selector)
assert selector_guard >= 0 and selector_end > selector

print("Release input contract: PASS (debug stage cycling disabled by default; explicit DEV_STAGE_SELECT=1 opt-in)")
