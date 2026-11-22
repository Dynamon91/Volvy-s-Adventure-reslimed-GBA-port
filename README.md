
> **Current development milestone: v1.9.3 (22 Nov 2026).** Based on v1.9.2,
> keeping World 1, Bonus Race, Shrewy and all source-derived art unchanged.
> Player, Shrewy and projectile metasprite parts outside the GBA viewport are
> now culled *before* writing hardware OAM, avoiding 9-bit/8-bit wraparound
> ghosts during camera transitions. Details: `RELEASE_NOTES_v1_9_3.md`.
> Host regression, ARM codegen and ROM structure pass; mGBA/hardware gameplay
> for v1.9.3 has not been verified.

# Volvy's Adventure: Reslimed — GBA homebrew adaptation

**Current milestone: v1.9.3 GBA OBJ wraparound/phantom-sprite protection (built on v1.9.2 visible-actor streaming).** The platform scenes and primary actor sprites now render at 1:1 source-pixel scale. Five source scenes are converted to 4bpp BG tiles with palette banks and a 32×32 streaming cache. The project now includes 56 native-resolution sprite animation sets, expanded World 1 enemy/prop rendering, original HUD glyphs, an OAM priority fix that keeps gameplay sprites from covering the HUD, and suppression of the source-only solid-green `spr_hole` marker. This remains a reconstruction prototype; the v1.9 ROM still needs an mGBA retest.

The project targets ARM7TDMI Game Boy Advance hardware using C, devkitARM/libgba, 20.12 fixed-point gameplay math, Mode 0 tiled backgrounds, hardware OBJ sprites, and VBlank-synchronized VRAM/OAM updates. The original user-supplied GameMaker `data.win` was parsed privately; it is not included in this project archive.

**Public repository asset policy:** the GitHub repository contains source code, conversion tools, tests and documentation. It intentionally excludes the original `data.win`, converted source-derived sprite/tile graphics and the experimental ROM. Those generated assets are included in the private v1.9 development ZIP; rebuilding from source requires supplying your own original `data.win` and running the extraction commands below.

## v1.9: hide source-only green hole markers (new mGBA capture review)

- Reviewed the newly supplied 112.8-second, 1920×1080, 30 FPS mGBA capture. It demonstrates a running build with title, W1-L1, onion pickups, breakable blocks, normal Volvy, Swolvy, Big Hand movement, vertical camera travel and pause.
- Found a visible neon-green 16×16 rectangle on the W1-L1 grass platform. Its color matches the extracted `spr_hole` frame: all 256 pixels use palette index 36 (BGR555 `0x33E6`, approximately RGB 49,255,98). The source object uses a solid-color marker; its original visibility flag is not recoverable from the available data, so the reconstruction treats it as a nonvisual logic marker.
- Removed `WORLD_HOLE` and bonus `HOLE`/`HOLE_DIR` from the runtime OBJ animation mapping. Their source placements remain in the object tables for future logic/reverse engineering; they no longer consume a visible OAM entry or render as a green square.
- Removed the unused `spr_hole` slot from the static OBJ cache: 25 visible static animation sets now reserve 18,560 bytes. Together with the 5,120-byte player reservation, the active world cache uses 23,680 of 32,768 OBJ VRAM bytes, leaving 9,088 bytes free.
- Updated World 1 and bonus OAM audit mappings so invisible hole markers are not counted as rendered sprites. Existing checked-in OAM reports remain conservative historical upper bounds until regenerated from source room data.
- Added `tests/test_invisible_marker_assets.py`, which verifies the original 16×16 source marker is a uniform green tile and is excluded from both world and bonus OBJ render mappings.
- Host regression suite: PASS. ARMv4T/Thumb code generation and experimental ROM structure checks are recorded in `docs/verification.md`.
- The new capture does not include a ROM checksum, so it cannot identify whether it was produced by v1.8. The v1.9 ROM has not yet been booted in mGBA. See `previews/v19_hole_marker_before_after.png` for an actual-capture/source-marker/source-reconstruction comparison; the right panel is not an emulator screenshot.

## v1.8: HUD layer priority correction from mGBA capture

