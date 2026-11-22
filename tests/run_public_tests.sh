#!/bin/sh
# Public-repository regressions that do not require the private converted room/sprite bundle.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
CC=\${CC:-gcc}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT INT TERM
INCLUDES="-I$ROOT/tests/support -I$ROOT/include"
FLAGS="-std=gnu11 -Wall -Wextra -Werror"

$CC $FLAGS $INCLUDES "$ROOT/tests/test_fixed.c" -o "$TMP/fixed"
"$TMP/fixed"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_game_timer.c" "$ROOT/source/core/game_timer.c" -o "$TMP/game_timer"
"$TMP/game_timer"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_static_frame_scheduler.c" "$ROOT/source/core/static_frame_scheduler.c" -o "$TMP/static_frame_scheduler"
"$TMP/static_frame_scheduler"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_sprite_visibility.c" -o "$TMP/sprite_visibility"
"$TMP/sprite_visibility"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_obj_parts_v193.c" "$ROOT/source/platform/obj_parts.c" -o "$TMP/obj_parts"
"$TMP/obj_parts"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_scene_window.c" "$ROOT/source/core/scene_window.c" -o "$TMP/scene_window"
"$TMP/scene_window"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_native_scene.c" "$ROOT/source/core/native_scene.c" -o "$TMP/native_scene"
"$TMP/native_scene"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_scene_cache_churn.c" "$ROOT/source/core/native_scene.c" -o "$TMP/scene_cache_churn"
"$TMP/scene_cache_churn"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_native_presentation.c" "$ROOT/source/core/native_scene.c" -o "$TMP/native_presentation"
"$TMP/native_presentation"

# The result-screen module can be syntax-checked without the private game assets.
$CC $FLAGS $INCLUDES -fsyntax-only "$ROOT/source/platform/stage_clear_screen.c"

python3 "$ROOT/tests/test_oam_priority_v18.py"
python3 "$ROOT/tests/test_static_frame_renderer.py"
python3 "$ROOT/tests/test_static_visibility_renderer.py"
python3 "$ROOT/tests/test_scene_stream_staging.py"
python3 "$ROOT/tests/test_hud_walkthrough_icons.py"
python3 "$ROOT/tests/test_walkthrough_hud_contract.py"
python3 "$ROOT/tests/test_stage_clear_asset.py"
python3 "$ROOT/tests/test_stage_clear_screen_contract.py"
python3 -m py_compile "$ROOT"/tools/*.py

printf '%s\n' 'All public host regressions PASS. Private-asset gameplay suite and full ROM build are separate.'
