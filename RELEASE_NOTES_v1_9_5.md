# Volvy's Adventure: ReSlimed — GBA v1.9.5 (22 Nov 2026)

## Gameplay collision correctness — test-reproduced bug fix

Continues **directly from v1.9.4** without changing artwork, source-derived
room placements, animation frames, OBJ palette, BG cache, menu styling,
or camera behavior. **Not a full 1:1 rebuild.**

### Reproduced failures before this patch

1. **Fast dash passes through a solid tile:** A player hitbox moving from
   x=48 to x=68 (20 gameplay pixels), with an 8px wall at x=64..71, was
   checked only at its destination. The wall was skipped.
2. **Negative world boundary leak:** C integer division truncated negative
   coordinates toward zero. Pixel x=-3 was checked as tile 0 instead of
   outside tile -1, letting the player slip beyond the left level edge.
3. **Potential fast-impulse platform tunneling:** Direct collision tests for
   higher speeds could skip solid or one-way rows when checking only the
   final Y coordinate. Standard movement caps mask this case in most scenes.

### Implemented

- Traverse all tile columns and rows crossed by the movement's leading edge.
- Preserve one-way-from-above behavior, breakable tile destruction, contact
  snapping, dash stopping and grounded semantics.
- Floor negative pixel coordinates to the correct negative tile coordinate.
- Add two C regression programs. A horizontal dash regression **fails on the
  unmodified v1.9.4 engine and passes on v1.9.5**; vertical tests directly
  exercise solid and one-way platforms under synthetic high-speed impulses.

### Tests (2026-11-22)

- Portable host C regression suite and source checks: **PASS**.
- ARMv4T/Thumb relocatable code generation: **PASS** (42 translation units,
  test-only GBA headers, not official devkitARM).
- Experimental Clang/LLD ROM structure: **PASS** (43 C translation units,
  2,097,152-byte image; 1,241,768-byte payload).
- ROM SHA-256: `cd0c8c48650b5536ef52906b78676ebce29d82ab887836cda5f759861bf88f3c`.
- Actual mGBA boot, native frame timing and physical GBA: **NOT TESTED**.

The original release advertises four worlds. The active GBA ROM contains
five native scenes (World 1 levels 1–3, Shrewy and the World 1 Bonus Race),
with provisional logic. This patch fixes a demonstrable engine error, not
missing game content. The earlier 44% project-completeness estimate is not a
verified measure of 1:1 gameplay or visual fidelity.
