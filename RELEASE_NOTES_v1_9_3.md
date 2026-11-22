# Volvy's Adventure: ReSlimed — GBA v1.9.3

**Built strictly from the recovered v1.9 → v1.9.1 → v1.9.2 source tree.**

## Fix: Phantom sprite wraparound and OAM capacity

The native player, Shrewy and Shrewy-rock draw path used to send OAM entries
for *all* metasprite parts, even those whose screen coordinates were entirely
outside the 240×160 GBA viewport. GBA hardware stores OBJ X in 9 bits and Y
in 8 bits. A far-off actor could therefore wrap onto a visible part of the
screen on a room change, vertical camera move or boss/projectile movement.

The new `platform/obj_parts.c` renderer intersects each hardware part's bounds
with the viewport before reserving OAM. It compacts visible OBJ parts, while
keeping their original 8bpp frame tile offsets (including the existing
64-byte unit alignment), H-flip geometry and priority. The caller advances
through the **actual** OAM entries used; the 128-entry limit is enforced.
The existing v1.9.2 culling of World 1 / Bonus Race static actors is preserved.

No original graphics are downscaled, redrawn or replaced. No character actions,
collision, physics, object placement, stage route, animation timing, or boss
logic is altered. This is a correctness-focused rendering fix.

## Test results (22 November 2026)

- `sh tests/run_host_tests.sh`: **PASS**, including the new compiled OBJ unit test.
- `sh tests/run_arm_codegen.sh`: **PASS**, 42 C translation units (test-only GBA headers).
- `sh tests/run_experimental_clang.sh`: **PASS**, 43 C translation units and structural ROM verification (Clang/LLD; not devkitARM/libgba).
- ROM file: `Volvy_Reslimed_GBA_v1.9.3_experimental.gba`
- ROM size: 2,097,152 bytes (2 MiB); SHA-256: `d40a89a77b1fc5acecbbbb433e92380c57f48ed45b079b6db188a5aa249dd792`.
- mGBA boot/gameplay and physical GBA: **NOT TESTED**.

Estimated overall reconstruction completeness is **44%**, held at v1.9.2's
estimate rather than inflating it for an isolated renderer defect fix.

## Suggested gameplay recording

On mGBA, visit the World 1 L1–L3 sections with large camera movement. Then
visit Shrewy, jump between boss and rock positions, and look for any sprites
teleporting to the opposite edge. Check that the sprite layer and HUD remain
stable. A video with the ROM version visible is useful for a targeted comparison.

The `previews/v193_source_only_check.png` image in the private source archive
uses source-derived room art; it is **not** a screenshot of the ROM running on
the emulator.
