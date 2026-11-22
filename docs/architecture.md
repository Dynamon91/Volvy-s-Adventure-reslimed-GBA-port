# Architecture — current prototype (v1.3)

## Hardware target

- ARM7TDMI / ARMv4T, 240×160 display, Mode 0 tiled backgrounds.
- BG1 uses an 8bpp scene map; BG0 uses a 4bpp bitmap-font HUD.
- 128 hardware OBJ entries, 8bpp sprites and 1D tile mapping.
- 20.12 fixed-point world coordinates; no software floating-point in frame updates.
- OAM, BG map, palette and changed sprite-frame transfers are committed during VBlank.

## Update loop

1. Poll active-low `REG_KEYINPUT` and calculate pressed/held/released edges.
2. Update player, hand state, world actors or Shrewy state machine, collision and camera outside VBlank.
3. Select animation frames and update the CPU-side OAM shadow table.
4. Wait for VBlank.
5. Upload a changed BG scene and 64×32 BG1 map window if the active room changed or camera crossed a streaming boundary.
6. Upload changed player, boss or projectile OBJ frame data and mode-specific palette.
7. Commit the 128-entry shadow OAM table and HUD map.

## BG VRAM allocation

| Region | Use |
|---|---|
| BG char block 0 | 4bpp HUD glyphs |
| BG char blocks 1+2 | Up to 32 KiB of active 8bpp scene tiles |
| BG screen blocks 27–28 | BG1 64×32 streamed map window |
| BG screen block 29 | BG0 32×32 HUD map |

Scene budgets:

| Scene | Dimensions | Unique 8×8 tiles | Bytes |
|---|---:|---:|---:|
| W1 L1 | 1129×180 | 209 | 13,376 |
| W1 L2 | 1129×180 | 453 | 28,992 |
| W1 L3 | 1129×180 | 473 | 30,272 |
| Shrewy arena | 256×144 | 417 | 26,688 |

The three platform scenes are converted 2:1 and streamed horizontally through the 64×32 BG1 map. The Shrewy arena uses native 1:1 scene pixels in a 32×18 tilemap, copied into the upper-left of the same map window. Background/parallax layers are currently baked into static composites; live parallax is pending.

## OBJ VRAM and palettes

### Platform mode

- Animated player frame upload starts at OBJ VRAM offset 0; largest tested player frame is 1,280 bytes.
- Static actor animation cache starts at offset 2 KiB and uses 7,360 bytes of sprite data, including the new original bunny, Swolvy-only block and bumper frames.
- Shared player/world/bonus OBJ palette: 32 opaque colors plus transparent index 0.

### Shrewy boss mode

- A separate native-resolution boss sprite package uses an 81-color OBJ palette.
- Player frame upload starts at offset 0; largest native player frame is below 4 KiB.
- Four onion animation frames occupy the 4–5 KiB window.
- Shrewy frame upload starts at 8 KiB; largest tested boss frame is 2,304 bytes.
- Shrewy rock frame upload starts at 24 KiB; largest tested rock frame is 1,024 bytes.
- The normal world actor cache overlaps the onion window and is rebuilt when switching back to a platform stage.

OBJ attr2 indexes 32-byte units. An 8bpp 8×8 tile occupies 64 bytes, so logical 64-byte tile indices are multiplied by two when written to attr2. All uploaded OBJ tile bases are aligned accordingly.

## Gameplay modes and stage routing

- `GAME_TITLE`, `GAME_PLAYING`, `GAME_PAUSED`, `GAME_OVER`, `GAME_VICTORY`, `GAME_BOSS`, `GAME_BONUS`.
- W1 stage IDs 1–3 select source-derived room collision, object arrays and static scenes.
- W1 L1 → W1 L2 → W1 L3 → Shrewy arena is the current prototype route.
- `GAME_BOSS` uses stage ID 4 and the dedicated `rm_shrewy` scene/collision table.
- Title Select cycles W1 L1/L2/L3/Bonus Race for development testing; it is not a source-verified original menu behavior.
- `GAME_BONUS` loads `rm_w1_b1_00`, source-derived AABB collision, 397 onions, rendered action blocks/springboards/squirrels/bunnies/bumpers, provisional Big Hand action-block rebound, springboard bounce, start gates, a local timer and a horizontal finish plane. Bunny AI and official race timing/rewards are not implemented.

## Collision and actor models

- Platform stages use 8×8 collision cells generated from source object AABBs and source room placement metadata.
- `Level` backing storage is 332×70 cells (23,240 bytes); the complete `Level` struct is 27,352 bytes on the host ABI. This lets the 2656×560 GBA bonus room share the fixed-point player controller. Active W1 platform dimensions remain 142×32 and the boss grid remains 32×18. World actor state is stored in fixed-size arrays.
- The Shrewy room uses a 32×18 collision grid derived from three solid-object AABBs and the top surface of one one-way platform.
- The bonus room uses a 332×70 AABB-derived grid: 166 solids, 195 standard/Swolvy breakables, 4 breakable-one-way blocks, 18 spike placements, 9 rails, 7 cloud platforms and 2 static moving-platform footprints. One-way platforms collide only while descending; moving paths are not implemented.
- Native-resolution boss mode uses 24×19 Volvy and 44×26 Swolvy hitboxes; platform mode uses 12×10 and 22×13 respectively.
- The Shrewy state machine and projectile behavior are provisional. Original GameMaker scripts and collision masks have not been decoded.

