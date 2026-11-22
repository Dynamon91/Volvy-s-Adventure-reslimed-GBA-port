#!/bin/sh
# Isolated ARM7TDMI code generation only; this is not a full cartridge link.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
CLANG=\${CLANG:-clang}
OUT=\${OUT:-"$ROOT/build-public-arm-check"}
mkdir -p "$OUT"
cd "$ROOT"
FLAGS="--target=armv4t-none-eabi -mcpu=arm7tdmi -mthumb -std=gnu11 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -O2 -Wall -Wextra -Werror"
INCLUDES="-I$ROOT/experimental/clang-gba/include -I$ROOT/include -I$ROOT/tests/support -I$ROOT/tests/support/arm_includes"
$CLANG $FLAGS $INCLUDES -c "$ROOT/source/platform/stage_clear_screen.c" -o "$OUT/stage_clear_screen.o"
$CLANG --target=armv4t-none-eabi -mcpu=arm7tdmi -c "$ROOT/experimental/clang-gba/stage_clear_asset.s" -o "$OUT/stage_clear_asset.o"
printf '%s\n' "ARM7TDMI Stage Clear module + embedded asset codegen PASS: $OUT"
printf '%s\n' 'This does not verify the complete renderer link, ROM boot, or mGBA output.'