- Reviewed the user's 117.5-second mGBA recording. It shows the title screen, W1-L1, onion collection, breakable blocks, pipes, normal Volvy, Swolvy, Big Hand movement and tall/vertical sections.
- The recording exposes a layer-order defect: gameplay OBJ sprites use priority 0, the same as the BG0 HUD. GBA OBJ wins equal-priority ties, so foreground OBJ parts can obscure HUD glyphs when the camera scrolls into tall geometry.
- Gameplay actors, boss actors, boss onions and static world actors now use OBJ priority 1. BG0 remains priority 0, so the HUD draws over actors; BG1 remains priority 1, so priority-1 OBJ actors still draw in front of the scene tiles. The title-screen Volvy remains at OBJ priority 0 to preserve the title composition.
- Added `tests/test_oam_priority_v18.py` and updated the OBJ tile-index regression for the priority bits.
- Host C regression suite: PASS. ARMv4T/Thumb generation: PASS (40 translation units with test-only headers). Experimental Clang/LLD ROM: PASS structural verification (2 MiB image, 41 C files, 1,240,652-byte payload).
- The v1.8 ROM has **not** yet been booted in mGBA. The user's recording is a reference capture, not a verified v1.8 runtime capture.

## v1.7: visual stability and original-source HUD

- Fixed the 32×32 streamed background's **visible vs. staged tile-window origin**. The player, sprites and BG1 scrolling now use the last VBlank-committed window until the new 4bpp tiles and map are ready. Gradual scrolling is clamped to the 256×256-pixel active window, preventing 8-pixel seam jumps; distant spawn relocations temporarily blank BG1 while the new map is staged.
- Fixed a VRAM alias: Shrewy's onion graphics previously started at OBJ byte 4,096, overlapping the reserved 5,120-byte player frame region. They now occupy OBJ bytes **6,144–7,168**, between the player frame and the boss graphic region starting at byte 8,192.
- Replaced the generated 5×5 ASCII font with 39 original glyphs extracted from the GameMaker `fnt_small` FONT/TPAG atlas. The glyphs are stored in **40 4bpp 8×8 tiles (1,280 bytes)**, including the transparent tile. There is no glyph resampling or synthesized outline. This is a source font selected for visual fidelity; the exact original HUD font callsite has not been recovered.
- Added 344,000+ camera-position presentation tests, native font data tests and boss/player/rock OBJ VRAM layout tests. The user's new 117.5-second MP4 shows a running Volvy GBA build in mGBA, including the title screen and W1-L1 gameplay at a window-reported 60 fps. The recording does not expose a ROM hash, so it cannot independently identify the exact binary revision. It reveals gameplay OBJ sprites covering parts of the HUD in tall vertical sections; v1.8 changes gameplay OBJ priority to correct that layer-order issue.
- New source-only previews: `previews/v17_hud_font_before_after.png`, `previews/v17_native_hud_scene.png` and `previews/v17_volvy_walk_source.gif`. These are asset reconstructions, not emulator captures.

## Current implementation

### Native-resolution graphics and scenes (v1.6)

- The primary renderer now uses **one original source pixel per GBA pixel**. The previous 2:1 downscaled player/world package is retained under `assets/experimental/legacy_2x/` for comparison.
- The source sprite pipeline emits 56 native-size animation sets, 501 OBJ metasprite parts and a shared 71-color 8bpp OBJ palette. The largest player animation frame is 4,608 bytes.
- The OBJ allocator reserves 5,120 bytes for the largest player frame, then 18,560 bytes for 25 visible streamed static animation sets. The combined reserved region ends at byte 23,680 of 32 KiB OBJ VRAM, leaving 9,088 bytes free. The source asset package still contains 26 static/prop animation sets; `spr_hole` is retained for auditing but not rendered. The active set covers Volvy, Swolvy, bunnies, platforms, onions, bumpers, hiveflies, Crowly, Vinechomp, hazards and room props.
- Native 4bpp BG scenes use up to 15 palette banks (bank 15 is reserved for the HUD), per-tile H/V flip flags, and 8×8 tile deduplication:
  - W1 L1: 2258×360 source pixels, 283×45 tile map, 504 unique tiles / 16,128 bytes.
  - W1 L2: 2258×360, 283×45, 971 tiles / 31,072 bytes.
  - W1 L3: 2258×360, 283×45, 1,415 tiles / 45,280 bytes.
  - Bonus Race: 5312×1120, 664×140, 154 tiles / 4,928 bytes.
  - Shrewy arena: 256×144, 32×18, 389 tiles / 12,448 bytes.
