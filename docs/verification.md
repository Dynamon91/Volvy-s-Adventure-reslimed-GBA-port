## v1.4 verification — 2026-11-22

- Host-side fixed-point, World 1 L1–L3, Shrewy, bunny, bumper, bonus-room and scrolling tests: PASS.
- Added `tests/test_bonus_platforms.c`: source anchors, 4/3 width, dynamic landing, carrying, one-way behavior, patrol limits and reset: PASS.
- ARMv4T/Thumb code generation and relocatable linking with test-only headers: PASS across 33 translation units.
- Moving-platform art: original `spr_moving_platform`, its 4/3 scaled variant and five original cloud-platform animation frames; source OBJ palette now 34 opaque colors.
- Static OBJ cache: 8,512 bytes, plus a separate 2,048-byte player reservation; fits 32 KiB OBJ VRAM.
- Full source-derived bonus-room static composite rebuilt from original `data.win`; generated C SHA-256 equals the previously integrated C asset.
- Moving platforms removed from static AABB collision tiles and handled by runtime one-way AABBs. Static cloud platforms remain in the collision grid.
- Race timer, bunny AI and moving-platform trajectories are provisional. Original scripts, platform per-create code and official race rules remain unavailable.

**Not verified:** the actual devkitARM/libgba ROM cross-build; mGBA boot; hardware gameplay; original moving-platform trajectories and damage masks.
## v1.3 verified additions

- `tests/test_bonus_bunnies.c`: PASS for both original `obj_bunny` pivots, explicit separation from nine `obj_bunny_ground` instances, deterministic bounded motion, hops and reset behavior.
- `tests/test_bonus_race.c`: PASS for square bumper rebound, 12-frame cooldown suppression, corner bumper diagonal response, and generated AABB validation (six square/four corner; one 135° corner exceeds 16×16 pixels). These validate **prototype physics only**.
- `tools/build_bonus_bumper_bounds.py`: generated ten conservatively rotated GBA AABBs directly from original instance scales, rotations and sprite origin (8,8), backed by `bonus_w1_bumper_bounds_report.json`.
- `tests/bonus_bunny_motion_trace.c`: compiled with the host engine and exported deterministic positions for frames 0, 80, 160 and 240; original source sprite frames composited into `previews/bonus_bunny_movement_comparison_v13.png`.
- `sh tests/run_host_tests.sh`: PASS; `sh tests/run_arm_codegen.sh`: PASS for 32 ARMv4T/Thumb translation units with **test-only GBA stubs**.
- Real devkitARM/libgba build and mGBA boot: **NOT RUN**.
