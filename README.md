# Volvy's Adventure: Reslimed — GBA homebrew adaptation

**Current milestone: World 1 Levels 1–3, a provisional native-resolution Shrewy boss encounter, extracted cage-drop playback helper, and a source-derived World 1 bonus/race mode prototype (v1.3). This is an engine/source integration prototype, not a completed or hardware-verified port.**

The project targets ARM7TDMI Game Boy Advance hardware using C, devkitARM/libgba, 20.12 fixed-point gameplay math, Mode 0 tiled backgrounds, hardware OBJ sprites, and VBlank-synchronized VRAM/OAM updates. The original user-supplied GameMaker `data.win` was parsed privately; it is not included in this project archive.

## Current implementation

### Original graphics and scenes

- 29 original Volvy/Swolvy animation sets and 8 world-actor sets are converted to 8bpp OBJ graphics at 2:1 scale. The updated 41-set player/world/bonus package shares 32 opaque colors plus transparent index 0.
- A separate native-resolution boss/actor package contains 58 sprite sets and an 81-color 8bpp OBJ palette. It includes Volvy, Swolvy, Shrewy, Shrewy's rock, onions, TNT, snakes, mole animations, holes, emitter art, and cage art.
- W1 L1 `rm_w1_l1_00`: 1129×180 GBA-world pixels, 209 unique 8×8 BG tiles, 13,376 bytes.
- W1 L2 `rm_w1_l2_00`: 1129×180 pixels, 453 BG tiles, 28,992 bytes.
- W1 L3 `rm_w1_l3_00`: 1129×180 pixels, 473 BG tiles, 30,272 bytes.
- Dedicated Shrewy arena `rm_shrewy`: native 256×144 pixels, 32×18 tile map, 417 BG tiles, 26,688 bytes.
- The three scrolling platform levels use a 64×32 streamed BG1 map. The Shrewy arena uses the same map window with its 32×18 scene placed at the left.
- Source background/parallax layers are currently baked into static scene composites. Live parallax is not yet implemented.

### World 1 gameplay

- Source-derived collision grids and placed interaction objects for W1 L1, L2 and L3.
- Fixed-point movement, jump buffering, coyote time, Big Hand hover/dash/grind/ground-pound/punch/dig, onion pickup, provisional Swolvy transformation, springboard bounce, squirrel patrol/stomp, action-block rebound and destructible blocks.
- Stage routing: W1 L1 → W1 L2 → W1 L3 → Shrewy arena. The title-screen `Select` key cycles W1 L1/L2/L3/Bonus Race for development testing; this is not an original-game menu feature.
- The W1 L3 mole, snake, TNT, boulder-spawner, hivefly and auto-digger placements are preserved as source metadata, but their gameplay is not yet reconstructed.

### World 1 bonus/race mode (v1.3)

- `GAME_BONUS` loads `rm_w1_b1_00` with a 332×70 GBA scene map, two-axis 64×32 BG streaming and a 332×70 AABB-derived collision grid.
- The original `obj_volvy` placement is not used as the runtime spawn; the initial spawn is based on the first source trace frame `(264.3,535.0)` at 2:1 scale.
- Both original race-start gate AABBs activate a local 60 Hz frame timer. The finish uses the horizontal marker plane, consistent with the recorded race event near the finish marker.
- All 397 source onions, 42 action blocks, 7 springboards, standard breakable blocks, six squirrels, 11 bunny placements, six square bumpers, four triangular bumpers and both Swolvy-only blocks are rendered from original converted sprites. Two `obj_bunny` actors now use separate source-anchored movement/hopping prototypes, while nine `obj_bunny_ground` instances remain static. Bumper contacts now give provisional impulses using source-rotation-aware bounding boxes; the original game logic is not reconstructed. Action blocks rebound on Big Hand contact; springboards bounce on descending contact. One-way cloud platforms and breakable one-way blocks are supported. Moving platforms are static.
- Authentic bunny competitor AI, the original bumper impulses, official race timer/rewards, dynamic platform paths, live parallax and room entry/exit callsites are not yet reconstructed. The provisional 5-onion transformation has been disabled for the bonus room because the supplied traces record collected onions but no Swolvy frames. The two larger Swolvy-only blocks now have dedicated graphics and collision tiles that normal Volvy cannot break.

