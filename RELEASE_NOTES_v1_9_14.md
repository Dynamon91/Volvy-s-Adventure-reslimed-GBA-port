# Volvy GBA — v1.9.14 image-fidelity comparison

## Objective

Reduce unintended aliasing and make visual previews accurately represent GBA
palette limits, while **preserving the original game's pixel-art aesthetic**.

Nine reference title/selection cards from the 2h22m14s user-supplied 640×360
walkthrough were re-extracted directly as PNG at their recorded timestamps,
then quantized to GBA-compatible BGR555 color.

### Two complete standalone builds

| Variant | Frame handling | GBA ROM bytes | SHA-256 |
|---|---|---:|---|
| Nearest | Existing nearest-neighbour pixel-art sampling | 262,144 | `bb90a047937fe8cf2bbb1aca8746655f5306e1a20c713c9ca145f5e6a3329c56` |
| Box | Area-average downsampling, softer letter/diagonal edges | 262,144 | `19e4109f4eddee87e9570b5d68d6df7cd3ce96f156dc164c41f50fd13cc65e9c` |

Both builds link using Clang ARM7TDMI/Thumb and LLD and have verified GBA
header complement checksums. They each present nine static walkthrough
screens and allow R/RIGHT to advance, L/LEFT to go back, and START to
return to the title screen. **These are screenshot viewers, not playable
Volvy's Adventure game builds.**

The source-derived ROM images, private converted asset ZIPs, and full visual
comparison are delivered directly to the user, **not uploaded to public
GitHub**. Reproducible public source remains in
`tools/build_handheld_select_asset.py` and
`experimental/handheld-card-viewer/`.

### Precision correction

Previously PNG previews used the original quantized 24-bit RGB palette,
even though the GBA uploads only 5 bits/channel in BGR555. The previews
now derive their display colors from the identical packed palette words
used by the GBA. This prevents inaccurate preview colors.

The 81:30 source frame better reads **Macaroni Metro** (not the former
“Macho Metro” guess); the original game's text assets remain the required
authority for definitive spelling.

### Verification and limits

- Public converter host tests for 16/256 colors, nearest/box/lanczos modes,
  LZ77 round-trip, 12/13px framing and exact preview palette: **PASS**.
- Isolated ARM7TDMI GBA viewer code generation/link with synthetic cards:
  **PASS** (GitHub Actions).
- Both private reference-card .gba builds and header checks: **PASS**.
- mGBA actual screen boot/rendering: **NOT TESTED**.
- Physical GBA: **NOT TESTED**.
- The complete game ROM and any newly playable level: **NOT BUILT**.

Overall native GBA porting roadmap estimate remains **44%**.
