#!/bin/sh
# Isolated ARM7TDMI code generation only; this is not a full cartridge link.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
CLANG=clang
OUT="$ROOT/build-public-arm-check"
mkdir -p "$OUT"
cd "$ROOT"
FLAGS="--target=armv4t-none-eabi -mcpu=arm7tdmi -mthumb -std=gnu11 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -O2 -Wall -Wextra -Werror"
INCLUDES="-I$ROOT/experimental/clang-gba/include -I$ROOT/include -I$ROOT/tests/support -I$ROOT/tests/support/arm_includes"
$CLANG $FLAGS $INCLUDES -c "$ROOT/source/platform/stage_clear_screen.c" -o "$OUT/stage_clear_screen.o"
$CLANG --target=armv4t-none-eabi -mcpu=arm7tdmi -c "$ROOT/experimental/clang-gba/stage_clear_asset.s" -o "$OUT/stage_clear_asset.o"
$CLANG $FLAGS $INCLUDES -c "$ROOT/source/platform/handheld_card_screen.c" -o "$OUT/handheld_card_screen.o"
# Asset-independent gameplay progression modules now receive ARMv4T/Thumb
# code-generation checks too; this still is NOT a full executable ROM build.
$CLANG $FLAGS $INCLUDES -c "$ROOT/source/core/run_stats.c" -o "$OUT/run_stats.o"
$CLANG $FLAGS $INCLUDES -c "$ROOT/source/core/campaign_progress.c" -o "$OUT/campaign_progress.o"
$CLANG $FLAGS $INCLUDES -c "$ROOT/source/core/result_text.c" -o "$OUT/result_text.o"
printf '%s\n' "ARM7TDMI bitmap and gameplay-record codegen PASS: $OUT"
printf '%s\n' 'This does not verify the complete renderer link, ROM boot, or mGBA output.'
