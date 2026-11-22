# World 1 Level 1 — source-driven gameplay milestone

This milestone connects the extracted `rm_w1_l1_00` room to the GBA engine. The scene is **visually integrated** and World 1 collision and object data now drive the player engine. It remains a prototype until a real devkitARM build and mGBA test succeed.

## Source-derived room data

- Original room metadata: 2258×360 GameMaker pixels.
- 2:1 nearest-neighbor conversion: 1129×180 GBA-world pixels.
- Static composite of six original background layers and two visible tile layers: 209 unique 8bpp tiles, 33 opaque colors.
- World solid collision: 38 instances of `obj_solid`, rasterized using the object's `spr_solid` 16×16 source sprite, 8×8 pivot and each instance's recorded scale. This yields 1,011 solid 8×8 collision cells.
- Breakable collision: 42 `obj_destructible_block` instances, rasterized to 49 8×8 cells due to object overlaps with the collision grid. Tiles can be removed by dash, ground-pound, ground dig and simple punch tests.
- Other placed objects: 84 `obj_onion`, 9 `obj_action_block`, 3 `obj_squirrel`, 2 `obj_springboard`, and 4 world transitions (two right, one up, one down). A compact list of 144 non-solid world actors is generated. All counts come from the extracted room's instance list.
- Actual GameMaker sprite dimensions and pivots are read from the user's `data.win` `OBJT`/`SPRT` records by `tools/build_world_objects.py`, rather than being inferred from a screenshot.

## What is connected

- Player moves through the extracted solid rectangles instead of the temporary physics room.
- Camera tracks the source-converted World 1 width with horizontal BG1 map streaming.
- Player pickup AABB detects source-derived onion positions, consumes them once, and increments the HUD.
- Breaking a source block clears all non-shared tiles covered by its footprint; when two source blocks overlap a coarse 8×8 collision tile, that tile stays solid until both blocks are removed. The actor sprite disappears with its owning object.
- The two rightward transition instances act as a **provisional level-clear trigger**; original routing scripts are not yet translated.
- Original Volvy/Swolvy sprite frames are loaded as 8bpp OBJ graphics. Five active actor types (`spr_onion`, `spr_destructible_block`, `spr_action_block`, `spr_squirrel_walk` / `spr_squirrel_idle`, `spr_springboard`) are shown in OAM. The new renderer preloads original multi-frame squirrel, springboard, and action-block/spent sprite animations so the state of individual instances can change without repeated OBJ VRAM uploads.
- Transparent BG0 tile-font HUD displays stage, onion counter, title, pause and clear/game-over messages.
- 8bpp OBJ indices are measured in 32-byte units; the player renderer now uses `tile_index * 2` for 64-byte logical tiles. Static sprites use aligned even OBJ indices.

## Unverified / provisional behavior

- The source `obj_volvy` pivot is `(64,384)` while the room metadata height is `360`. In this prototype the safe test spawn uses source pivot `(64,312)` (player collision bottom at source y=320, first solid platform). This is **not** a claim about the original game spawn script.
- Swolvy's five-onion threshold is a **temporary test rule** and not established by the supplied traces or GameMaker source scripts.
- Solid masks are currently the axis-aligned bounding rectangles of source `obj_solid` instances, rasterized to 8×8 cells. Pixel-accurate collision and any script-modified masks are not verified.
- Springboards now launch a descending player, with a 16-frame instance cooldown. Their launch speed (-4.5 GBA px/frame) and triggered animation cadence are **provisional**, not verified against GameMaker scripts.
- Squirrels now patrol within 12 GBA pixels of their extracted placements, moving 1 pixel every 4 frames and reversing at ledges, solid walls, and patrol limits. The exact AI/animation timings have not been recovered.
- Stomping a squirrel or landing a hand attack defeats it once; direct side contact currently sets the prototype's game-over flag. Stomp rebound is -2.0 GBA px/frame, also provisional.
- Action blocks have a provisional attack-triggered upward rebound (-2.5 GBA px/frame), a 24-frame cooldown and a distinct extracted spent-frame animation. None of these values is confirmed as original behavior.
- The original hit points, hurt/invulnerability recovery, enemy pathfinding and source behavior scripts remain unverified.
- All six source parallax layers are baked into a single BG1 static composite rather than independently scrolling.
- World 1 Level 1 original HUD/art presentation, intro, sound effects, level transitions and save data are not reconstructed.
- Real GBA compilation, mGBA boot, audio playback and hardware validation remain **unverified**.

## Run unit tests

```sh
sh tests/run_host_tests.sh
```

The script uses a small **test-only** `tests/support/gba.h` stub for host syntax and gameplay logic checks. It never emulates GBA hardware or proves hardware compatibility.

## Reproduce the room data

```sh
python tools/extract_gamemaker_assets.py data.win --out private_assets
python tools/extract_gamemaker_rooms.py data.win \
  --asset-manifest private_assets/manifest.json \
  --assets-root private_assets --out private_rooms \
  --rooms rm_w1_l1_00
python tools/build_world_objects.py private_rooms/rm_w1_l1_00/room_data.json \
  --data-win data.win --out assets/converted/rooms
```

Generated private assets in this ZIP are supplied for the user's local build and should not be redistributed publicly without the rights to the underlying game resources.
