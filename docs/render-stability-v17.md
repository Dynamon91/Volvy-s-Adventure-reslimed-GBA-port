# v1.7 native GBA rendering stability and font fidelity

## Source of the issue

In v1.6, `scene_stream_update()` assigned `s_scene_start_x/y` to the requested upcoming 32×32 map window before the tile uploads and map DMA had completed. Meanwhile `gba_display_set_camera()` immediately subtracted those new offsets from the current camera. When scrolling across an 8-pixel tile boundary the background scroll could therefore reference a tile window which had not reached BG1 VRAM yet. This could cause visual jumps or wrong tiles even though the original scene data was pixel-identical to the source.

## v1.7 correction

`NativeScenePresentation` separates the visible/committed BG map origin from the requested/staged origin. The renderer continues to calculate actor positions and scroll relative to the currently visible 256×256px map until the pending data has finished uploading. The visible origin is updated only after the 32×32 BG1 screen map DMA completes during VBlank.

During normal incremental scrolling, the presentation camera stays in the active window's legal 0–16px horizontal and 0–96px vertical scroll range. During hard teleports the BG is temporarily disabled while the new map is staged; this includes the edge case where all required new tiles are cached and no tile uploads happen. BG1 is re-enabled when the camera and map origin can be presented coherently.

## Boss sprite memory fix

v1.6 loaded boss-room onions at OBJ tile32 index 128 (4,096 bytes), inside the player's 5,120-byte frame reservation. v1.7 moves onions to tile32 index 192 (6,144 bytes). Four source onion frames occupy 1,024 bytes, ending at byte 7,168. The boss body starts at byte 8,192, the rock starts at byte 24,576, and all occupy disjoint ranges. The 8bpp OBJ attr2 addressing and actual padded frame-size limits are regression tested.

## Original GameMaker font conversion

The new HUD font is extracted from the original `FONT` resource named `fnt_small` (font index 9). The extractor follows the original `FONT` glyph pointer table, `TPAG` texture entry and decoded `TXTR` page. The original opaque glyph pixels are placed into 8×8 4bpp tiles at native 1:1 scale with transparent palette index 0 and white pixel index 1. The 39 required glyphs fit without cropping, antialias loss or pixel resampling.

This is an original font resource from `data.win`, but the original game's exact HUD font selection is not yet established from the missing GML callsites. v1.7 chooses `fnt_small` as a fidelity-focused replacement for the temporary hand-coded 5×5 font.

To rebuild the private original-derived font assets:

```sh
python3 tools/build_hud_font_gba.py /path/to/data.win --out assets/converted \
  --preview previews/v17_source_font.png
```

The generated `hud_source_font.c/.h` and other copyrighted source-derived artwork stay in the private project archive; do not commit them to a public GitHub repository.

## Verification and remaining risks

`sh tests/run_host_tests.sh` checks the full existing gameplay suite, camera presentation states, source font tiles and boss sprite VRAM bounds. `sh tests/run_arm_codegen.sh` checks ARMv4T/Thumb code generation with **test-only headers**. `sh tests/run_experimental_clang.sh` builds and structurally verifies an experimental bare-metal `.gba` image.

These checks do not prove mGBA boot or runtime correctness of v1.7. Hardware performance, full VBlank budgets, per-scanline OBJ limits, actual original intro/HUD layout, live parallax and enemy AI still require additional work and emulator screenshots.
