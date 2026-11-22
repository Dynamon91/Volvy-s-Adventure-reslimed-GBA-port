# v1.9.6 verification — 22 November 2026

## Walkthrough evidence
- Six sequential uploaded MP4 parts joined and verified.
- Combined bytes: 601,191,189; duration: 8,534.076372 seconds.
- Video: H.264 640×360 at 30 FPS; audio: AAC.
- 24 timestamped keyframes and six 30-second contact sheets generated.
- Evidence index: `docs/walkthrough-reference-v1_0.md`.

## Build and tests
- Portable host C suite, including `test_game_timer.c`: **PASS**.
- Walkthrough HUD source contract: **PASS**.
- ARMv4T/Thumb relocatable code generation: **PASS** (43 translation units; test stubs).
- Experimental Clang/LLD ROM: **PASS** (44 C translation units).
- ROM size: 2,097,152 bytes (2 MiB).
- ROM SHA-256: `ef8b7635c458156fa133d2e07a8f9f6c5dba79c2b484ed4cf5766383aa8e2ff5`.

## Not tested
- mGBA boot or actual rendered HUD.
- Official devkitARM/libgba build.
- Physical GBA hardware.

The new timer formatter is covered by C tests, but its visual placement is not
verified in an emulator. The walkthrough is source-game evidence, not a GBA
build test. Overall reconstruction estimate remains **44%** and is not a
measured fidelity score.
