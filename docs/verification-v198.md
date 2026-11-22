# v1.9.8 source verification — 22 November 2026

## Implemented

- Added assets/converted/ui/stage_clear_v198.bin, a BIOS-LZ77 stream derived from the 04:00 walkthrough Stage Clear frame.
- Added a 16-color BGR555 palette and a Mode 4 bitmap renderer.
- Added a guarded first-stage victory route; boss and bonus result paths are excluded.
- Queued bitmap/palette writes for VBlank and added a Mode 0 restore path before the next scene load.
- Added a standard devkitPro bin2o rule for the new binary asset.

## Checks actually completed

| Check | Result |
|---|---|
| Independent BIOS-LZ77 round-trip of the 8,780-byte asset | **PASS** |
| 240×135 packed-pixel dimensions and 16-color palette inspection | **PASS** |
| Renderer gate / VBlank / restore source-contract inspection | **PASS** |
| C compiler syntax check of stage_clear_screen.c | **NOT RUN** |
| ARMv4T/Thumb code generation | **NOT RUN** |

The first three checks were performed against the asset and repository source. The C and ARM compile claims made in an earlier status were not backed by an executed compiler result and are withdrawn here.

## Not verified

- Full host C suite (the public repository omits the private generated room, collision, sprite and tile assets required by older tests).
- Full-project ARMv4T/Thumb code generation and link.
- Full devkitARM/libgba ROM build.
- mGBA boot or actual rendered Stage Clear screen.
- Physical GBA hardware.

A public CI workflow has now been added. It runs asset-independent public tests and isolated ARM7TDMI code generation for the Stage Clear module; no completed workflow result is recorded yet.

**Release status:** source + reference asset only; no v1.9.8 .gba image was generated.

The walkthrough's first outdoor result screen is tentatively mapped to W1-L1; that mapping still needs gameplay confirmation. The image is static and does not calculate score, damage, combo or rank. The overall reconstruction estimate remains **44%**, a planning estimate rather than a measured fidelity score.
