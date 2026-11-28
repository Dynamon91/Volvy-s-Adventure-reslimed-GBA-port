#!/usr/bin/env python3
"""Validate the whole-game completion snapshot, docs and 100% gate."""
import copy
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import full_game_progress as tracker

raw = json.loads((ROOT / "progress/full_game_progress.json").read_text(encoding="utf-8"))
tracker.validate(raw)
pct = raw["whole_game_progress"]["completed_percent"]
rem = raw["whole_game_progress"]["remaining_percent"]
assert 0 <= pct <= 100
assert rem == 100 - pct
assert tracker.bar(0) == "░" * 25
assert tracker.bar(100) == "█" * 25
assert tracker.bar(pct).count("█") == (pct * 25 + 50) // 100
assert tracker.readme_block(raw) in (ROOT / "README.md").read_text(encoding="utf-8")
assert tracker.report(raw) == (ROOT / "docs/full-game-progress.md").read_text(encoding="utf-8")

invalid = copy.deepcopy(raw)
invalid["whole_game_progress"]["remaining_percent"] = 55
try:
    tracker.validate(invalid)
except ValueError:
    pass
else:
    raise AssertionError("Accepting incorrect remaining progress")

invalid = copy.deepcopy(raw)
invalid["whole_game_progress"]["completed_percent"] = 100
invalid["whole_game_progress"]["remaining_percent"] = 0
invalid["history"][-1]["completed_percent"] = 100
try:
    tracker.validate(invalid)
except ValueError:
    pass
else:
    raise AssertionError("Allowing full completion with unverified gates")

invalid = copy.deepcopy(raw)
invalid["whole_game_progress"]["source_scenes_implemented"] = 10
try:
    tracker.validate(invalid)
except ValueError:
    pass
else:
    raise AssertionError("Allowing invented scene count")

print("Full game progress: PASS (estimates, generated docs and 100% release gates)")
