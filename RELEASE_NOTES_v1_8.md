# Volvy's Adventure: Reslimed — GBA v1.8

This revision is based on review of the user's mGBA gameplay capture.

## Visual correction

Gameplay actors were using OBJ priority 0, the same priority as the BG0 HUD. On GBA, an OBJ wins an equal-priority tie against a background, so foreground sprites could cover HUD text during vertical camera sections.

v1.8 assigns priority 1 to gameplay player parts, world actors, bonus actors, boss parts, boss rocks and boss onions. The BG0 HUD remains priority 0; the BG1 world scene remains priority 1. This keeps the HUD above gameplay sprites while preserving actors in front of the scene. The title-screen Volvy remains at priority 0 to preserve the title composition.

## Verification

- Host C regression suite: PASS.
- HUD/OAM priority regression: PASS.
- ARMv4T/Thumb code generation: PASS (40 translation units, test-only headers).
- Experimental Clang/LLD ROM: PASS structural checks (2 MiB, 1,240,964-byte payload, 41 C files).
- devkitARM/libgba build: NOT VERIFIED.
- v1.8 mGBA boot: NOT YET TESTED.

The user's video shows a Volvy GBA build running in mGBA, but it does not expose a ROM checksum and therefore cannot identify the exact binary revision.
