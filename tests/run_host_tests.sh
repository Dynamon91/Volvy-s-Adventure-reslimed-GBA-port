#!/bin/sh
# Host-only logic/unit/syntax tests. NOT a devkitARM or mGBA verification.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
CC=${CC:-gcc}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT INT TERM
INCLUDES="-I$ROOT/tests/support -I$ROOT/include -I$ROOT/assets/converted -I$ROOT/assets/converted/rooms -I$ROOT/assets/converted/tilesets"
FLAGS="-std=gnu11 -Wall -Wextra -Werror"

# Intentional shell word splitting for compiler option lists.
# shellcheck disable=SC2086
$CC $FLAGS $INCLUDES "$ROOT/tests/test_fixed.c" -o "$TMP/fixed"
"$TMP/fixed"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_game_timer.c" "$ROOT/source/core/game_timer.c" -o "$TMP/game_timer"
"$TMP/game_timer"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_static_frame_scheduler.c" \
    "$ROOT/source/core/static_frame_scheduler.c" -o "$TMP/static_frame_scheduler"
"$TMP/static_frame_scheduler"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_sprite_visibility.c" -o "$TMP/sprite_visibility"
"$TMP/sprite_visibility"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_obj_parts_v193.c" \
    "$ROOT/source/platform/obj_parts.c" -o "$TMP/obj_parts_v193"
"$TMP/obj_parts_v193"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_player.c" "$ROOT/source/core/player.c" "$ROOT/source/core/level.c" "$ROOT/assets/converted/rooms/bonus_w1_room.c" "$ROOT/assets/converted/rooms/bonus_w1_collision.c" "$ROOT/assets/converted/rooms/w1_l1_objects.c" "$ROOT/assets/converted/rooms/w1_l2_objects.c" "$ROOT/assets/converted/rooms/w1_l3_objects.c" "$ROOT/assets/converted/rooms/shrewy_room.c" -o "$TMP/player"
"$TMP/player"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_collision_sweep_v195.c" \
    "$ROOT/source/core/player.c" "$ROOT/source/core/level.c" \
    "$ROOT/assets/converted/rooms/bonus_w1_room.c" \
    "$ROOT/assets/converted/rooms/bonus_w1_collision.c" \
    "$ROOT/assets/converted/rooms/w1_l1_objects.c" \
    "$ROOT/assets/converted/rooms/w1_l2_objects.c" \
    "$ROOT/assets/converted/rooms/w1_l3_objects.c" \
    "$ROOT/assets/converted/rooms/shrewy_room.c" -o "$TMP/collision_sweep_v195"
"$TMP/collision_sweep_v195"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_collision_sweep_vertical_v195.c" \
    "$ROOT/source/core/level.c" "$ROOT/assets/converted/rooms/bonus_w1_room.c" \
    "$ROOT/assets/converted/rooms/bonus_w1_collision.c" \
    "$ROOT/assets/converted/rooms/w1_l1_objects.c" \
    "$ROOT/assets/converted/rooms/w1_l2_objects.c" \
    "$ROOT/assets/converted/rooms/w1_l3_objects.c" \
    "$ROOT/assets/converted/rooms/shrewy_room.c" -o "$TMP/collision_sweep_vertical_v195"
"$TMP/collision_sweep_vertical_v195"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_world1.c" "$ROOT/source/core/level.c" "$ROOT/assets/converted/rooms/bonus_w1_room.c" "$ROOT/assets/converted/rooms/bonus_w1_collision.c" "$ROOT/assets/converted/rooms/w1_l1_objects.c" "$ROOT/assets/converted/rooms/w1_l2_objects.c" "$ROOT/assets/converted/rooms/w1_l3_objects.c" "$ROOT/assets/converted/rooms/shrewy_room.c" -o "$TMP/world1"
"$TMP/world1"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_gameplay.c" "$ROOT/source/core/player.c" "$ROOT/source/core/level.c" "$ROOT/assets/converted/rooms/bonus_w1_room.c" "$ROOT/assets/converted/rooms/bonus_w1_collision.c" "$ROOT/source/core/camera.c" "$ROOT/assets/converted/rooms/w1_l1_objects.c" "$ROOT/assets/converted/rooms/w1_l2_objects.c" "$ROOT/assets/converted/rooms/w1_l3_objects.c" "$ROOT/assets/converted/rooms/shrewy_room.c" -o "$TMP/gameplay"
"$TMP/gameplay"