- All five scene conversions were rendered back from the generated 4bpp tile/map/palette data and compared against their source images after BGR555 quantization: **zero differing pixels**.
- BG1 uses a 32×32 (256×256-pixel) cache window. The cache can hold any visible window's at-most-1,024 unique tiles, even when the complete source level exceeds 32 KiB. Up to 128 tiles (4 KiB) are uploaded per VBlank.
- OBJ animation frames are streamed into fixed per-animation slots instead of storing every frame in VRAM. The current-frame cache is updated in bounded VBlank batches.
- The native camera preserves source pixel detail but shows a smaller portion of the source world than the old 2:1 mode. A 240×160 GBA cannot simultaneously retain a 480×320 source field of view and every source pixel; v1.6 prioritizes pixel fidelity and adjusts camera framing.
- Original background/parallax layers are baked into static scene composites. Live parallax is not yet implemented.

### World 1 gameplay

- Source-derived collision grids and placed interaction objects for W1 L1, L2 and L3.
- Fixed-point movement, jump buffering, coyote time, Big Hand hover/dash/grind/ground-pound/punch/dig, onion pickup, provisional Swolvy transformation, springboard bounce, squirrel patrol/stomp, action-block rebound and destructible blocks.
- Stage routing: W1 L1 → W1 L2 → W1 L3 → Shrewy arena. The title-screen `Select` key cycles W1 L1/L2/L3/Bonus Race for development testing; this is not an original-game menu feature.
- The W1 L3 mole, snake, TNT, boulder-spawner, hivefly and auto-digger placements are preserved as source metadata, but their gameplay is not yet reconstructed.

### World 1 bonus/race mode (v1.6)

- `GAME_BONUS` loads `rm_w1_b1_00` with a 664×140 native 4bpp scene and a 32×32 BG tile cache. Its AABB-derived collision grid remains 332×70 in the half-scale gameplay coordinate domain.
- The original `obj_volvy` placement is not used as the runtime spawn; the initial spawn is based on the first source trace frame `(264.3,535.0)` in source pixels and stored in the 2:1 physics coordinate domain.
- Both original race-start gate AABBs activate a local 60 Hz frame timer. The finish uses the horizontal marker plane, consistent with the recorded race event near the finish marker.
- All 397 source onions, 42 action blocks, 7 springboards, standard breakable blocks, six squirrels, 11 bunny placements, six square bumpers, four triangular bumpers, both Swolvy-only blocks, ten hiveflies, five Crowly actors, Vinechomp, spikes, cannon, button, rails, background-onion props and both platform families are rendered from original source sprites. `obj_hole`/`obj_hole_dir` placements are retained as logic markers but are intentionally not drawn: their extracted `spr_hole` is a solid neon-green rectangle. Two `obj_bunny` actors now use separate source-anchored movement/hopping prototypes, while nine `obj_bunny_ground` instances remain static. Bumper contacts now give provisional impulses using source-rotation-aware bounding boxes; the original game logic is not reconstructed. Action blocks rebound on Big Hand contact; springboards bounce on descending contact. One-way cloud platforms and breakable one-way blocks are supported. Two moving platforms now use source-derived pivots, per-instance scale and original sprite art, with provisional horizontal patrol and player carry.
- Authentic bunny competitor AI, original bumper impulses, official race timer/rewards, authentic platform paths, live parallax and room entry/exit callsites are not yet reconstructed. Enemy/prop sprites are rendered and animated, but their original AI, hit reactions and damage logic are not claimed. The provisional 5-onion transformation has been disabled for the bonus room because the supplied traces record collected onions but no Swolvy frames. The two larger Swolvy-only blocks now have dedicated graphics and collision tiles that normal Volvy cannot break.

### Shrewy boss encounter (new in v0.6)

