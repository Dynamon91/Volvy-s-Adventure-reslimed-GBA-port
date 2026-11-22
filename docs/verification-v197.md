# v1.9.7 source verification — 22 November 2026

## Implemented source changes

- Added two walkthrough-derived 16×16 HUD icons as eight 4bpp BG tiles.
- Appended icon tiles after the 40-tile source font without overlapping its range.
- Installed the icon palette in BG palette bank 15 while preserving white font index 1 and transparent index 0.
- Added a one-tile margin around the HUD groups and retained the existing right-aligned timer.

## Checks completed for this patch

| Check | Result |
|---|---|
| `source/platform/ui.c` syntax with minimal GBA/font stubs (`gcc -std=c11 -Wall -Wextra -Werror -fsyntax-only`) | **PASS** |
| `tests/test_hud_walkthrough_icons.py` asset/palette/layout contract | **PASS** |
| `tests/test_walkthrough_hud_contract.py` HUD integration contract | **PASS** |

The UI syntax check and the two source-contract checks are isolated patch-level checks; they are not a full project build.

## Not verified for v1.9.7

- Full host C suite (requires private generated source-room assets omitted from the public repository).
- ARMv4T/Thumb code generation.
- Experimental Clang/LLD ROM generation and ROM checksum.
- devkitARM/libgba build.
- mGBA boot or rendered HUD.
- Physical GBA hardware.

**Release status:** source-only patch; no v1.9.7 `.gba` image was generated. Do not treat the video reference as proof of a GBA build or emulator boot.

The project planning estimate remains **44%**, unchanged from v1.9.6 and not a measured fidelity score.
