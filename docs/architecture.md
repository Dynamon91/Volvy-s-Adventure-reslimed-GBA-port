# Architecture — current prototype (v1.6)

## Coordinate domains

- Gameplay physics and collision remain in the existing 2:1 coordinate domain to keep the established movement, tile collision and actor bounds stable.
- The renderer now maps gameplay positions back to native source pixels (`source_x = gameplay_x * 2`) and displays original-size sprites at 1:1 on the 240×160 GBA screen.
- Source room instance tables store source coordinates; the World 1 runtime object tables store half-scale coordinates and the renderer converts them back to source pixels.
- This is a deliberate fidelity tradeoff: native source pixels preserve sprite and tile detail, but the GBA viewport covers half the source area of a 480×320 view. Camera framing is centered on the player and clamped to the native scene bounds.

## Background scene pipeline

`include/core/native_scene.h` defines `NativeSceneAsset`, `NativeSceneCell`, the 32×32 cache and upload descriptors.

- Five runtime scenes are generated as 4bpp tile data with per-tile palette-bank selection, H/V flip flags and 8×8 tile deduplication.
- The scene BG uses palette banks 0–14; bank 15 remains reserved for HUD glyphs.
- A visible 32×32 tile window is mapped into BG1 screen block 27. The tile cache can hold up to 1,024 unique 8×8 tiles in the combined BG character blocks 1+2 (32 KiB).
- The complete W1 L3 scene is 45,280 bytes, so the runtime streams visible tiles rather than attempting to copy the entire level into VRAM.
- `native_scene_cache_build()` preserves cached source tiles across camera movement and emits upload descriptors for new tiles. The renderer uploads at most 128 tiles (4 KiB) per VBlank, then commits the 32×32 map.
- A stage transition disables BG1 until its first native window is uploaded. Large cache changes are also hidden until their tile uploads and map are coherent.
- The five generated scene images round-trip from 4bpp tile/map/palette data with zero differing pixels after BGR555 quantization.
- Original background-sprite layers and tile layers are baked into each scene. Their original parallax rates are documented but not animated at runtime.

| Scene | Source dimensions | Tile map | Unique tiles | Tile bytes |
|---|---:|---:|---:|---:|
| W1 L1 | 2258×360 | 283×45 | 504 | 16,128 |
| W1 L2 | 2258×360 | 283×45 | 971 | 31,072 |
| W1 L3 | 2258×360 | 283×45 | 1,415 | 45,280 |
| Bonus Race | 5312×1120 | 664×140 | 154 | 4,928 |
| Shrewy | 256×144 | 32×18 | 389 | 12,448 |

## OBJ and animation pipeline

- The primary player/actor package contains 56 source sprite animation sets at 1:1 resolution and uses an 8bpp OBJ palette with 71 opaque colors.
- The largest player animation frame is 4,608 bytes. OBJ VRAM bytes 0–5,119 are reserved for the animated player.
- Twenty-five visible static actor animation sets use one current-frame slot each. The solid-green source `spr_hole` marker is excluded from rendering and the runtime cache. The slot allocator reserves 18,560 bytes beginning at OBJ VRAM offset 5,120; the complete player+static allocation ends at byte 23,680, leaving 9,088 bytes of the 32 KiB OBJ VRAM unused.
- Static animation frames are streamed into their fixed slots when their frame index changes. The VBlank uploader uses a 6 KiB budget for static actor animation updates.
- Native multi-part metasprites are supported for the player and static actors. OAM entries are generated per sprite part; the renderer hides additional actors when the 128-entry OAM limit is reached.
- The source-derived bonus-room OAM estimate is 66/128 entries including eight player parts. World 1 L1/L2/L3 estimates are 58/128, 66/128 and 82/128. These are conservative rectangle/part-count estimates, not a per-scanline 32-OBJ proof.
- Shrewy uses a separate boss OBJ layout and palette. Returning from the boss schedules static actor cache restoration in 4 KiB VBlank batches rather than copying the complete cache in one VBlank.

## Runtime modes

- `GAME_TITLE`, `GAME_PLAYING`, `GAME_BONUS`, `GAME_PAUSED`, `GAME_OVER`, `GAME_VICTORY` remain the high-level game states.
- World 1 stages 1–3 use native background assets and the expanded source-derived object tables.
- `GAME_BONUS` uses the source-derived bonus room, 397 onion placements, moving platforms, cloud platforms, bunny actors, bumpers and additional enemy/hazard/prop artwork.
- Shrewy arena uses the native 4bpp arena scene and its separate boss sprite package.

## Rendering limits and known gaps

- The native renderer is source-pixel faithful at the scene/sprite conversion boundary. Runtime enemy behavior is not yet source-faithful: hivefly, Crowly and Vinechomp are rendered/animated, but their original AI, attack timing and damage reactions are not implemented.
- Bumper collision bounds account for source rotation; runtime OAM affine rotation for the rendered rotated props is not yet implemented.
- Moving-platform paths and spring/bounce forces remain provisional where the source event logic has not been reconstructed.
- Live parallax, per-scanline OBJ-limit validation, original room transitions, the complete intro/menu, audio and save persistence remain unfinished.
- The standard `devkitARM`/`libgba` build has not been verified in this environment. The separate experimental Clang/LLD bare-metal build produces a structurally checked `.gba` image, but it has not been booted in mGBA or on hardware.

## v1.9 capture-driven visual cleanup

- The supplied 112.8-second mGBA capture contains a neon-green 16×16 square on a World 1 platform. It is not a background tile: its RGB color matches the extracted `spr_hole` asset (uniform palette index 36, BGR555 `0x33E6`).
- `obj_hole` and bonus `obj_hole`/`obj_hole_dir` are source placement markers. They remain in room/object tables but are not submitted to OAM. The `spr_hole` asset remains in the extraction manifest for source auditing, but its VRAM slot is no longer reserved.
- The active world static cache is 25 visible animation sets / 18,560 bytes, plus the 5,120-byte player reservation.
- This change is based on source-asset inspection and the capture; the new v1.9 ROM still requires an mGBA retest.
