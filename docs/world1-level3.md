# World 1 Level 3 — Bundtcake Burrows (v0.5)

## Original source extracted

`rm_w1_l3_00` comes from the supplied GameMaker `data.win`. Extraction is source-derived, not reverse-traced from a YouTube screenshot.

| Source resource | Count / size |
|---|---:|
| Room source dimensions | 2258x360 |
| Original instance records | 633 |
| Solids (`obj_solid`) | 78 |
| Onions | 318 |
| Destructible blocks | 157 |
| Action blocks | 15 |
| Springboards | 3 |
| TNT | 4 |
| Snakes | 3 |
| Underground moles | 4 |
| Auto-diggers | 24 |

The original room includes three burrow background images (`bg_burrow`, `bg_burrow_1`, `bg_burrow_2`), plus the visible tilesets `TileSet_burrow` and `TileSet_pipes`. They have been extracted and used for a static GBA scene. Live scrolling and sprite behavior are separate implementation tasks.

## GBA asset budget

| Component | Value |
|---|---:|
| Scene pixels | 1129x180 |
| Tilemap | 142x23 tiles |
| Unique 8bpp BG tiles | 473 |
| BG tile data | 30,272 bytes |
| Opaque BG colors | 33 |
| Source-derived coarse solid cells | 2605 |
| Source-derived coarse breakable cells | 100 |
| Loaded interaction records | 495 |
| Interaction capacity | 576 |

The 8bpp BG tile data fits 30,272 / 32,768 bytes in character blocks 1 and 2 (charblock 1 starts at VRAM + 0x4000). Screen blocks 27/28 are separate and hold the 64x32 scrolling map. No tile is deliberately removed to fit memory.

## Implemented in source

- Level 3 collision source table, with solid-region priority over breakable masks, from source object rectangles.
- 318 onion placements with pickup state, 157 breakable object placements and 15 action blocks.
- Three springboard placements, applying the existing provisional bounce mechanic.
- Source-derived right exit trigger and stage progression L1 -> L2 -> L3.
- BG1 scene and HUD W1-L3 selection. A **development-only** title-screen `Select` key cycles among 1/2/3 for direct stage tests; `Start` begins the selected level.
- The previous breakable cleanup is optimized so it revisits only newly destroyed objects, not every previously destroyed object on every hit.

## Not implemented / verification gaps

- The complete original game scripting is not available as plain GML. TNT, snakes, surface/underground moles, auto-diggers, holes, boulder spawner, water and special transitions are **not yet simulated**. Source placements and coordinates are preserved in `assets/converted/rooms/w1_l3_unimplemented.json` (60 instances).
- Collision geometry still uses source sprite AABBs rasterized onto an 8x8 grid; this does not reproduce original per-pixel collision masks.
- The source `obj_volvy` instance pivot is (49, 382), outside the nominal room height. The level currently uses the provisional GBA safe spawn (25,142), not a verified original script spawn.
- Stage 3 completion now enters the provisional native-resolution Shrewy arena (`GAME_BOSS`, stage ID 4). The boss state machine, damage windows and attack timings are still test behavior rather than a source-verified reproduction.
- Sixteen-bit GBA APU audio and music, intro, title animation, saves, extra worlds and physical-hardware performance testing remain pending.
- devkitARM/libgba toolchain and mGBA are not available in this environment. Host tests and Clang ARMv4T codegen are **not** a verified `.gba` build.

## Regenerate locally

```sh
python tools/extract_gamemaker_assets.py data.win --out private_assets
python tools/extract_gamemaker_rooms.py data.win \
  --asset-manifest private_assets/manifest.json \
  --assets-root private_assets --out private_rooms --rooms rm_w1_l3_00
python tools/build_room_scene_gba.py private_rooms/rm_w1_l3_00/tile_layers.png \
  --name rm_w1_l3_scene --scale 2 --out assets/converted/rooms
python tools/build_world_objects.py private_rooms/rm_w1_l3_00/room_data.json \
  --data-win data.win --out assets/converted/rooms
```
