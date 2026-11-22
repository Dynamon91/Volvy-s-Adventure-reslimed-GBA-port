# Original GameMaker asset extraction

## Results

The supplied `data.win` was parsed using a small, MIT-licensed subset of `gm-extract` (NazarHK, 2026); its license is included under `tools/vendor/gm_extract/LICENSE`.

- GameMaker resource: `Volvys_Adventure_6`
- Sprite resources parsed: 726
- Selected sprite resources extracted: 86 (including player/world actors, burrow hazards and Shrewy)
- Background tilesets parsed/extracted: 18
- Platform player/Swolvy animation sets: 29
- Platform world-actor sprite sets: 8
- Separate native-resolution boss/hazard sprite sets generated: 58
- Extraction errors: 0

The private extracted PNGs are kept outside the distributable project archive. They are not committed by default because they are original game assets.

## Confirmed sprite dimensions

Selected original source canvases include:

| Animation | Source dimensions | Frames |
|---|---:|---:|
| `spr_idle` | 24x19 | 4 |
| `spr_walk` | 24x21 | 4 |
| `spr_jump` | 24x24 | 4 |
| `spr_float` | 40x32 | 4 |
| `spr_dash` | 47x28 | 2 |
| `spr_dash_up` | 28x47 | 2 |
| `spr_grind` | 32x40 | 9 |
| `spr_ground_pound` | 20x39 | 8 |
| `spr_punch` | 55x24 | 10 |
| `spr_swolvy_idle` | 44x26 | 4 |
| `spr_swolvy_walk` | 32x28 | 4 |
| `spr_swolvy_transform` | 46x36 | 11 |

All 18 original tilesets use 16x16 source tiles. They include beach, boardwalk, bonus, burrow, factory, forest, office, park, pipes, roof, sewer, shipping, subway and underground variants.

## GBA conversion

`tools/prepare_player_assets.py` performs:

1. 2:1 nearest-neighbor downscaling.
2. Shared 8bpp OBJ palette construction.
3. 8x8 tile packing in GBA 8bpp format.
4. Splitting frames wider/taller than one legal GBA OBJ into multiple OAM parts.
5. C array/header generation for the player animation set.

The earlier 37-set platform player/world-actor package used 29 opaque colors plus transparent index 0, so the shared OBJ palette preserves the sampled source colors without quantization. Original GameMaker sprite origins are scaled and stored per frame; the renderer now aligns animation frames around that pivot instead of centering their bounding boxes.

The 18 BG tilesets were separately converted to deduplicated 8bpp tiles. Each tileset has at most 219 unique 8x8 tiles (within the 256-tile capacity of one 16 KiB GBA 8bpp character block). Combined tile data across all 18 sets is 108,480 bytes; only the active world's tileset is intended to be loaded at a time.

## Reproduction

First extract the private PNGs from the user's `data.win`:

```sh
python tools/extract_gamemaker_assets.py data.win --out /path/to/private_assets
```

Then generate GBA C assets:

```sh
python tools/prepare_player_assets.py \
  /path/to/private_assets/manifest.json \
  --assets-root /path/to/private_assets \
  --out assets/converted \
  --scale 2
```

Do not commit the private PNG extraction directory to a public repository without permission to redistribute the original assets.


## World 1 Level 1 room reconstruction

The GameMaker `ROOM` chunk was parsed for `rm_w1_l1_00`.

- Room metadata size: 2258x360 source pixels.
- Visible tile layers: `Tiles_1` (TileSet1, depth 700) and `Pipes` (TileSet_pipes, depth 600).
- Background layers: shrubs, light/dark hills, light/dark clouds and blue sky.
- Tilemap grid: 142x23 source cells at 16x16 each.
- Tilemap stream: GameMaker 2024.2+ RLE-compressed data, decoded into source tile indices.
- The room contains 259 placed instances, including 84 onions, 63 plants, 42 destructible blocks, 38 solid objects, 9 action blocks, 3 squirrels, 2 springboards and transition/hole objects.
- The `obj_volvy` instance is recorded at (64,384). This lies below the room's nominal 360px height; its runtime spawn/camera interpretation remains unverified and must not be normalized by assumption.

