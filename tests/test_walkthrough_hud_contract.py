#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
ui = (root / "source/platform/ui.c").read_text()
render = (root / "source/platform/render.c").read_text()
game = (root / "source/core/game.c").read_text()
level = (root / "include/core/level.h").read_text()

assert '#include "core/game_timer.h"' in ui
assert 'game_timer_format(timer_frames, time_text)' in ui
assert 'draw_str(1, 0, count)' in ui
assert 'draw_str(time_column, 0, time_text)' in ui
for debug_label in ('"ONIONS"', '"SHREWY"', '"HAND ATTACK"', '"RACE"', '"W1-L1"'):
    assert debug_label not in ui, f"prototype-only HUD label returned: {debug_label}"
assert 'uint32_t elapsed_frames;' in level
assert game.count('++s_level.elapsed_frames;') == 2
assert 'mode == GAME_BONUS ? level->race_elapsed_frames :' in render
assert 'level->elapsed_frames);' in render
print("Walkthrough HUD contract: PASS (onion counter, M:SS:CC timer, pause-safe stage/boss timing)")
