# User mGBA video review — v1.9 marker cleanup

## Capture facts

- File: user-uploaded `20261122_111225.mp4`.
- Duration: 112.7667 seconds (3,383 frames at 30 FPS).
- Dimensions: 1920×1080.
- Emulator title bar: `mGBA - VOLVY RESLIM (60 fps)`.
- The recording shows the title screen and W1-L1 gameplay, including onion pickups, breakable blocks, pipe sections, normal Volvy, Swolvy, Big Hand movement, vertical camera travel and pause.
- The capture does not show a ROM filename or checksum. It proves a Volvy GBA build runs in mGBA, but does not identify whether it is v1.8 or another revision.

## Visual issue found

A neon-green square appears on a grass platform in the later W1-L1 section (for example around 100.8 seconds). It is not part of the native BG scene. The screen pixel average is approximately RGB (48,253,97).

The generated source sprite `spr_hole` is a 16×16 single-part frame with 256 pixels all using OBJ palette index 36. Its BGR555 color is `0x33E6`, which converts to RGB (49,255,98). This is a direct color match to the visible square. The `obj_hole` and `obj_hole_dir` source placements therefore use a uniform green logic/placement marker, not finished visible artwork.

## v1.9 correction

- Keep `WORLD_HOLE`, `BONUS_W1_ROOM_OBJ_HOLE` and `BONUS_W1_ROOM_OBJ_HOLE_DIR` in source room/object tables for future behavior reconstruction.
- Do not map those markers to an OBJ animation in World 1 or Bonus Race.
- Remove `STATIC_HOLE` and the `g_spr_hole` VRAM cache slot.
- Keep the original `spr_hole` asset and manifest entry for source auditing.
- Update OAM analysis tools to omit invisible hole markers from future visible-sprite estimates.

## Verification status

- Host C regression suite: PASS.
- New marker-source/render-exclusion regression: PASS.
- ARMv4T/Thumb code generation: PASS (40 translation units, test-only headers).
- Experimental Clang/LLD ROM structure: PASS (2 MiB image, 1,240,652-byte payload, 41 C files).
- devkitARM/libgba build: not verified; `DEVKITARM` is not configured in this environment.
- mGBA executable/boot: not available in this environment; v1.9 ROM retest is pending.
- Physical GBA: not tested.

`previews/v19_hole_marker_before_after.png` shows the actual uploaded frame, the extracted marker, and a source-scene reconstruction with the marker suppressed. The right panel is not a screenshot from the v1.9 ROM.
