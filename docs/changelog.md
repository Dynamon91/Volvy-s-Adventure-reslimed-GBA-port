## 2026-11-21 — v0.5 (World 1 Level 3)

- Extracted Bundtcake Burrows `rm_w1_l3_00` and original burrow parallax resources.
- Converted 473 8bpp scene tiles (30,272 bytes), 142x23 map and 495 interaction records.
- Increased `LEVEL_MAX_OBJECTS` from 384 to 576 and linked three-stage collision, renderer and HUD routing.
- Added development-only title Select level cycling and Level 3 host tests.
- Preserved 60 not-yet-implemented world-object source placements as metadata.
- Improved solid-vs-breakable collision priority and removed repeated breakable cleanup work.
- Host C tests, GCC strict syntax, asset budgets and ARMv4T/Thumb relocatable build pass with test headers.
- Not verified as a playable GBA ROM; original mole/snake/TNT/auto-digger behavior and Shrewy boss pending.

## 2026-11-21 — v0.4 (World 1 Level 2)

- Imported Fudge Forest original room scene, 453 tiles/28,992 bytes.
- Loaded 308 named actors and the W1 L2 coarse collision grid.
- Implemented stage selection, vblank scene replacement, HUD stage label.
- Expanded Level object arrays to 384 records.
- Added source-derived stage 2 tests and two-char-block VRAM checks.
- ARMv4T object codegen passes with test-only headers; true ROM build pending.

# Change history

## v0.8 — 2026-11-21 (Shrewy cage-drop sequence)

- Parsed the original `SEQN` chunk (version 1, seven sequence resources) and extracted `seq_cage_drop` with its nested `obj_cage` and parameter tracks.
- Recovered the 60 fps / 60-frame sequence metadata and five-point Bezier `position.y` curve (67-frame keyframe span).
- Added a no-floating-point 20.12 fixed-point sample table and `shrewy_cage_drop_offset()` helper for frames 0–59.
- Added source sequence extraction and conversion tools, JSON manifests, a six-frame visual contact sheet and regression tests.
- Preserved the unknown sequence origin and trigger callsite as unresolved; the curve is not yet triggered from boss defeat.
- Expanded the source audit to list all seven sequence resources; no GML event bytecode is claimed.
- Host tests and ARMv4T/Thumb code generation pass with test-only headers. Real devkitARM/libgba ROM build and mGBA boot remain unverified.


## v0.7 — 2026-11-21 (Shrewy source placement audit)

- Reopened the original RAR and extracted the packaged `data.win`, selected gameplay traces and Shrewy music for private analysis.
- Expanded the Shrewy room prop table from 11 to 15 placements by including all four `obj_adjustable_autodigger` instances.
- Preserved source instance IDs, pivots, Q8.8 scales, rotation angles and rotated sprite AABBs for all Shrewy props.
- Added `tools/audit_shrewy_source.py`, which inventories related script names, room placements and trace coverage without pretending to decompile unavailable GML.
- Confirmed that `data.win` contains 5,510 `SCPT` resource names but no `CODE` chunk; boss AI parameters remain provisional.
- Added regression assertions for all five Shrewy prop classes and exact source transforms.
- Added a source-placement overlay preview. It is a diagnostic composition, not an emulator screenshot.
- Real devkitARM/libgba ROM build and mGBA execution remain unverified.


## v0.3 — 2026-11-21

- Added `core/world_interactions.c` with springboard bounce, stomp, hand-attacks, action-block interaction and side-contact damage prototype.
- Added per-instance squirrel patrol position, facing and cooldown state to the World 1 level.
- Added per-object collision rectangle queries and per-frame actor movement.
- Fixed coarse tile collision cleanup when a misaligned destructible block shares a collision tile with its neighbor.
- Integrated original `spr_squirrel_walk`, `spr_squirrel_hurt` and `spr_action_block_spent` assets, retaining previous original player/background assets.
- Renderer now preloads static actor animation frames and selects them according to runtime state, avoiding repeated actor uploads per frame.
- Fixed World 1 collision map initialization to use a contiguous byte copy rather than out-of-bounds multidimensional array indexing.
- Added `tests/test_world_interactions.c` and `tests/test_asset_layout.py`, including spring bounce, patrol, attack/hurt and VRAM budget checks.
- Verified host tests, ARMv4T/Thumb target object generation with test-only headers, and generated static art previews.

**Not verified:** actual devkitARM/libgba link, mGBA boot, original script physics/AI, audio, saves or all-world gameplay.

## v0.2 — 2026-11-21

- Extracted World 1 Level 1 collision and object placements from packaged GameMaker room data.
- Integrated a streamed original World 1 scene, original character animation data and a simple GBA HUD.
- Added host-side collision/object and player camera gameplay integration tests.

## 2026-11-21 — v0.6 (Shrewy boss encounter prototype)

- Recovered `rm_shrewy`, `obj_boss_shrewy`, `obj_boss_shrewy_boulder`, and the original Shrewy animation sprites from the supplied `data.win`.
- Converted the native 256×144 boss arena into 417 8bpp BG tiles / 26,688 bytes and generated a 32×18 AABB-derived collision map.
- Added a separate 81-color native-resolution boss/actor OBJ sprite package, preserving the normal platform-stage 2:1 sprite set and 29-color palette.
- Added native-resolution player hitboxes and scaled fixed-point physics for the boss arena.
- Added `GAME_BOSS`, stage ID 4, boss-room scene loading, boss HUD, onion pickup, Shrewy sprite-state rendering and rock-projectile rendering.
- Added provisional Shrewy idle/wind-up/jump/stomp/dropkick/tornado/hurt/recover/defeated state machine and three-hit test health.
- Added boss-room, native-scale player, attack cycle, damage-window, projectile and asset-memory regression tests.
- Host suite passes; ARMv4T/Thumb relocatable code generation passes with test-only headers. Real devkitARM/libgba ROM build and mGBA boot remain unverified.

## v0.9 — 2026-11-21 (World 1 race/bonus room source integration)

- Re-extracted `rm_w1_b1_00` with all 719 available sprite resources and all 18 source tilesets, resolving its four background sprites.
- Classified the room as a race-style bonus challenge from its source race gates, finish, podium, crowds, bunny objects and recorded `race` event; explicitly separated it from `rm_shrewy`.
- Converted the room's two visible `TileSet_bonus` layers into a 332×70 GBA tilemap with 139 unique 8bpp tiles (8,896 bytes) and 22 opaque colors.
- Baked source tile rotate/mirror/flip flags into deduplicated tile art and preserved all 1,024 source instances across 46 object kinds in a compact C table.
- Preserved the four visible background layer offsets and parallax rates as source metadata; live parallax remains pending.
- Analyzed four trace clips: 2,983 player frames and 478 interaction events, including one race-finish event. Correctly withheld a full-race time because the clips' time domains overlap.
- Added source-route and four 240×160 camera-sample previews from original room graphics and trace sprite frames.
- Added a deterministic 60 Hz fixed-point cage sequence player for the extracted Shrewy Bezier samples; trigger/origin remain unresolved and no automatic gameplay trigger is claimed.
- Added bonus-room C data tests, trace/asset budget assertions, and documentation. Actual race controller, bunny AI and minigame progression remain unimplemented.
- Real devkitARM/libgba ROM build and mGBA boot remain unverified.