### Shrewy boss encounter (new in v0.6)

- Extracted the dedicated source room `rm_shrewy`, its room scene, source object placements, collision bounds, and original sprite animations.
- Source boss pivot: `(184,104)`. Original `spr_shrewy_idle`: 48×48 pixels, origin `(25,39)`.
- Original Volvy room pivot: `(64,160)`, which lies below the 144px room. The source room-create relocation has not been decoded; the prototype uses a provisional floor-aligned spawn at top-left `(50,93)`.
- Boss arena collision is a 32×18 grid derived from three `obj_solid` AABBs and the top surface of one `obj_owp` platform. The derived floor row begins at y=112; this is not an original pixel-perfect collision mask.
- A native-resolution player controller uses 24×19 normal and 44×26 Swolvy hitboxes, with physics constants scaled to native pixels. The normal 2:1 platform stages retain their 12×10 / 22×13 hitboxes.
- `GAME_BOSS` is connected to stage progression. The player can jump and use Big Hand attacks; Shrewy has idle, wind-up, jump, stomp, dropkick, tornado, hurt, recovery and defeated states. The tornado prototype launches the original Shrewy-rock sprite. Boss HP is currently a three-hit test value.
- Four source onions are collectible in the boss room. The boss HUD displays the provisional HP count.
- All 15 Shrewy prop placements are now preserved: four onions, one cage, two autoholes, four adjustable autodigger markers and four cone emitters. The exporter stores instance IDs, pivot positions, Q8.8 scales, rotations and rotated sprite AABBs. These transforms are source-derived; hazard behavior remains unimplemented.
- A source audit found 5,510 script resource names and 65 Shrewy-related names, but no `CODE` chunk in `data.win`; original GML event logic has not been decompiled.
- Extracted the native `SEQN` resource `seq_cage_drop`: 60 fps, 60-frame sequence with an `obj_cage.position.y` Bezier curve; baked 60 samples into 20.12 fixed-point data. The sequence origin/trigger callsite is not yet known, so the curve is not yet connected to a gameplay event.
- Boss attack timing, damage windows, rock trajectory, health, player damage rules and exact spawn relocation remain provisional. The original GameMaker event bytecode/scripts have not been decompiled or behavior-matched.
- The Shrewy cage-drop Bezier now has a deterministic 60 Hz fixed-point sequence player. It outputs the extracted offset samples but is not auto-triggered; the sequence origin and boss callsite remain unresolved.

### World 1 race/bonus room source data (v0.9) — runtime added in v1.0

- Re-extracted `rm_w1_b1_00` at 5312×1120 source pixels (2656×560 GBA world pixels). Source placements include two race-start gates, one finish, a podium, seven crowd actors and bunny competitors, confirming a race-style bonus challenge distinct from the Shrewy arena.
- Converted the two visible `TileSet_bonus` layers into a 332×70 GBA map with 139 unique 8bpp tiles (8,896 bytes) and 22 opaque colors. Source tile rotate/mirror/flip flags are baked into the tile art.
- Preserved all 1,024 source instances across 46 object kinds, including 397 onions, 193 destructible blocks, 166 solid objects, nine rails and seven springboards.
- Baked all four visible background-sprite layers and both tile layers into a static 332×70 scene composite (486 unique 8bpp tiles / 31,104 bytes, 24 opaque colors). Source parallax speeds remain in metadata; live parallax is pending. The race renderer and controller were integrated in v1.0 and expanded with object interactions in v1.1.
- Analyzed four trace clips containing 2,983 player frames and 478 interaction events, including a race event near the source finish. The clips' `time_ms` ranges overlap, so no full-race time is claimed. The traces do not record bunny AI positions.
- Added a full-room route preview and four 240×160 source-trace camera crops. These are asset/trace composites, not emulator captures.

## Controls in the prototype

