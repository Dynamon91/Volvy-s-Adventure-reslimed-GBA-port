# Volvy's Adventure: Reslimed — GBA v1.9.1

## Targeted fix: OBJ animation frame coherence

This update is **based on the user's recovered and hash-verified v1.9**,
not the older GitHub main revision. The original v1.9 `.gba` and ZIP are
preserved unchanged separately.

On v1.9 the world and bonus actor renderer drew OAM geometry from each
animation's `loaded_frame`, but requested its next frame's tile upload later
in the *same* VBlank. This could display a new frame's tiles with the
previous frame's OAM part positions/shape for a single presented frame.

v1.9.1 adds a deterministic `StaticFrameScheduler`:

- Shares one VBlank upload budget across at most 25 animation sets.
- Selects each animation set at most once per draw, regardless of instance count.
- Only draws a new animation frame when its complete graphics transfer fits
  the 6144-byte static-actor VBlank budget; otherwise keeps the uploaded
  previous frame until another VBlank.
- Uses that same selected frame for the OAM geometry and the VBlank upload.
- Adds host C unit tests and a source integration regression for this ordering.

No changes to world geometry, room placement, collision physics, original
source sprite pixels, the green `spr_hole` marker exclusion, or input mapping.

## Verification performed

- `sh tests/run_host_tests.sh` — PASS (includes new static-frame unit and renderer tests).
- `sh tests/run_arm_codegen.sh` — PASS (41 translation units, test GBA stubs).
- `sh tests/run_experimental_clang.sh` — PASS (42 C files, structural checks of
  2 MiB experimental ROM; no emulator proof).
- ROM built from v1.9.1 source is **not** bit-identical to v1.9, as expected.

**Remaining:** mGBA boot, hardware compatibility, actual absence of sprite
flicker and frame timing under load. DevkitARM/libgba builds are unverified.
