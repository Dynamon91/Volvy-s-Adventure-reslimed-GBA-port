# Changelog

## v1.9.8 — walkthrough-derived Stage Clear screen

- Add a static Stage Clear result frame sampled from the user's walkthrough at 04:00.
- Convert the 240×135 reference frame to 16-color 4bpp pixels, BIOS-LZ77 compress it and include the binary asset in the project.
- Expand the packed pixels into a 240×160 Mode 4 bitmap with 12px / 13px purple margins.
- Gate the screen to the provisional W1-L1 victory route; do not reuse it for boss or bonus victories.
- Queue bitmap/palette writes for VBlank and restore Mode 0 before the next scene streams in.
- Add asset decompression, palette, dimensions and renderer-transition regression tests.
- BIOS-LZ77 round-trip and renderer source-contract inspection: PASS. C syntax, ARM code generation, full ROM build and mGBA rendering: NOT VERIFIED.
- The captured result values/art remain static; dynamic combo, rank and score calculations are still missing.

## v1.9.7 — walkthrough-derived HUD icon pass

- Add 16×16 onion and stopwatch icons sampled from the supplied gameplay walkthrough.
- Pack both icons into eight 4bpp BG tiles immediately after the 40 source-font tiles.
- Preserve palette index 0 transparency and index 1 white for the original font.
- Add one-tile HUD margins and keep the timer group right-aligned.
- Use quotient/remainder centisecond conversion for the HUD cache key to avoid early multiplication overflow.
- Add icon-payload/layout regression coverage and extend the HUD contract.
- UI syntax with minimal GBA stubs and the two patch-level Python tests: PASS.
- Source-only update: no v1.9.7 ROM, ARM codegen, devkitARM build, mGBA boot or hardware test is claimed.
- Icon graphics are reconstructed from compressed walkthrough footage and are not claimed as bit-identical source sprites.
