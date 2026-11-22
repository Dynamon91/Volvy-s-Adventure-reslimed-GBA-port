# Stage Clear screen reconstruction — v1.9.8

## Source evidence

- Reference: the six-part user-supplied walkthrough, offset **04:00** (240 seconds).
- Captured source frame: 640×360.
- Directly visible: purple results background, orange STAGE CLEAR! banner, Volvy thumbs-up artwork, green splat, the line "No damage: Failure...", and five thumbs-up/rating icons.
- The walkthrough index identifies this as the end of the first outdoor platforming sequence. The match between that sequence and the current GBA W1-L1 stage is **provisional**, not proven.

## GBA representation

- Preserve the complete 16:9 frame; scale to 240×135 and letterbox to the GBA's 240×160 display with 12 top / 13 bottom rows.
- Quantize to 16 colors and store the image as 4bpp pixels, low nibble first.
- Raw packed image: 16,200 bytes.
- BIOS-compatible LZ77 asset: 8,780 bytes including word-alignment padding.
- At runtime, decompress into aligned EWRAM, expand the 4bpp indices into a 240×160 8bpp Mode 4 framebuffer, and upload the 16-color BGR555 palette.
- The bitmap and palette are committed during VBlank. Mode 0 is restored before the native tiled scene renderer resumes.

## Integration scope

The static result image is enabled only for GAME_VICTORY when active_stage == 1, excluding boss and bonus rooms. This is a cautious first integration because the walkthrough's 04:00 outdoor stage is only tentatively mapped to W1-L1.

Boss and bonus victory paths keep the existing renderer. Other standard stages keep the generic result fallback until matching reference captures are mapped.

## Fidelity limitations

This is a **single captured result frame**, not a dynamic result system. Its displayed text, damage status and five thumbs are static and do not yet reflect the current run. Combo totals, score/rank calculations, animated splat/banner, boss-clear layouts, and stage-specific result values remain future work.

The preview is reconstructed from the walkthrough frame after GBA palette reduction. It is not an mGBA screenshot, and the v1.9.8 ROM has not been built or boot-tested.