## Current gameplay versus visual integration

- Original Volvy/Swolvy animation frames and pivots are integrated in platform and boss modes.
- Original W1 L1/L2/L3 static scenes, collision tables and placed object arrays are loaded by the stage selector.
- Onion pickup, destructible blocks, squirrel patrol/stomp, springboard bounce and action-block interactions have host-side tests but provisional tuning.
- Shrewy's original idle/wind-up/jump/stomp/dropkick/tornado/hurt animations are selected by the prototype boss state machine. Four boss-room onions are collectible.
- Boss health, attack timings, vulnerability windows, projectile velocity, contact damage and source spawn relocation remain unverified.
- W1 L3 mole/snake/TNT/hivefly/auto-digger behavior remains incomplete. The `rm_w1_b1_00` bonus/race room now has a source-derived scene, collision grid, onion pickup, start/finish state and local timer; bunny AI, moving-platform paths, official race timing and reward rules remain incomplete.

## Performance policy

- No floating-point math in frame updates; floats are confined to offline source-asset conversion tools.
- Keep hot player/collision routines in ARM code where profiling justifies it.
- Use fixed-size pools and arrays for entities.
- Stream large room maps rather than allocating dynamic map structures.
- Keep OAM, palette and VRAM transfers synchronized to VBlank.
- A real devkitARM/libgba build and mGBA/physical hardware validation are still required.


## Source animation data

The Shrewy `seq_cage_drop` resource is exported to a 60-sample 20.12 fixed-point table. The helper is available to the boss-room sequence runner, but its trigger and room-origin offset remain unresolved, so it is not started automatically.


## Additional source-derived room data (v1.2)

`rm_w1_b1_00` has a 332×70 tile-layer-only conversion (139 tiles / 8,896 bytes) and a runtime static composite of all visible background+tile layers (486 unique 8bpp tiles / 31,104 bytes, 24 opaque colors), plus a 332×70 AABB-derived collision grid and a 1,024-instance source table. The `GAME_BONUS` runtime consumes the composite, collision and source object placements. Original parallax rates remain metadata only; bunny AI and moving-platform paths are not implemented.


## Bonus race room runtime

- `GAME_BONUS` is available through the development title selector (stage ID 5).
- The room is 332×70 8px GBA cells. `scene_stream_update` now streams both horizontal and vertical 64×32 map windows; the BG scroll offsets are relative to the streamed origin.
- The source room's 397 onion instances are rendered and collected through the `Level.removed[1024]` table.
- Collision values 7 and 8 are one-way and breakable-one-way platforms. Player vertical movement only lands on them while descending; ground-pound/Swolvy contact breaks the breakable variant.
- The local timer counts update frames after source start-gate overlap. The finish is an X-plane derived from the source trace event and finish marker; the original race controller remains unknown.

## v1.2 source sprite and scrolling corrections

- The sprite exporter now includes `spr_bunny_hop`, `spr_destructible_block_swolvy`, `spr_bumper_sq` and `spr_bumper_tri`, yielding 41 player/world/bonus sprite sets using 32 opaque OBJ colors. Source bunny placements are animated visually but remain stationary without recovered original competitor AI.
- `TILE_SWOLVY_BLOCK` (9) separates the two source Swolvy-only destructible objects from ordinary breakables. Normal Volvy cannot destroy these cells; powered-up Swolvy can. The 5-onion automatic test transformation is disabled only in `GAME_BONUS` pending the original trigger.
- The 64×32 scroll-map origin and screenblock packing are now implemented in `core/scene_window.c` and verified through host tests of bonus/W1/Shrewy viewports.
- Worst-case room sprite occupancy was checked over all 969,217 integral camera origins: 93 static actors, or 101 OBJ slots after reserving eight player parts, below the 128-entry hardware OAM cap. The test does not cover per-scanline OBJ-cycle limits or live animation on real GBA hardware.

## v1.3 bonus opponent and bumper modules

- `BonusRace.bunnies[2]` tracks only the two source `obj_bunny` instances; nine `obj_bunny_ground` decorations retain source positions. The motion is a non-authentic deterministic 20.12 patrol/hop test harness. The renderer reads the resulting positions without inventing extra sprites.
- `BonusRace.bumper_hits` counts prototype bumper contacts; source-derived AABBs are stored in `bonus_w1_bumper_bounds.c`. Only 10 transformed source placements are checked per frame, instead of scanning all 1,024 room instances for bumper collision.
- Version 1.3 adds two C translation units (core motion and generated bumper geometry). All mode-dependent state resets at `bonus_race_init()` and moves only during the active gameplay update.
- The original event/script behavior and rotated bumper OBJ graphics remain unavailable. GBA timing/performance have not been measured on emulator or hardware.