$CC $FLAGS $INCLUDES "$ROOT/tests/test_world_interactions.c" "$ROOT/source/core/player.c" "$ROOT/source/core/level.c" "$ROOT/assets/converted/rooms/bonus_w1_room.c" "$ROOT/assets/converted/rooms/bonus_w1_collision.c" "$ROOT/source/core/world_interactions.c" "$ROOT/assets/converted/rooms/w1_l1_objects.c" "$ROOT/assets/converted/rooms/w1_l2_objects.c" "$ROOT/assets/converted/rooms/w1_l3_objects.c" "$ROOT/assets/converted/rooms/shrewy_room.c" -o "$TMP/world_interactions"
"$TMP/world_interactions"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_world2.c" "$ROOT/source/core/level.c" "$ROOT/assets/converted/rooms/bonus_w1_room.c" "$ROOT/assets/converted/rooms/bonus_w1_collision.c" "$ROOT/source/core/player.c" "$ROOT/source/core/world_interactions.c" "$ROOT/assets/converted/rooms/w1_l1_objects.c" "$ROOT/assets/converted/rooms/w1_l2_objects.c" "$ROOT/assets/converted/rooms/w1_l3_objects.c" "$ROOT/assets/converted/rooms/shrewy_room.c" -o "$TMP/world2"
"$TMP/world2"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_world3.c" "$ROOT/source/core/level.c" "$ROOT/assets/converted/rooms/bonus_w1_room.c" "$ROOT/assets/converted/rooms/bonus_w1_collision.c" "$ROOT/source/core/player.c" "$ROOT/source/core/world_interactions.c" "$ROOT/assets/converted/rooms/w1_l1_objects.c" "$ROOT/assets/converted/rooms/w1_l2_objects.c" "$ROOT/assets/converted/rooms/w1_l3_objects.c" "$ROOT/assets/converted/rooms/shrewy_room.c" -o "$TMP/world3"
"$TMP/world3"

$CC $FLAGS $INCLUDES "$ROOT/tests/test_boss_shrewy.c" "$ROOT/source/core/boss_shrewy.c" "$ROOT/source/core/player.c" "$ROOT/source/core/level.c" "$ROOT/assets/converted/rooms/bonus_w1_room.c" "$ROOT/assets/converted/rooms/bonus_w1_collision.c" "$ROOT/assets/converted/rooms/w1_l1_objects.c" "$ROOT/assets/converted/rooms/w1_l2_objects.c" "$ROOT/assets/converted/rooms/w1_l3_objects.c" "$ROOT/assets/converted/rooms/shrewy_room.c" -o "$TMP/boss_shrewy"
"$TMP/boss_shrewy"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_cage_sequence.c" "$ROOT/source/core/cage_sequence.c" "$ROOT/assets/converted/rooms/shrewy_cage_sequence.c" -o "$TMP/cage_sequence"
"$TMP/cage_sequence"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_bonus_room.c" "$ROOT/assets/converted/rooms/bonus_w1_room.c" -o "$TMP/bonus_room"
"$TMP/bonus_room"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_bonus_race.c" "$ROOT/source/core/bonus_race.c" "$ROOT/source/core/bonus_bunnies.c" "$ROOT/source/core/bonus_platforms.c" "$ROOT/assets/converted/rooms/bonus_w1_bumper_bounds.c" "$ROOT/source/core/level.c" "$ROOT/source/core/player.c" "$ROOT/assets/converted/rooms/bonus_w1_room.c" "$ROOT/assets/converted/rooms/bonus_w1_collision.c" "$ROOT/assets/converted/rooms/w1_l1_objects.c" "$ROOT/assets/converted/rooms/w1_l2_objects.c" "$ROOT/assets/converted/rooms/w1_l3_objects.c" "$ROOT/assets/converted/rooms/shrewy_room.c" -o "$TMP/bonus_race"
"$TMP/bonus_race"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_bonus_bunnies.c" "$ROOT/source/core/bonus_bunnies.c" "$ROOT/source/core/bonus_platforms.c" "$ROOT/assets/converted/rooms/bonus_w1_bumper_bounds.c" "$ROOT/source/core/level.c" "$ROOT/assets/converted/rooms/bonus_w1_room.c" "$ROOT/assets/converted/rooms/bonus_w1_collision.c" "$ROOT/assets/converted/rooms/w1_l1_objects.c" "$ROOT/assets/converted/rooms/w1_l2_objects.c" "$ROOT/assets/converted/rooms/w1_l3_objects.c" "$ROOT/assets/converted/rooms/shrewy_room.c" -o "$TMP/bonus_bunnies"
"$TMP/bonus_bunnies"

