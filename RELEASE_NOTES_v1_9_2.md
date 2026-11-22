# Volvy's Adventure: Reslimed — GBA v1.9.2

## Continuation of v1.9.1 (renderer-only correction)

World 1 and Bonus Race static OBJ actors previously selected/requested their
animation frame **before** rejecting their off-screen metasprite parts. An
invisible actor could consume the shared 6,144-byte per-VBlank static animation
transfer allowance, preventing a visible actor's requested frame from uploading
in that VBlank. This can cause visible animations to appear stalled under load.

The renderer now checks each actor's actual metasprite part bounds against the
240×160 GBA viewport **before** invoking `request_static_frame`. The culling
rule checks both the requested animation frame and the previously resident
frame, guarding against changes in animation-frame geometry near an edge.
Both world and bonus paths use the same part-rectangle clipping predicate,
including the exact viewport edges and off-screen OBJ coordinate wrapping.

Unchanged: v1.9.1 frame scheduler; source-derived art at 1:1 pixel scale; World
1 levels 1–3; Shrewy; Bonus Race; HUD priority; original object placements;
physics; green marker suppression. No emulated screenshots or actual visual
fidelity claims are inferred from structural ROM checks.

## Performed verification

- `sh tests/run_host_tests.sh`: PASS (new off-screen visibility and source-integration tests included).
- `sh tests/run_arm_codegen.sh`: PASS (test headers/stubs, *not* devkitARM/libgba).
- `sh tests/run_experimental_clang.sh`: PASS (Clang/LLD, 42 C units; 2 MiB GBA image, header and ELF checks).

## To validate in mGBA

1. Boot `Volvy_Reslimed_GBA_v1.9.2_experimental.gba`.
2. Test World 1 L1–L3 and Bonus Race in areas with many sprites: check for
   animation stalls, sprite wrapping, HUD overlap and camera seams.
3. Confirm Shrewy and return-to-world transitions. Compare gameplay to a
   capture of the original game; send the timestamp when a discrepancy occurs.

The ROM is **experimental**. mGBA boot, execution speed, zero flicker and
hardware compatibility are **not yet verified**. Current conservative total
project-completion estimate is still 44% (several levels/worlds and mechanics
remain to be reconstructed).