The visible tile layers were merged into a single 8bpp GBA room asset:

- 50 unique 8x8 GBA tiles.
- 26 opaque colors plus transparent index 0.
- 3,200 bytes of tile graphics.
- 142x23 map entries.

This is comfortably below the 256-tile capacity of one 16 KiB 8bpp BG character block. The 50-tile room-layer C data is included separately. The flattened 209-tile scene is loaded by the BG1 renderer and streamed through a 64×32 map window. The original World 1 object AABBs are also converted to a collision grid and consumed by the platform controller. Collision remains an AABB approximation, not a pixel-perfect original mask.


## Rebuilding the converted room data

```sh
python tools/prepare_tilesets.py private_assets/manifest.json \
  --assets-root private_assets --out assets/converted/tilesets --scale 2

python tools/extract_gamemaker_rooms.py data.win \
  --asset-manifest private_assets/manifest.json \
  --assets-root private_assets --out private_rooms --rooms rm_w1_l1_00

python tools/build_room_gba.py private_rooms/rm_w1_l1_00/room_data.json \
  --asset-manifest private_assets/manifest.json \
  --assets-root private_assets --out assets/converted/rooms --scale 2
```


The six visible background/parallax layers and two visible tile layers were also flattened into a static 2:1 scene image and tiled for the GBA renderer:

- 1129x180 GBA-world pixels.
- 142x23 8x8 tilemap.
- 209 unique 8bpp tiles (13,376 bytes).
- 33 opaque colors plus transparent index 0.

This fits one 16 KiB 8bpp BG character block. The renderer now streams a 64x32 map window from this scene. The source parallax speeds are retained in the room metadata, but the current scene is a static composite and does not yet reproduce live parallax motion.

## Shrewy arena and native-resolution boss assets (v0.6)

The v0.6 selected source set included 86 sprite resources plus the 18 original tilesets. At that milestone, the player/world conversion script filtered 37 animation sets used by the platform controller (29 Volvy/Swolvy sets and 8 world actor sets). The separate `prepare_boss_sprites.py` converter emits 58 native-resolution boss/actor sprite sets with an 81-color 8bpp OBJ palette.

Recovered boss resources:

- Dedicated room `rm_shrewy`, 256×144 source pixels, 24 placed instances.
- `obj_boss_shrewy` at `(184,104)`, idle sprite 48×48 with origin `(25,39)`.
- `obj_boss_shrewy_boulder` object and `spr_shrewy_rock` sprite.
- Shrewy animation sets: idle, windup, jump, stomp, dropkick, tornado, tornado slam, hurt and rock.
- Volvy source room pivot `(64,160)`, outside the room's 144px height. Runtime relocation remains unresolved.
- Four onions, three solid objects, one one-way platform, one cage, two autoholes and four cone emitters.

The boss room's static scene converts to 417 unique 8×8 8bpp tiles / 26,688 bytes at 1:1 scale, fitting the combined 32 KiB BG character blocks 1+2. `build_boss_room.py` exports a 32×18 AABB-derived collision grid and prop records. The derived one-way platform top surface rounds to y=112. This is a coarse collision approximation, not a source collision mask.

Rebuild commands:

```sh
python tools/prepare_boss_sprites.py private_assets/manifest.json \
  --assets-root private_assets --out assets/converted --scale 1
python tools/extract_gamemaker_rooms.py data.win \
  --asset-manifest private_assets/manifest.json \
  --assets-root private_assets --out private_rooms --rooms rm_shrewy
python tools/build_room_scene_gba.py private_rooms/rm_shrewy/tile_layers.png \
  --name shrewy_scene --scale 1 --out assets/converted/rooms
python tools/build_boss_room.py private_rooms/rm_shrewy/room_data.json \
  --data-win data.win --out assets/converted/rooms --scale 1
```

