#!/bin/sh
# Compile ARMv4T/Thumb relocatable objects with test-only GBA headers.
# This does NOT build or validate a working GBA ROM or link against libgba.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
CLANG=${CLANG:-clang}
LLD=${LLD:-ld.lld}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT INT TERM
INCLUDES="-I$ROOT/tests/support/arm_includes -I$ROOT/tests/support -I$ROOT/include -I$ROOT/assets/converted -I$ROOT/assets/converted/rooms -I$ROOT/assets/converted/tilesets"
N=0
for SRC in "$ROOT"/source/*.c "$ROOT"/source/core/*.c "$ROOT"/source/platform/*.c \
           "$ROOT"/assets/converted/*.c "$ROOT"/assets/converted/rooms/*.c \
           "$ROOT"/assets/converted/tilesets/*.c; do
    N=$((N+1))
    # shellcheck disable=SC2086
    "$CLANG" --target=armv4t-none-eabi -mcpu=arm7tdmi -mthumb \
        -ffreestanding -std=gnu11 -O2 -Wall -Wextra -Werror \
        $INCLUDES -c "$SRC" -o "$TMP/$N.o"
done
"$LLD" -flavor gnu -r "$TMP"/*.o -o "$TMP/combined.o"
printf 'ARMv4T/Thumb code generation and relocatable link: PASS (%s translation units, test stubs only)\n' "$N"
printf '%s\n' 'Actual devkitARM/libgba cross-build and mGBA boot remain unverified.'
