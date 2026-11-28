#!/usr/bin/env python3
"""Public non-proprietary checks for RAM campaign progress and results UI."""
from pathlib import Path

root=Path(__file__).resolve().parents[1]
game=(root/"source/core/game.c").read_text()
game_h=(root/"include/core/game.h").read_text()
ui=(root/"source/platform/ui.c").read_text()
campaign=(root/"source/core/campaign_progress.c").read_text()
result=(root/"source/core/result_text.c").read_text()
make=(root/"Makefile").read_text()

assert "static CampaignProgress s_campaign;" in game
init=game[game.index("void game_init(void)"):game.index("void game_update(void)")]
assert "campaign_progress_reset(&s_campaign);" in init
assert game.count("campaign_progress_record_clear(&s_campaign") == 1
update=game[game.index("void game_update(void)"):game.index("void game_render(void)")]
assert "if (s_mode == GAME_VICTORY)" in update
assert "s_level.bonus_room" in update
assert "s_level.race_elapsed_frames : s_level.elapsed_frames" in update
assert update.index("run_stats_observe(&s_run_stats") < update.index("campaign_progress_record_clear(&s_campaign")
assert "const CampaignProgress *game_campaign_progress(void)" in game
assert "const CampaignProgress *game_campaign_progress(void)" in game_h
assert "UINT32_MAX" in campaign and "has_clear" in campaign
assert "const RunStats *run = game_run_stats();" in ui
assert 'result_text_uint32(run->onions_collected, collected);' in ui
assert 'result_text_uint32(run->enemies_defeated, defeated);' in ui
assert 'result_text_heading(stage)' in ui
assert '"COLLECTED"' in ui and '"DEFEATED"' in ui
assert 'game_timer_format(timer_frames, time_text);' in ui
assert '"SHREWY DEFEATED"' in result
assert "combo" not in result.lower()
assert "SOURCES := source source/core source/platform" in make
print("Campaign/results source integration: PASS (single clear, best times, real counts, no fabricated ranks)")
