# Volvy's Adventure: Reslimed — GBA v1.9

## Capture-driven fix

- Reviewed the newly supplied mGBA gameplay capture (112.766 s, 1920×1080, 30 FPS).
- Identified a neon-green 16×16 square as the source `spr_hole` marker: 256 pixels all use palette index 36 (BGR555 `0x33E6`, RGB 49,255,98).
- Removed world `WORLD_HOLE` and bonus `HOLE`/`HOLE_DIR` marker sprites from the runtime renderer. Source placements remain available for logic reconstruction.
- Removed the unused hole marker from the static OBJ cache. Active visible cache is 25 animation sets / 18,560 bytes; with the 5,120-byte player reserve, 9,088 bytes of OBJ VRAM remain free.
- Updated OAM audit tools and added a regression test for marker classification and rendering exclusion.

## Verification

- Host C regression suite: PASS.
- Invisible source-marker regression: PASS.
- ARMv4T/Thumb code generation: PASS (40 translation units with test-only headers).
- Experimental Clang/LLD ROM: PASS structural checks (2 MiB, 1,240,652-byte payload, 41 C files); this is not a devkitARM/libgba build.
- mGBA boot of the v1.9 ROM: NOT YET TESTED.

See `docs/mgba-video-review-v19.md` and `previews/v19_hole_marker_before_after.png` for the source-derived diagnosis and visual comparison.
