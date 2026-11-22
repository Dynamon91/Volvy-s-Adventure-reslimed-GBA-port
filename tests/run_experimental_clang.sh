#!/bin/sh
# Builds an experimental cartridge IMAGE; does not test emulator boot.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT INT TERM
${CC:-gcc} -std=c11 -Wall -Wextra -Werror -O2 -fno-builtin \
  "$ROOT/tests/test_experimental_minicrt.c" \
  "$ROOT/experimental/clang-gba/minicrt.c" -o "$TMP/minicrt_host"
"$TMP/minicrt_host"
python3 "$ROOT/experimental/clang-gba/build.py"
python3 "$ROOT/tests/verify_experimental_rom.py" \
  "$ROOT/build-clang-experimental/volvy_reslimed.gba" \
  "$ROOT/build-clang-experimental/volvy_reslimed.elf"
