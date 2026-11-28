#!/usr/bin/env python3
"""Require reliable per-run metric resets without altering stage progression."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
game=(root/"source/core/game.c").read_text()
stats=(root/"source/core/run_stats.c").read_text()
assert '#include "core/run_stats.h"' in game
assert "static RunStats s_run_stats;" in game
for start,end in [
    ("static void reset_world1","static void reset_boss_shrewy"),
    ("static void reset_boss_shrewy","static void reset_bonus_race"),
    ("static void reset_bonus_race","#if VOLVY_DEV_STAGE_SELECT"),
    ("void game_init(void)","void game_update(void)"),
]:
    section=game[game.index(start):game.index(end)]
    assert section.count("run_stats_reset(&s_run_stats") == 1, start
update=game[game.index("void game_update(void)"):game.index("void game_render(void)")]
assert "const GameMode mode_before_update = s_mode;" in update
assert "s_mode != GAME_PAUSED" in update
assert "run_stats_observe(&s_run_stats" in update
assert "mode_before_update == GAME_BONUS" in update
assert "mode_before_update == GAME_BOSS" in update
assert "mode_before_update == GAME_PLAYING" in update
assert "UINT32_MAX" in stats
assert "combo" not in stats.lower() or "not an original score" in stats
print("RunStats integration: PASS (World1, boss, bonus and init resets; pause gating)")
