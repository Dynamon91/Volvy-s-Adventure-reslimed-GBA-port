# Volvy's Adventure: Reslimed — GBA v1.3

This revision adds **provisional, source-anchored bunny movement** to the two original `obj_bunny` instances and **source-rotation-aware bumper hitboxes** for six square and four corner objects. Nine source `obj_bunny_ground` placements remain static. It does not claim to reproduce the missing GameMaker enemy scripts, official race results or rewards.

New regression tests pass for bunny start positions, bounded movement, hops, reset, source instance distinction, bumper collision/impulses, suppression cooldowns and a rotated 135-degree corner.

The included side-by-side preview uses original extracted graphics and frame positions emitted by host-compiled project C, not a screenshot of a running `.gba` ROM.

**Verification:** host C tests and asset checks pass; ARMv4T/Thumb code generation and relocatable link pass for 32 units with test-only headers. Actual devkitARM/libgba cross-build and mGBA boot remain unverified.

**Estimated overall completion:** 36% (project planning figure, not measured playability).