- Extracted the dedicated source room `rm_shrewy`, its room scene, source object placements, collision bounds, and original sprite animations.
- Source boss pivot: `(184,104)`. Original `spr_shrewy_idle`: 48×48 pixels, origin `(25,39)`.
- Original Volvy room pivot: `(64,160)`, which lies below the 144px room. The source room-create relocation has not been decoded; the prototype uses a provisional floor-aligned spawn at top-left `(50,93)`.
- Boss arena collision is a 32×18 grid derived from three `obj_solid` AABBs and the top surface of one `obj_owp` platform. The derived floor row begins at y=112; this is not an original pixel-perfect collision mask.
- The Shrewy arena uses `player_init_native()` with 24×19 normal and 44×26 Swolvy hitboxes. World 1 and Bonus Race retain 12×10 / 22×13 half-scale physics hitboxes; the v1.6 renderer maps their positions to native 1:1 source sprites.
- `GAME_BOSS` is connected to stage progression. The player can jump and use Big Hand attacks; Shrewy has idle, wind-up, jump, stomp, dropkick, tornado, hurt, recovery and defeated states. The tornado prototype launches the original Shrewy-rock sprite. Boss HP is currently a three-hit test value.
- Four source onions are collectible in the boss room. The boss HUD displays the provisional HP count.
- All 15 Shrewy prop placements are now preserved: four onions, one cage, two autoholes, four adjustable autodigger markers and four cone emitters. The exporter stores instance IDs, pivot positions, Q8.8 scales, rotations and rotated sprite AABBs. These transforms are source-derived; hazard behavior remains unimplemented.
- A source audit found 5,510 script resource names and 65 Shrewy-related names, but no `CODE` chunk in `data.win`; original GML event logic has not been decompiled.
- Extracted the native `SEQN` resource `seq_cage_drop`: 60 fps, 60-frame sequence with an `obj_cage.position.y` Bezier curve; baked 60 samples into 20.12 fixed-point data. The sequence origin/trigger callsite is not yet known, so the curve is not yet connected to a gameplay event.
- Boss attack timing, damage windows, rock trajectory, health, player damage rules and exact spawn relocation remain provisional. The original GameMaker event bytecode/scripts have not been decompiled or behavior-matched.
- The Shrewy cage-drop Bezier now has a deterministic 60 Hz fixed-point sequence player. It outputs the extracted offset samples but is not auto-triggered; the sequence origin and boss callsite remain unresolved.

### World 1 race/bonus room source data (v0.9) — historical 2:1 conversions; active runtime uses v1.6 native assets

- Re-extracted `rm_w1_b1_00` at 5312×1120 source pixels (2656×560 GBA world pixels). Source placements include two race-start gates, one finish, a podium, seven crowd actors and bunny competitors, confirming a race-style bonus challenge distinct from the Shrewy arena.
- The earlier 2:1 tile-only conversion is retained as legacy data (332×70, 139 unique 8bpp tiles / 8,896 bytes). The active v1.6 runtime uses the native 664×140 4bpp scene described above.
- Preserved all 1,024 source instances across 46 object kinds, including 397 onions, 193 destructible blocks, 166 solid objects, nine rails and seven springboards.
- The earlier 2:1 composite (486 unique 8bpp tiles / 31,104 bytes) is retained for comparison only. The v1.6 native scene round-trips to its source composite with zero differing pixels after BGR555 quantization.
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

**Current environment status (v1.9):** the host C regression suite passes, including the new invisible-marker test. ARMv4T/Thumb code generation passes across 40 translation units using test-only GBA headers. The experimental Clang/LLD path produces a 2 MiB `.gba` image with a 1,240,652-byte payload from 41 C files; ARM entry, Nintendo logo prefix, checksum and ELF section checks pass. This is **not** a devkitARM/libgba build. The supplied video demonstrates an earlier/unknown build running in mGBA, but the v1.9 ROM has **not** been booted in mGBA or on hardware. Do not treat the experimental image as a verified playable ROM until that test is completed.

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
  --assets-root private_assets --out assets/converted --scale 1
python tools/prepare_boss_sprites.py private_assets/manifest.json \
  --assets-root private_assets --out assets/converted --scale 1

