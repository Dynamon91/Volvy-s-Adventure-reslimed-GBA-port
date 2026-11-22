# Volvy's Adventure: Reslimed — GBA prototype v1.2

This milestone expands the 2:1 GBA bonus race prototype with original bunny, Swolvy-only block, square-bumper and triangular-bumper artwork recovered from the user-supplied GameMaker archive. The new shared OBJ sprite package contains 41 animation sets / 32 opaque palette colors. The converted full room background remains 486 tiles / 31,104 bytes. The expanded static OBJ graphics cache is 7,360 bytes (plus a 2 KiB player-frame reservation).

The two source Swolvy-only blocks now use `TILE_SWOLVY_BLOCK=9` and cannot be broken by normal Volvy. The provisional five-onion Swolvy trigger is disabled in the bonus room to avoid contradicting supplied traces with onion collection and no Swolvy frames. Bunny sprites are animated visually at the original 11 placements, but competitor AI and bumper behavior are **not** reconstructed.

New `core/scene_window` regression tests exercise the 64×32 BG map's vertical and horizontal coverage. An OAM-density audit checks all 969,217 integral camera positions: maximum 93 visible static actors, estimated 101/128 OBJ slots including eight reserved player parts. The audit does **not** simulate scanline OBJ cycles.

**Verified:** host C tests, Python asset checks, ARMv4T/Thumb code generation for 30 translation units using test stubs, archive integrity.

**Not verified:** actual devkitARM/libgba build, emulator boot, original collision-mask/AI fidelity, audio, real GBA performance. No working `.gba` file is claimed in this package.

Run `sh tests/run_host_tests.sh` and `sh tests/run_arm_codegen.sh`. A real build requires a devkitARM/libgba installation followed by `make clean && make`; test the resulting ROM in mGBA before claiming playability.

Visual previews in `previews/bonus_*_v12.png` are source-art/compiled-tile reconstructions, **not emulator screenshots**.

**Informal planning estimate: 35%** of the intended complete port. Not a validated percentage of total gameplay fidelity.
