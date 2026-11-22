# World 1 bonus/race room — `rm_w1_b1_00` (v1.3 runtime prototype)

## Classification and source evidence

The original room is a race-style bonus challenge, separate from the Shrewy boss arena. Source placements include two `obj_race_start` gates, one `obj_race_finish`, one podium, seven crowd actors, two `obj_bunny` instances, nine `obj_bunny_ground` instances, and four segment markers. The supplied trace contains one `race` event.

The source trace does not expose bunny opponent positions or AI state. In v1.3, two `obj_bunny` instances use independent, deterministic hop/patrol **prototype** motion anchored to their recorded room pivots; the nine `obj_bunny_ground` placements remain stationary. The room is still a solo timed-race shell: no authenticated bunny racing AI, placement results or prize rules have been reconstructed.

## Room and graphics

| Property | Source / GBA conversion |
|---|---:|
| Source room | 5312 × 1120 pixels |
| GBA world | 2656 × 560 pixels |
| Tile grid | 332 × 70 cells |
| Visible tile layers | `Tiles_1`, `TilesFront` |
| Tile-layer-only conversion | 139 unique tiles / 8,896 bytes |
| Runtime full composite | 486 unique 8bpp tiles / 31,104 bytes |
| Runtime palette | 24 opaque colors + transparent index |
| Source instances | 1,024 across 46 object kinds |

The converter crops the 527×364 GameMaker layer storage to the 332×70 room bounds. It composites `Tiles_1` behind `TilesFront`, bakes tile rotate/mirror/flip flags into the tile art, and deduplicates the result. The full composite fits the combined 32 KiB 8bpp BG character-block budget.

The renderer now streams the 332×70 map through its 64×32 BG1 window on both axes. The vertical map origin slides as needed to keep the 160px viewport inside the 32-tile (256px) BG window; BG scroll offsets are relative to that origin. This is needed because the room is 70 tiles tall, unlike the 23-tile platform scenes.

The room extractor's `tile_layers.png` composites all four visible background-sprite layers (`bg_bonus_4`, `bg_bonus_3`, `bg_bonus_1`, `bg_bonus_2`) with both visible tile layers in GameMaker depth order. `tools/build_bonus_composite_gba.py` converts that full image to the runtime 332×70 map: 486 unique 8×8 tiles, 31,104 bytes and 24 opaque colors. It fits the combined 32 KiB BG character-block budget (charblocks 1+2). The source background layer parallax rates are retained in metadata, but this runtime composite is static; live parallax is not yet implemented.

The separate `bonus_w1_room.c/.h` remains a tile-layer-only conversion (139 tiles / 8,896 bytes) and preserves the original 1,024-instance table. The renderer uses `bonus_w1_composite.c/.h` for the actual BG1 scene and `bonus_w1_room.c/.h` for source object placements.

## Collision conversion

`tools/build_bonus_collision.py` uses the original object sprite dimensions and origins parsed from `data.win`, then applies each instance's scale and rotation to produce an axis-aligned collision footprint. The resulting 332×70 GBA grid uses:

| Tile value | Meaning | Source placements |
|---:|---|---:|
| 1 | Solid | 166 `obj_solid` |
| 2 | Breakable | 193 `obj_destructible_block` |
| 3 | Rail | 9 `obj_rail_dave` |
| 5 | Hazard | 18 `obj_spikes_dave` |
| 7 | One-way | 7 cloud platforms + 2 moving platforms |
| 8 | Breakable one-way | 4 one-way destructible blocks |
| 9 | Swolvy-only breakable | 2 `obj_destructible_block_swolvy` |

Collision cells after overlap priority: 8,725 solid, 271 ordinary breakable, 8 Swolvy-only breakable, 146 rail, 235 hazard, 43 one-way and 4 breakable-one-way. The cell map is an AABB-derived approximation, not a pixel-perfect mask. Moving platforms currently use their initial source AABB; their movement paths are not reproduced. Hole and hole-direction semantics, dynamic platforms, and several hazards remain unimplemented.

## Runtime integration

The bonus room is available as a development-selectable mode:

- Title `Select` cycles W1 L1 → W1 L2 → W1 L3 → Bonus Race → W1 L1.
- `Start` enters the selected mode.
- `GAME_BONUS` uses the normal fixed-point player controller and the 332×70 collision grid.
- The player spawn is based on the first recorded trace frame: source pivot `(264.3,535.0)`, converted to GBA world pixels and adjusted for the normal player origin. The separate `obj_volvy` room instance at `(158,675)` is not used because it lies within a large source solid AABB and likely requires a room-create relocation.
- Both `obj_race_start` AABBs are used as start gates. Their source sprite is `spr_solid` (16×16, origin 8×8), converted to 8×8 GBA collision/trigger boxes.
- The finish is represented by the source finish marker's horizontal AABB plane. The trace fires its `race` event at source x=4972.59, immediately before the finish marker at x=4984. The trace event's Y does not overlap the marker's sprite AABB, so the prototype uses an X-plane crossing rather than inventing a vertical collision condition.
- A local frame counter increments at the GBA update cadence (nominal 60 Hz) after the start gate is crossed. The HUD displays this local elapsed time. It is not claimed to match the original race timer.
- The 397 source onion placements are rendered from the converted onion animation and collected using their source sprite bounds. Collected state is indexed by the original 1,024 room-instance table.
- One-way cloud platforms and one-way destructible blocks are supported by downward-only collision. Moving platforms are static until their path behavior is reconstructed.