$CC $FLAGS $INCLUDES "$ROOT/tests/test_bonus_platforms.c" "$ROOT/source/core/bonus_platforms.c" "$ROOT/source/core/player.c" "$ROOT/source/core/level.c" "$ROOT/assets/converted/rooms/bonus_w1_room.c" "$ROOT/assets/converted/rooms/bonus_w1_collision.c" "$ROOT/assets/converted/rooms/w1_l1_objects.c" "$ROOT/assets/converted/rooms/w1_l2_objects.c" "$ROOT/assets/converted/rooms/w1_l3_objects.c" "$ROOT/assets/converted/rooms/shrewy_room.c" -o "$TMP/bonus_platforms"
"$TMP/bonus_platforms"

$CC $FLAGS $INCLUDES "$ROOT/tests/test_scene_window.c" "$ROOT/source/core/scene_window.c" -o "$TMP/scene_window"
"$TMP/scene_window"

$CC $FLAGS $INCLUDES "$ROOT/tests/test_native_scene.c" "$ROOT/source/core/native_scene.c" -o "$TMP/native_scene"
"$TMP/native_scene"
$CC $FLAGS $INCLUDES "$ROOT/tests/test_scene_cache_churn.c" \
    "$ROOT/source/core/native_scene.c" -o "$TMP/scene_cache_churn"
"$TMP/scene_cache_churn"

$CC $FLAGS $INCLUDES "$ROOT/tests/test_native_presentation.c" "$ROOT/source/core/native_scene.c" -o "$TMP/native_presentation"
"$TMP/native_presentation"

$CC $FLAGS $INCLUDES "$ROOT/tests/test_native_scene_assets.c" "$ROOT/source/core/native_scene.c" \
    "$ROOT/assets/converted/rooms/w1_l1_native.c" \
    "$ROOT/assets/converted/rooms/w1_l2_native.c" \
    "$ROOT/assets/converted/rooms/w1_l3_native.c" \
    "$ROOT/assets/converted/rooms/bonus_native.c" \
    "$ROOT/assets/converted/rooms/shrewy_native.c" -o "$TMP/native_scene_assets"
"$TMP/native_scene_assets"

for src in "$ROOT"/source/*.c "$ROOT"/source/core/*.c "$ROOT"/source/platform/*.c "$ROOT"/assets/converted/*.c "$ROOT"/assets/converted/rooms/*.c "$ROOT"/assets/converted/tilesets/*.c; do
    $CC $FLAGS $INCLUDES -fsyntax-only "$src"
done
python3 "$ROOT/tests/test_asset_layout.py"
python3 "$ROOT/tests/test_hud_font_asset_v17.py"
python3 "$ROOT/tests/test_obj_vram_layout_v17.py"
python3 "$ROOT/tests/test_oam_priority_v18.py"
python3 "$ROOT/tests/test_invisible_marker_assets.py"
python3 "$ROOT/tests/test_static_frame_renderer.py"
python3 "$ROOT/tests/test_static_visibility_renderer.py"
python3 "$ROOT/tests/test_scene_stream_staging.py"
python3 "$ROOT/tests/test_walkthrough_hud_contract.py"
python3 -m py_compile "$ROOT"/tools/*.py
printf '%s\n' 'All host C tests and source syntax checks PASS (hardware build not tested).'
