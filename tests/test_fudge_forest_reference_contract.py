#!/usr/bin/env python3
"""Original-world naming corrections must not regress or break legacy assets."""
from pathlib import Path

root=Path(__file__).resolve().parents[1]
source=(root/"docs/source-audit.md").read_text()
timeline=(root/"docs/walkthrough-reference-v1_0.md").read_text()
cards=(root/"docs/handheld-selection-reconstruction-v111.md").read_text()
stage=(root/"docs/walkthrough-stage-index.csv").read_text()
game=(root/"source/core/game.c").read_text()
render=(root/"source/platform/render.c").read_text()
run_header=(root/"include/core/game.h").read_text()
assert "Matcha Meadows" in source and "Fudge Forest" in source
assert "Matcha Meadows" in timeline and "Fudge Forest" in timeline
assert "10:29" in timeline and "10:30" in timeline
assert "Matcha Meadows" in cards and "Fudge Forest" in cards
assert "Matcha Meadows" in stage and "Fudge Forest" in stage
# Historical correction notes may legitimately quote the wrong name.
# Enforce the *active source observations*, not absence of that quote.
assert "| 10:29–10:33 |" in timeline
assert "title reads **Fuzz Forest**" not in timeline
assert "Fuzz Forest" not in stage
# Old converter names must not be silently changed until the private asset
# pack is migrated together; public release builds still require no card art.
assert '#include "fuzz_forest_bin.h"' in render
assert '#include "fuzz_forest_palette.h"' in render
assert "const RunStats *game_run_stats(void)" in run_header
assert "const RunStats *game_run_stats(void)" in game
print("Fudge Forest source fidelity: PASS (original name and legacy asset compatibility)")
