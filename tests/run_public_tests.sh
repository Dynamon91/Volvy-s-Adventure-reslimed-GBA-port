#!/bin/sh
# Public-repository regressions that do not require the private converted room/sprite bundle.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
CC=gcc
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT INT TERM
INCLUDES="-I$ROOT/tests/support -I$ROOT/include"
FLAGS="-std=gnu11 -Wall -Wextra -Werror"

$CC $FLAGS $INCLUDES "$ROOT/tests/test_fixed.c" -o "$TMP/fixed"
"$TMP/fixed"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_game_timer.c" "$ROOT/source/core/game_timer.c" -o "$TMP/game_timer"
"$TMP/game_timer"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_title_menu.c" "$ROOT/source/core/title_menu.c" -o "$TMP/title_menu"
"$TMP/title_menu"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_static_frame_scheduler.c" "$ROOT/source/core/static_frame_scheduler.c" -o "$TMP/static_frame_scheduler"
"$TMP/static_frame_scheduler"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_sprite_visibility.c" -o "$TMP/sprite_visibility"
"$TMP/sprite_visibility"

# Supply only the generated sprite type declarations; no private sprite data
# or generated animation tables are needed by this OBJ-parts unit test.
$CC $FLAGS -I"$ROOT/tests/support/obj_parts_fake" $INCLUDES \
  "$ROOT/tests/test_obj_parts_v193.c" "$ROOT/source/platform/obj_parts.c" -o "$TMP/obj_parts"
"$TMP/obj_parts"

$CC $FLAGS $INCLUDES "$ROOT/tests/test_scene_window.c" "$ROOT/source/core/scene_window.c" -o "$TMP/scene_window"
"$TMP/scene_window"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_native_scene.c" "$ROOT/source/core/native_scene.c" -o "$TMP/native_scene"
"$TMP/native_scene"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_scene_cache_churn.c" "$ROOT/source/core/native_scene.c" -o "$TMP/scene_cache_churn"
"$TMP/scene_cache_churn"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_native_presentation.c" "$ROOT/source/core/native_scene.c" -o "$TMP/native_presentation"
"$TMP/native_presentation"

# Exercise the Stage Clear C module against host-side GBA memory/DMA stubs.
$CC $FLAGS -I"$ROOT/tests/support/stage_clear_fake" $INCLUDES \
  "$ROOT/tests/test_stage_clear_screen_runtime.c" \
  "$ROOT/source/platform/stage_clear_screen.c" -o "$TMP/stage_clear_runtime"
"$TMP/stage_clear_runtime"

# Exercise 256-color card playback using deterministic public synthetic bytes.
$CC $FLAGS -I"$ROOT/tests/support/stage_clear_fake" $INCLUDES \
  "$ROOT/tests/test_handheld_card_screen_runtime.c" \
  "$ROOT/source/platform/handheld_card_screen.c" -o "$TMP/card_screen_runtime"
"$TMP/card_screen_runtime"

# World interaction logic uses a synthetic public object set, not private level assets.
$CC $FLAGS -I"$ROOT/tests/support/world_interactions_fake" $INCLUDES \
  "$ROOT/tests/test_world_interactions_public.c" \
  "$ROOT/source/core/world_interactions.c" -o "$TMP/world_interactions_public"
"$TMP/world_interactions_public"

# The result-screen module can be syntax-checked without the private game assets.
$CC $FLAGS $INCLUDES -fsyntax-only "$ROOT/source/platform/stage_clear_screen.c"

python3 "$ROOT/tests/test_oam_priority_v18.py"
python3 "$ROOT/tests/test_static_frame_renderer.py"
python3 "$ROOT/tests/test_static_visibility_renderer.py"
python3 "$ROOT/tests/test_scene_stream_staging.py"
python3 "$ROOT/tests/test_hud_walkthrough_icons.py"
python3 "$ROOT/tests/test_walkthrough_hud_contract.py"
python3 "$ROOT/tests/test_handheld_select_asset.py"
python3 "$ROOT/tests/test_handheld_card_integration.py"
python3 "$ROOT/tests/test_stage_clear_asset.py"
python3 "$ROOT/tests/test_stage_clear_screen_contract.py"
python3 "$ROOT/tests/test_release_flags.py"
python3 -m py_compile "$ROOT"/tools/*.py

printf '%s\n' 'All public host regressions PASS. Private-asset gameplay suite and full ROM build are separate.'