| Input | Action |
|---|---|
| D-pad Left / Right | Move |
| A | Jump |
| B + Left / Right | Big Hand dash |
| Hold B in air | Hover |
| B + Up in air | Vertical dash |
| B + Down in air | Ground-pound |
| B + Up on ground | Upward punch |
| B + Down on ground | Dig |
| B on ground | Forward punch |
| Hold R at a rail | Grind |
| Start | Start / pause / resume / restart |
| Select (title only) | Cycle W1 L1, L2, L3 and Bonus Race for development testing |

In the Shrewy arena, attack during the boss's idle/recovery windows. The exact source timing and damage rules are not yet verified.

## GBA build and emulator verification

Install the devkitPro **GBA** group, including `devkitARM`, `libgba`, and `gba_rules`, then run:

```sh
export DEVKITPRO=/opt/devkitpro
export DEVKITARM="$DEVKITPRO/devkitARM"
make clean
make
```

The expected output is `volvy_reslimed.gba`.

**Current environment status:** no devkitARM/libgba SDK or mGBA executable is installed here. No `.gba` ROM has been produced, and no emulator or physical-hardware boot is claimed. Clang has emitted and relocatably linked 32 ARMv4T/Thumb translation units using test-only GBA headers; that is CPU code-generation validation, not a real SDK link or bootable ROM.

## Reproducible host checks

```sh
sh tests/run_host_tests.sh
sh tests/run_arm_codegen.sh    # ARMv4T code generation with test-only headers; not a ROM
```

The host suite covers GBA map-window streaming at all tested camera positions, fixed-point math, player movement, World 1 stages, collision/object interactions, springboards, squirrels, Shrewy room collision and source-prop transforms, native-resolution player sizing, boss attack-state progression, damage windows, rock projectile movement, bonus race gates/timer/onion pickup/one-way platforms, scene budgets and C syntax. These tests do not replace a devkitARM build or mGBA run.

## Rebuild assets from the private original `data.win`

Requires Python 3 and Pillow:

```sh
python -m pip install Pillow
python tools/extract_gamemaker_assets.py data.win --out private_assets

python tools/prepare_player_assets.py private_assets/manifest.json \
  --assets-root private_assets --out assets/converted --scale 2
python tools/prepare_boss_sprites.py private_assets/manifest.json \
  --assets-root private_assets --out assets/converted --scale 1

python tools/extract_gamemaker_rooms.py data.win \
  --asset-manifest private_assets/manifest.json \
  --assets-root private_assets --out private_rooms \
  --rooms rm_w1_l1_00 rm_w1_l2_00 rm_w1_l3_00 rm_shrewy

python tools/build_room_scene_gba.py private_rooms/rm_shrewy/tile_layers.png \
  --name shrewy_scene --scale 1 --out assets/converted/rooms
python tools/build_boss_room.py private_rooms/rm_shrewy/room_data.json \
  --data-win data.win --out assets/converted/rooms --scale 1
# Optional: --trace-dir points to CSVs extracted from the supplied private archive.
python tools/audit_shrewy_source.py data.win private_rooms/rm_shrewy/room_data.json \
  --trace-dir private_traces --out assets/converted/rooms/shrewy_source_audit.json
python tools/extract_gamemaker_sequences.py data.win \
  --only seq_cage_drop --out assets/converted/rooms/shrewy_cage_sequence.json
python tools/build_shrewy_cage_sequence.py assets/converted/rooms/shrewy_cage_sequence.json \
  --out assets/converted/rooms
```

For the World 1 race/bonus room, after extracting the room and original source tileset:

```sh
python tools/build_bonus_room_gba.py private_rooms/rm_w1_b1_00/room_data.json \
  private_assets/tilesets/TileSet_bonus.png --out assets/converted/rooms
python tools/build_bonus_composite_gba.py private_rooms/rm_w1_b1_00/tile_layers.png \
  --name bonus_w1_composite --scale 2 --out assets/converted/rooms
python tools/build_bonus_collision.py private_rooms/rm_w1_b1_00/room_data.json data.win \
  --out assets/converted/rooms
python tools/analyze_bonus_room_trace.py private_rooms/rm_w1_b1_00/room_data.json \
  private_traces/shadow_rm_w1_b1_00.json private_traces \
  --out assets/converted/rooms/bonus_w1_trace_summary.json
python tools/build_bonus_room_preview.py private_rooms/rm_w1_b1_00/tile_layers.png \
  private_rooms/rm_w1_b1_00/room_data.json private_traces/shadow_rm_w1_b1_00.json \
  private_traces private_assets/manifest.json private_assets --out previews
```