python tools/extract_gamemaker_rooms.py data.win \
  --asset-manifest private_assets/manifest.json \
  --assets-root private_assets --out private_rooms \
  --rooms rm_w1_l1_00 rm_w1_l2_00 rm_w1_l3_00 rm_w1_b1_00 rm_shrewy

# Native 1:1 4bpp scenes. No downscaling is performed.
python tools/build_native_scene_gba.py private_rooms/rm_w1_l1_00/tile_layers.png \
  --name w1_l1 --out assets/converted/rooms
python tools/build_native_scene_gba.py private_rooms/rm_w1_l2_00/tile_layers.png \
  --name w1_l2 --out assets/converted/rooms
python tools/build_native_scene_gba.py private_rooms/rm_w1_l3_00/tile_layers.png \
  --name w1_l3 --out assets/converted/rooms
python tools/build_native_scene_gba.py private_rooms/rm_w1_b1_00/tile_layers.png \
  --name bonus --out assets/converted/rooms
python tools/build_native_scene_gba.py private_rooms/rm_shrewy/tile_layers.png \
  --name shrewy --out assets/converted/rooms

# Rebuild source-derived World 1 actor tables (includes native-rendered enemies/props).
python tools/build_world_objects.py private_rooms/rm_w1_l1_00/room_data.json \
  --data-win data.win --out assets/converted/rooms
python tools/build_world_objects.py private_rooms/rm_w1_l2_00/room_data.json \
  --data-win data.win --out assets/converted/rooms
python tools/build_world_objects.py private_rooms/rm_w1_l3_00/room_data.json \
  --data-win data.win --out assets/converted/rooms
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
# Legacy 2:1 8bpp composite, retained only for comparison (not v1.6 runtime):
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

- BG char blocks 1+2: 32 KiB of 4bpp native-resolution scene tile cache. The complete L3 scene is 45,280 bytes, but only the visible 32×32 tile window is cached. Other native scene tile payloads are listed in `docs/visual-fidelity-v16.md`.
- BG1 screen block 27: one 32×32 (2 KiB) streamed map window. The map is rebuilt and copied when the camera crosses a tile boundary.
- BG palette banks 0–14: per-tile scene palettes; bank 15 is reserved for HUD glyphs.
- Level state: 27,352 bytes on the host ABI, including the 332×70 collision backing grid and 1,024 actor/removal slots.
- BG0 screen block 29: 4bpp HUD glyph overlay in character block 0.
- World-mode OBJ: 5,120-byte player frame reservation at OBJ VRAM offset 0; 18,560-byte one-frame-per-animation cache for 25 visible animation sets begins at offset 5,120. Both use 8bpp OBJ tiles.
- Boss-mode OBJ: native player frames at offset 0; boss onion frames at 4 KiB; Shrewy frames at 8 KiB; rock frame at 24 KiB. The world actor cache overlaps the boss onion window and is rebuilt when returning to a platform stage.
- All BG-map, OBJ-frame and OAM commits are synchronized to VBlank.

## Remaining work and limitations

- Decode and reproduce the original GameMaker scripts, pixel collision masks, boss behavior, spawn relocation, enemy AI and hazard logic.
- Refine the `rm_w1_b1_00` race/bonus prototype: recover original controller and entry/exit flow, validate/reconstruct authentic bunny AI, official timer/reward rules, original moving-platform paths and remaining hazard behavior.
- Implement authentic title/intro flow, saves, sound/music and live parallax.
- Run a real devkitARM/libgba build, boot in mGBA, inspect VRAM/OAM and test timing on hardware.
- Confirm original sprite scaling, onion threshold, boss HP, attack timings, hit rules and stage transitions against source scripts or captured gameplay.

New v1.3 movement diagnostics: `previews/bonus_bunny_movement_comparison_v13.png` and `previews/bonus_bunny_motion_trace_v13.csv`, derived from original graphics and a host-compiled prototype motion trace, **not emulator screenshots**. Earlier source-art previews include `previews/bonus_bunny_source_v12.png`, `previews/bonus_track_source_v12.png`, `previews/bonus_swolvy_block_v12.png` and `previews/bonus_original_sprite_reference_v12.png`. These are **asset reconstructions, not emulator screenshots**. The original v1.2 OAM-density audit covers 969,217 integer camera locations and finds 101 estimated entries (including eight reserved player parts) for static placements. Moving the two active bunnies can increase a camera view by at most two entries, giving a conservative **103/128 OAM capacity bound** for the v1.3 prototype. Neither result tests per-scanline OBJ cycles or an emulator.