The boss-room spawn relocation and boss event scripts are not decoded. Boss attack timing, health, damage windows and projectile behavior are provisional implementation values.


## Shrewy cage-drop sequence (v0.8)

`tools/extract_gamemaker_sequences.py` reads the versioned `SEQN` pointer table and nested sequence tracks/keyframes. For the supplied GameMaker 2026.0.0.23 archive it exports `seq_cage_drop`, including the `obj_cage.position.y` embedded Bezier curve. `tools/build_shrewy_cage_sequence.py` samples the 60-frame timeline into 20.12 fixed-point values and writes `shrewy_cage_sequence.c/.h` plus a JSON manifest. The sequence's trigger callsite and room-origin offset remain unverified; the helper is not wired to boss defeat.

```sh
python tools/extract_gamemaker_sequences.py data.win \
  --only seq_cage_drop --out assets/converted/rooms/shrewy_cage_sequence.json
python tools/build_shrewy_cage_sequence.py assets/converted/rooms/shrewy_cage_sequence.json \
  --out assets/converted/rooms
```


## Bonus race collision grid

`tools/build_bonus_collision.py` reads `rm_w1_b1_00/room_data.json` and the original `data.win`. It resolves source object sprite dimensions/origins from the GameMaker `OBJT`/`SPRT` resources, applies per-instance scale and rotation, then rasterizes the transformed AABBs into a 332×70 grid at 8 GBA pixels per cell.

```sh
python tools/build_bonus_collision.py \
  private_rooms/rm_w1_b1_00/room_data.json data.win \
  --out assets/converted/rooms
```

The resulting `bonus_w1_collision.c/.h` grid contains solid, breakable, rail, hazard, one-way and breakable-one-way cells. The player controller only lands on one-way cells while descending. Moving platforms are represented by their initial AABB only. The grid is an AABB-derived approximation, not a pixel-perfect mask; original collision scripts and masks have not been recovered.


## Full bonus-room static composite

The room extractor's `tile_layers.png` includes both visible `TileSet_bonus` layers and all visible background-sprite layers, rendered in GameMaker depth order. The runtime uses a static 2:1 composite so the original sky/cloud/trim/star artwork is not omitted:

```sh
python tools/build_bonus_composite_gba.py \
  private_rooms/rm_w1_b1_00/tile_layers.png \
  --name bonus_w1_composite --scale 2 --out assets/converted/rooms
```

The current source image converts to a 332×70 GBA map, 486 unique 8×8 8bpp tiles (31,104 bytes), and 24 opaque colors. It fits the combined 32 KiB BG charblock 1+2 region. The original layer parallax rates are retained in the room metadata but are not animated by this flattened composite.

## v1.2 original bunny and bumper source sprite conversion

The GameMaker extractor's optional `--only-sprites` argument supports focused extraction. The full selected sprite set now includes 90 original resources (previous 86 + bunny, large Swolvy block, square bumper, corner bumper); `prepare_player_assets.py` selects 41 player/world/bonus sets and builds a shared 32-opaque-color 8bpp OBJ palette. The boss-mode sprite converter remains independent and uses its original native-resolution palette. The private source PNGs remain outside this project ZIP.

```sh
python tools/extract_gamemaker_assets.py data.win --out private_assets --no-tilesets
python tools/prepare_player_assets.py private_assets/manifest.json \
  --assets-root private_assets --out assets/converted --scale 2
python tools/analyze_bonus_oam_density.py private_rooms/rm_w1_b1_00/room_data.json \
  assets/converted/player_sprites_manifest.json \
  --out assets/converted/rooms/bonus_w1_oam_density_v12.json
python tools/preview_bonus_runtime_assets.py \
  --room-json private_rooms/rm_w1_b1_00/room_data.json \
  --asset-manifest private_assets/manifest.json --assets-root private_assets \
  --out previews
```