## Trace analysis

The four supplied clips contain 2,983 player frames and 478 interaction records. The trace's segment anchors and positions are continuous enough to inspect the route, but `time_ms` ranges overlap. The final clip alone spans 33,682 ms from its first row to the `race` event; that is not the full race time. The full gate-to-finish elapsed time cannot be calculated from these files.

The trace contains no bunny opponent positions or AI variables. It also contains no Swolvy frames; consequently the provisional 5-onion auto-transformation is disabled in this bonus runtime. Do not use the trace as evidence for bunny speed, opponent placement, race rewards, or Swolvy behavior in this room.

## Tests

`tests/test_bonus_race.c` checks:

- 332×70 collision-grid dimensions and source collision populations;
- safe body/floor cells at the trace-derived player spawn;
- one-way landing from above;
- breakable one-way destruction by ground-pound;
- source onion collection and removal;
- start-gate activation, elapsed-frame increment, and finish-plane crossing.

These are host-side logic tests, not mGBA or hardware validation.

## Files

- `assets/converted/rooms/bonus_w1_room.c/.h` — BG tiles, palette, 332×70 map and all 1,024 source instances.
- `assets/converted/rooms/bonus_w1_room_manifest.json` — scene dimensions, tile and object counts.
- `assets/converted/rooms/bonus_w1_collision.c/.h` — 332×70 AABB-derived collision grid.
- `assets/converted/rooms/bonus_w1_collision_report.json` — source sprite bounds and collision provenance.
- `assets/converted/rooms/bonus_w1_trace_summary.json` — trace statistics and timing caveats.
- `assets/converted/rooms/bonus_w1_trace_frames.csv` — normalized 2,983-frame trace table.
- `previews/bonus_w1_race_overview_v09.png` — full-room source route preview.
- `previews/bonus_w1_gba_camera_samples_v09.png` — four trace camera crops.
- `previews/bonus_w1_race_start_v11.png` — initial trace-spawn 240×160 scene/player composition.
- `previews/bonus_w1_race_collision_v11.png` — initial-spawn collision-grid overlay.
- `previews/bonus_w1_race_midcourse_v11.png` — mid-course 240×160 composition using trace segment 2, frame 494 (`spr_ground_pound`, image 4, 16 onions) and source-placed actors.
- `previews/bonus_w1_race_midcourse_collision_v11.png` — matching mid-course collision-grid overlay.

All previews are static source-asset compositions or trace diagnostics, not emulator captures.

## Rebuild commands

```sh
python tools/extract_gamemaker_rooms.py data.win \
  --asset-manifest private_assets/manifest.json \
  --assets-root private_assets --out private_rooms \
  --rooms rm_w1_b1_00

python tools/build_bonus_room_gba.py \
  private_rooms/rm_w1_b1_00/room_data.json \
  private_assets/tilesets/TileSet_bonus.png \
  --out assets/converted/rooms

python tools/build_bonus_composite_gba.py \
  private_rooms/rm_w1_b1_00/tile_layers.png \
  --name bonus_w1_composite --scale 2 --out assets/converted/rooms

python tools/build_bonus_collision.py \
  private_rooms/rm_w1_b1_00/room_data.json data.win \
  --out assets/converted/rooms
```

## Remaining work

1. Recover original room-entry and room-exit controller logic.
2. Reconstruct bunny competitor AI and official timer/reward rules.
3. Implement moving-platform paths, live parallax, and remaining hazard semantics.
4. Confirm the start/finish trigger rules from original scripts or additional footage.
5. Validate the streamed map, OBJ density, timer cadence and collision in mGBA and on hardware.

## v1.2 original actor sprite integration and OAM audit