The v1.3 source rotation-aware bumper exporter is:

```sh
python tools/build_bonus_bumper_bounds.py \
  private_rooms/rm_w1_b1_00/room_data.json --out assets/converted/rooms
```

The bonus collision exporter is `tools/build_bonus_collision.py`; it converts source sprite AABBs into solid, breakable, Swolvy-only breakable, rail, spike, one-way and breakable-one-way cells. It does not reproduce pixel masks or dynamic platform paths. The world-scene and world-object build commands are documented in `docs/asset-extraction.md`, `docs/world1-level2.md`, `docs/world1-level3.md` and `docs/bonus-room.md`. The original commercial archive and extracted private PNGs are not bundled. Generated C arrays are ignored by Git by default to avoid accidental public redistribution; the local working ZIP includes the generated C assets for convenience.

## Memory conventions

- BG char blocks 1+2: up to 32 KiB of 8bpp scene tiles. W1 L3 uses 30,272 bytes; Shrewy uses 26,688 bytes; Bonus Race uses 31,104 bytes (486 tiles); the separate tile-only source conversion uses 8,896 bytes.
- BG1 screen blocks 27–28: 64×32 streamed map window, now streamed horizontally and vertically for the 70-tile-high bonus room.
- Level state: 27,352 bytes on the host ABI, including the 332×70 collision backing grid and 1,024 actor/removal slots.
- BG0 screen block 29: 4bpp HUD glyph overlay in character block 0.
- World-mode OBJ: player frame region at OBJ VRAM offset 0; static actor cache begins at 2 KiB and contains 7,360 bytes of 8bpp artwork (plus 2 KiB player reservation).
- Boss-mode OBJ: native player frames at offset 0; boss onion frames at 4 KiB; Shrewy frames at 8 KiB; rock frame at 24 KiB. The world actor cache overlaps the boss onion window and is rebuilt when returning to a platform stage.
- All BG-map, OBJ-frame and OAM commits are synchronized to VBlank.

## Remaining work and limitations

- Decode and reproduce the original GameMaker scripts, pixel collision masks, boss behavior, spawn relocation, enemy AI and hazard logic.
- Refine the `rm_w1_b1_00` race/bonus prototype: recover original controller and entry/exit flow, validate/reconstruct authentic bunny AI, official timer/reward rules, moving-platform paths and remaining hazard behavior.
- Implement authentic title/intro flow, saves, sound/music and live parallax.
- Run a real devkitARM/libgba build, boot in mGBA, inspect VRAM/OAM and test timing on hardware.
- Confirm original sprite scaling, onion threshold, boss HP, attack timings, hit rules and stage transitions against source scripts or captured gameplay.

New v1.3 movement diagnostics: `previews/bonus_bunny_movement_comparison_v13.png` and `previews/bonus_bunny_motion_trace_v13.csv`, derived from original graphics and a host-compiled prototype motion trace, **not emulator screenshots**. Earlier source-art previews include `previews/bonus_bunny_source_v12.png`, `previews/bonus_track_source_v12.png`, `previews/bonus_swolvy_block_v12.png` and `previews/bonus_original_sprite_reference_v12.png`. These are **asset reconstructions, not emulator screenshots**. The original v1.2 OAM-density audit covers 969,217 integer camera locations and finds 101 estimated entries (including eight reserved player parts) for static placements. Moving the two active bunnies can increase a camera view by at most two entries, giving a conservative **103/128 OAM capacity bound** for the v1.3 prototype. Neither result tests per-scanline OBJ cycles or an emulator.

See `docs/shrewy-boss.md`, `docs/world1-gameplay.md`, `docs/architecture.md`, `docs/asset-extraction.md`, `docs/source-audit.md` and `docs/verification.md`.

**Informal overall project progress estimate: 36%.** This is a planning estimate, not a verified measure of completeness or playability.
