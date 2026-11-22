# v1.3 verification — 2026-11-22

## Source-derived additions

The user-supplied GameMaker `data.win` was privately extracted and parsed. The four added source sprite resources are `spr_bunny_hop` (4 frames, source 48×48, origin 25×40), `spr_destructible_block_swolvy` (1 frame, source 32×32), `spr_bumper_sq` (1 frame, source 16×16), and `spr_bumper_tri` (1 frame, source 16×16). The 90 selected source sprites were extracted without errors. The converted player/world/bonus package contains 41 animations and 32 opaque 8bpp OBJ colors plus transparent index 0.

The game renderer now draws all 11 bunny placements, two Swolvy-only blocks, six square bumpers and four corner bumpers with the original scaled assets. The original bunny and bumper GameMaker scripts are still unavailable. Version 1.3 implements separate **provisional** motion for the two `obj_bunny` instances and collision response for six square and four corner bumpers. Those responses must not be mistaken for original competitor AI or original knockback behavior.

The source collision converter now emits `TILE_SWOLVY_BLOCK=9`, with eight final occupied cells, separate from 271 normal breakable cells. The player controller accepts destruction by Swolvy but blocks normal Volvy. The source bonus traces contain recorded onion pickups but no Swolvy states, so the provisional five-onion auto-transformation was disabled in `GAME_BONUS`; the authentic transformation rule remains unknown.

## Host and static verification

- `sh tests/run_host_tests.sh`: **PASS**. Includes prior World 1 L1–L3, Shrewy and source collision/action tests; new type-9 Swolvy block behavior and bonus no-auto-transform assertions; 64×32 BG window horizontal/vertical coverage tests; 8bpp asset and OAM budgets.
- `sh tests/run_arm_codegen.sh`: **PASS**, 32 translation units cross-compiled to ARMv4T/Thumb objects and combined relocatably with **test-only** GBA headers. Not a libgba SDK build or bootable ROM.
- `tools/analyze_bonus_oam_density.py`: evaluated **969,217** integral 240×160 camera positions against the original 672 drawable actor placements and generated hardware OBJ frame bounds. Worst-case 93 static actors, **101 / 128** estimated OAM entries allowing eight player parts. Does **not** test hardware per-scanline OBJ cycles.
- Original full static bonus BG composite: 486 8bpp tiles / 31,104 bytes, under the 32 KiB combined BG charblock budget. Scene uses a 332×70 source map with a 64×32 streamed hardware window. All image previews are **source-art/compiled-C compositions**, not emulator screenshots.
- A parser-only Makefile check is possible with a dummy `gba_rules`; this does not prove a working devkitARM build.

## Not verified or not implemented

- A real devkitARM/libgba `.gba` ROM build and mGBA/physical GBA boot are **not verified**. No SDK or mGBA executable is installed in this environment.
- The original bonus race timer/rewards, authentic bunny opponent AI, authentic bumper physics, dynamic moving platforms, live background parallax, original room-entry/exit rules, and full race time are not reconstructed.
- Bonus collision still uses source-object transformed AABBs rather than original pixel collision masks. The four source trace clips have overlapping `time_ms` ranges and no bunny opponent state. The original GameMaker event logic is not present as readily decompilable code.
- Most other worlds, boss mechanics, save data, native intro/title transitions, sound/music playback and original-level correctness verification remain incomplete.

## Reproduction

```sh
sh tests/run_host_tests.sh
sh tests/run_arm_codegen.sh
# For a real SDK build, install devkitPro devkitARM + libgba first:
make clean && make
# Boot the produced volvy_reslimed.gba in mGBA and inspect rendering, OAM,
# physics, audio and frame timing. None of those hardware tests were run here.
```

## v1.3 verified additions

- `tests/test_bonus_bunnies.c`: PASS for both original `obj_bunny` pivots, explicit separation from nine `obj_bunny_ground` instances, deterministic bounded motion, hops and reset behavior.
- `tests/test_bonus_race.c`: PASS for square bumper rebound, 12-frame cooldown suppression, corner bumper diagonal response, and generated AABB validation (six square/four corner; one 135° corner exceeds 16×16 pixels). These validate **prototype physics only**.
- `tools/build_bonus_bumper_bounds.py`: generated ten conservatively rotated GBA AABBs directly from original instance scales, rotations and sprite origin (8,8), backed by `bonus_w1_bumper_bounds_report.json`.
- `tests/bonus_bunny_motion_trace.c`: compiled with the host engine and exported deterministic positions for frames 0, 80, 160 and 240; original source sprite frames composited into `previews/bonus_bunny_movement_comparison_v13.png`.
- `sh tests/run_host_tests.sh`: PASS; `sh tests/run_arm_codegen.sh`: PASS for 32 ARMv4T/Thumb translation units with **test-only GBA stubs**.
- Real devkitARM/libgba build and mGBA boot: **NOT RUN**.
