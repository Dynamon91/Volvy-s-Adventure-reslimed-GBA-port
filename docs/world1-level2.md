# World 1 Level 2 (Fudge Forest) — source-derived v0.4

## Archive extraction

The user-supplied `data.win` GameMaker ROOM resource `rm_w1_l2_00` has:

- Room source size: 2258x360 pixels; GBA world at 2:1 is 1129x180 pixels.
- Two visible 16x16 source tile layers (forest tiles and pipes) plus eight BG/parallax layers.
- 432 placed GameMaker instances in the source ROOM record.
- From named actors: 180 onions, 107 destructible blocks, 34 solid regions,
  12 action blocks, 4 springboards, 3 squirrels, 1 right transition and 1 down transition.
- 308 runtime interaction records (the non-solid named objects). The `Level`
  actor arrays now reserve 384 entries, rather than the previous 160.
- Coarse collision from sprite AABBs: 1044 solid 8x8 cells, 157 breakable cells.

## GBA conversion

- Original BG/parallax and tile layers are currently flattened into one static scene.
- Output: `rm_w1_l2_scene.c`, 142x23 tilemap, 453 unique 8x8 8bpp tiles,
  39 opaque colors, and 28,992 bytes of tile graphics.
- This exceeds one 16 KiB BG character block. The renderer streams the source
  map window into BG1 and loads its tiles into contiguous BG char blocks 1+2
  (32 KiB capacity). BG screenblocks 27/28 and BG0 HUD occupy different memory.
- Original sprite animation arrays and actor sprites are reused from Level 1.

## Stage flow

From the title, Start enters W1 L1. Reaching its right exit shows STAGE CLEAR;
pressing Start from that state initializes W1 L2: new collision map, 308 actors,
new BG tile data and palette, stage label W1-L2, and a cleared player state.
Game over restarts the current stage. In v0.5, finishing W1 L2 advances to W1 L3
when Start is pressed; this is temporary until World 1 Level 3 is implemented.

## Confidence limitations

- The original `obj_volvy` in Level 2 also has source pivot `(64,384)`, below
  the room's nominal 360px height. Runtime spawn initialization is unknown;
  the prototype uses its tested provisional `(25,150)` GBA top-left.
- Actor placements and sprite sizes are extracted; collision precision and
  squirrel/springboard/action-block behavior remain approximate.
- Parallax layers are flattened. Original camera logic, scripts, cutscenes,
  music and runtime transitions have not been recovered.
- No actual devkitARM/libgba ROM build, mGBA boot, or hardware test was possible
  here. Host tests and ARMv4T/Thumb object code generation are not equivalent.

## Reproduction

```sh
python tools/extract_gamemaker_assets.py data.win --out private_assets
python tools/extract_gamemaker_rooms.py data.win \
  --asset-manifest private_assets/manifest.json \
  --assets-root private_assets --out private_rooms \
  --rooms rm_w1_l1_00 rm_w1_l2_00
python tools/build_world_objects.py private_rooms/rm_w1_l2_00/room_data.json \
  --data-win data.win --out assets/converted/rooms
python tools/build_room_scene_gba.py private_rooms/rm_w1_l2_00/tile_layers.png \
  --name rm_w1_l2_scene --scale 2 --out assets/converted/rooms
```

Extracted PNGs remain private; generated original-game-derived C arrays are
included in the local development ZIP but ignored in public Git by default.
