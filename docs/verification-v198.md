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
| GCC syntax check of stage_clear_screen.c with the repository test GBA shim | **PASS** |
| Host C runtime test: prepare-once, nibble expansion, letterbox, palette/frame commit and Mode 0 restore (deterministic LZ77 stub) | **PASS** |
| ARM7TDMI/Thumb code generation of Stage Clear C module and embedded asset object | **PASS** |

The runtime test executes the actual Stage Clear C module against a host GBA shim. Its LZ77 function is deterministic test data, so this test validates pixel expansion, frame layout, palette/frame transfer and display-mode transitions; the separate asset test validates the real BIOS-LZ77 stream. The isolated ARM check compiles the module using the experimental GBA headers. These checks do not compile the full renderer or prove the normal libgba link.

## Not verified

- Full host C suite (the public repository omits the private generated room, collision, sprite and tile assets required by older tests).
- Full-project ARMv4T/Thumb code generation and link.
- Full devkitARM/libgba ROM build.
- mGBA boot or actual rendered Stage Clear screen.
- Physical GBA hardware.

A public CI workflow has been added for asset-independent public regressions and isolated Stage Clear ARM code generation. No completed GitHub Actions run is recorded yet.

**Release status:** source + reference asset only; no v1.9.8 .gba image was generated.

The walkthrough's first outdoor result screen is tentatively mapped to W1-L1; that mapping still needs gameplay confirmation. The image is static and does not calculate score, damage, combo or rank. The overall reconstruction estimate remains **44%**, a planning estimate rather than a measured fidelity score.