- Re-extracted four original sprite resources from the user-supplied `data.win`, bringing the original player/world/bonus sprite converter to 41 sets / 32 opaque colors.
- `spr_bunny_hop`: four source 48×48 frames become 24×24 GBA artwork stored as one padded 32×32 hardware OBJ per frame. All two `obj_bunny` and nine `obj_bunny_ground` placements can now be drawn with the original animation. This is **visual integration only**; racing AI remains unknown.
- `spr_destructible_block_swolvy`: source 32×32 sprite becomes a native 16×16 GBA OBJ, and its dedicated collision type (9) prevents ordinary Volvy from breaking the block.
- `spr_bumper_sq` and `spr_bumper_tri`: original 16×16 source frames become 8×8 GBA OBJ artwork. Version 1.3 adds provisional bumper impulses with a 12-frame cooldown. Offline hitboxes are computed from each original instance's pivot, scale and rotation; four corners include 9.63° and 135° placements. Exact original rebound rules and sprite rotation rendering are still missing.
- Static OBJ frame cache requires 7,360 bytes; with the reserved 2 KiB player animation region it remains below 32 KiB.
- `tools/analyze_bonus_oam_density.py` reports 93 source actors potentially visible in any integral 240×160 GBA camera viewport; allowing up to eight player OBJ parts gives an estimated worst-case 101 / 128 hardware OBJ entries. This is a full integer-camera AABB occupancy audit, **not** an mGBA scanline/cycle validation.
- `tools/preview_bonus_runtime_assets.py` reconstructs the **compiled 8bpp GBA scene arrays** and composites source sprites and trace poses. Its previews are not emulator screenshots.

Preview files: `previews/bonus_bunny_source_v12.png`, `previews/bonus_track_source_v12.png`, `previews/bonus_swolvy_block_v12.png`, `previews/bonus_original_sprite_reference_v12.png`.

## v1.3 bunny movement and source-rotation-aware bumpers

The original room contains precisely **two** `obj_bunny` instances at source pivots `(2144,792)` and `(1944,792)` (GBA pivots `(1072,396)` and `(972,396)`). The separate nine `obj_bunny_ground` instances are clustered around source x=3249–3297. The source does not identify those nine as active racers.

`core/bonus_bunnies.c` implements a deliberately provisional, deterministic state per active bunny: 20.12 fixed-point position/velocity, conservative horizontal patrol within ±44 GBA pixels of its source pivot, local ledge/wall probes, 80-frame staggered hops, gravity and tile-supported landing. The bunnies do not alter the race finish condition and do not damage Volvy. The bunnies only update after a start gate has been crossed; pause/game-over logic prevents subsequent movement updates. These parameters are engineering defaults, **not** values recovered from the original GameMaker scripts.

The six square `obj_bounce_block` and four `obj_corner_bumper` objects now produce collision responses and 12-frame cooldowns in the local bonus prototype. `tools/build_bonus_bumper_bounds.py` exports GBA AABB hitboxes from the original room's instance ID, sprite bounds, scales and rotations. A corner at 135° occupies a **24×23 GBA pixel AABB**, larger than an unrotated 16×16 object. The C runtime uses the generated positions, but the impulse directions and strengths are provisional, and GBA OBJ rotation/art masks are not implemented.

Host-side regression tests cover source pivots, separation from nine static bunnies, hop/patrol/reset, square/diagonal rebound, cooldown suppression and transformed bumper geometry. `tests/bonus_bunny_motion_trace.c` exports a deterministic CSV of the actual host-compiled motion, and `tools/preview_bonus_bunny_motion.py` composites those positions over original extracted source art. This preview is not an mGBA capture.

Source regeneration:

```sh
python tools/build_bonus_bumper_bounds.py \
  private_rooms/rm_w1_b1_00/room_data.json --out assets/converted/rooms
```

Pending: original AI scripts, official racing opponents, precise bumper masks/rotated graphics, bonus race rewards and actual GBA testing.

## Moving platform integration (v1.4)

Two original `obj_moving_platform` placements are present at source pivots `(2576,488)` and `(3080,424)`, with `scale_x` 1.333333 and 1.0. Their source sprite is `spr_moving_platform` (48×8; origin `(24,8)`). The resulting GBA widths are 32 and 24 pixels, with 4-pixel height. The first instance uses a generated 4/3 horizontal source-art scale, performed inside `prepare_player_assets.py` so no extra private PNG is required. Five original cloud platform frames are also converted.

The original platform setup IDs 3320 and 3321 are recorded, but their event code/path controllers have not been decompiled. Thus this prototype deliberately uses ±24 GBA-pixel patrols at 0.5 px/frame. The current runtime handles one-way landing and carrying while standing. Their original collision AABBs are excluded from the static grid to prevent invisible platforms after the actors move. Static cloud platforms retain downward-only tile collisions.

`tools/trace_bonus_platforms.c` generates the compiled C frame-position CSV. `tools/preview_bonus_platforms.py` composites those positions over the source-derived backdrop for diagnostic frames 0 and 48; this is not an mGBA capture. `tests/test_bonus_platforms.c` verifies the original pivots/scale, dynamic geometry, carriage, pass-through from below, movement limits, and reset.