### New v1.6 native-pixel fidelity milestone

- Replaced the active 2:1 player/world rendering path with native 1:1 source sprite frames and native 4bpp BG scenes.
- Added a per-tile palette-bank scene format with H/V flip deduplication and a tested 1,024-tile streaming cache.
- Rebuilt all five runtime scenes. The generated 4bpp maps round-trip to their source composites with zero pixel differences after BGR555 quantization.
- Expanded the runtime actor asset set to 26 source animations; 25 visible animation sets use one current frame per set. Added original hivefly, Crowly, Vinechomp, spike, cannon, button, hole, rail, background-onion and exit-hole graphics.
- Expanded World 1 source object tables with visible hivefly, hole, plant, worm and level-exit actors. Their rendering is integrated; enemy AI and damage behavior are not.
- Added native-resolution OAM audits: W1 L1 58/128, W1 L2 66/128, W1 L3 82/128 and Bonus Race 66/128 estimated entries including eight player parts. These are conservative object/part bounds, not per-scanline hardware-cycle tests.
- Added native scene cache and actual asset-window regression tests. Host tests pass; ARMv4T/Thumb code generation passes for 39 translation units with test stubs.
- Rebuilt the experimental 2 MiB ROM image (1,239,856-byte payload). Header/logo/checksum/ELF structure pass; emulator boot is still unverified.
- Added source-derived preview comparisons: `previews/v16_volvy_sprite_before_after.png`, `previews/v16_scene_before_after.png`, `previews/v16_w1_l1_native_240x160.png` and `previews/v16_native_sprite_sheet.png`. These are source reconstructions, not mGBA captures.

See `docs/render-stability-v17.md`, `docs/shrewy-boss.md`, `docs/world1-gameplay.md`, `docs/architecture.md`, `docs/asset-extraction.md`, `docs/visual-fidelity-v16.md`, `docs/source-audit.md` and `docs/verification.md`.

**Informal overall project progress estimate: 44%.** This is a planning estimate, not a verified measure of completeness or playability.

### New v1.4 moving-platform reconstruction

- Both original `obj_moving_platform` instance positions and source scaling (1.333× and 1.0×) are preserved. Their original 48×8 pixel sprite is converted to 24×4 GBA pixels, with an automatically generated 32×4 scaled variant for the wider instance. Five original `spr_cloud_platform` animation frames are also included.
- Moving platforms use a **provisional**, 0.5 GBA pixel/frame horizontal oscillation over ±24 pixels around the source pivot. The original per-instance `pre_create_code` IDs (3320 and 3321) have not been decoded, so these paths are not authentic.
- Their initial one-way collision footprints are now excluded from the static room grid and handled dynamically. The prototype supports descending landings, fractional-pixel carrying while standing, pass-through from below, and reset.
- Host regression tests and a C-generated motion trace accompany new 240×160 visual comparisons.
- The actual devkitARM/libgba build and mGBA boot remain **unverified**.

### New v1.5 ROM build milestone (superseded by v1.6)

- `sh tests/run_experimental_clang.sh` creates and structurally checks a 1 MiB **experimental** ARM7TDMI GBA image without relying on the missing devkitARM SDK. The image's code/data payload is 599,500 bytes; ROM header checksum and ARM ELF layout pass.
- Source graphics, collision tables, bunny prototypes, moving platforms and boss logic remain from v1.4. **No new original gameplay completeness is claimed.**
- Corrected initial OBJ palette upload to DMA and palette RAM clearing to 16-bit writes, respecting GBA graphics-hardware restrictions.
- `mGBA`/physical GBA testing and the standard `devkitARM` build remain unverified.

**User test request:** run the provided experimental `.gba` in mGBA and send the title/gameplay screenshots and any crash/error logs. See `docs/experimental-rom-v15.md` for the exact checklist.
