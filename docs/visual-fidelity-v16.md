# Visual fidelity pass — v1.6

## Why the previous output looked too pixelated

The previous active sprite and background conversion reduced original source coordinates by 2 before rendering. A source sprite such as Volvy's 24×19 idle frame became 12×10 pixels on the GBA. Enlarging that reduced image could not recover the source outline or interior shading.

v1.6 changes the active render path to one source pixel per GBA pixel. It does not use bilinear filtering, artificial sharpening or generated replacement art. The conversion retains original source pixels and quantizes colors to the GBA's 15-bit BGR format.

## Sprite changes

- 56 original animation sets are converted at 1:1.
- OBJ sprites remain 8bpp to preserve the shared 71-color palette.
- Multi-part metasprites are supported. The largest player frame is 4,608 bytes.
- 26 static animation sets share fixed VRAM slots. Only one current frame per animation is resident; changed frames are streamed during VBlank.
- The player has a 5,120-byte frame reservation. Static actor slots use 18,816 bytes, ending at byte 23,936 of 32 KiB OBJ VRAM.
- Added the source graphics for bonus-room enemies and props, and expanded the World 1 object table to draw several previously omitted actors.

## Background changes

- Five active scenes are converted to native 4bpp.
- Each tile map cell carries a 10-bit cached tile index, H/V flip flags and a 4-bit palette bank.
- Fifteen BG palette banks are used for scene art; bank 15 is reserved for HUD glyphs.
- A 32×32 visible tile cache is streamed through BG1. The complete W1 L3 tile set is larger than VRAM, but any visible window fits the 1,024-tile cache.
- The converter supports an optional `--preview-out` PNG. It reconstructs pixels from the generated tile bytes, map, flip flags and BGR555 palette.

## Pixel-perfect conversion verification

| Scene | Unique tiles | Tile bytes | Round-trip differing pixels |
|---|---:|---:|---:|
| W1 L1 | 504 | 16,128 | 0 |
| W1 L2 | 971 | 31,072 | 0 |
| W1 L3 | 1,415 | 45,280 | 0 |
| Bonus Race | 154 | 4,928 | 0 |
| Shrewy | 389 | 12,448 | 0 |

The zero-difference result compares the generated 4bpp data to the source composite after BGR555 quantization. It verifies the conversion pipeline, not runtime rendering on hardware.

## Resolution tradeoff

The source game room coordinates are rendered at 1:1, but the GBA display is only 240×160. If the original game view is 480×320, a native-pixel GBA view shows half as much world in each dimension. It is not possible to preserve both the complete 480×320 field of view and every source pixel on a 240×160 display without a larger output resolution. v1.6 prioritizes source-pixel fidelity and uses a player-follow camera with native scene bounds.

## Remaining visual fidelity work

- Implement GBA affine OBJ transforms for rotated source instances; collision bounds already preserve source rotation in some bonus-room objects, but their rendered sprites are not yet rotated.
- Verify per-scanline OBJ limits and VBlank upload timing in mGBA and on hardware.
- Reconstruct live parallax rather than using baked composites.
- Match the original UI/HUD layout, font, title/menu and intro animation.
- Compare actual runtime screenshots against the supplied gameplay video and correct camera offsets, animation timing and object layering.


## Source-derived preview artifacts

- `previews/v16_volvy_sprite_before_after.png` — source `spr_idle` comparison: 2:1 downscaled output versus native 1:1 source pixels.
- `previews/v16_scene_before_after.png` — same source region compared at the old 2:1 field of view and the new 1:1 detail scale.
- `previews/v16_w1_l1_native_240x160.png` — native 240×160 World 1 L1 crop with source-placed actors.
- `previews/v16_bonus_native_240x160.png` — native bonus-room crop with source-placed actors.
- `previews/v16_native_sprite_sheet.png` — native-size original sprite samples.

These are source/data.win reconstructions. They are not screenshots from mGBA or physical GBA hardware.
