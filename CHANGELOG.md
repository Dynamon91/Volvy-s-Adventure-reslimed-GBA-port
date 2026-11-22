# Changelog

## v1.9.2 — visible-actor animation streaming optimization

- Skip requesting static OBJ animation frame uploads for World 1 and Bonus Race actors whose full desired and current resident frames are both outside the 240×160 viewport.
- Unify part-level clipping with the off-screen visibility check, including hardware X/Y coordinate wraparound cases.
- Keep every source sprite, animation frame, actor placement and collision table unchanged. The v1.9.1 VBlank frame coherence scheduler remains intact.
- Add portable viewport-edge tests plus a source-level regression that verifies culling happens *before* the shared frame upload budget is reserved.
- Host C/asset tests, ARMv4T/Thumb code generation and experimental 2 MiB GBA ROM structure: PASS. **No mGBA boot or hardware verification.**

## v1.9 — suppress source-only green hole marker

- Reviewed the new mGBA recording and traced the neon-green 16×16 rectangle to the uniform `spr_hole` source marker (palette index 36 / BGR555 `0x33E6`).
- Kept `obj_hole` placements for source logic but removed their world/bonus OBJ rendering.
- Removed the unused static animation cache slot: 25 visible static animation sets, 18,560 bytes; combined player+static OBJ allocation 23,680 bytes, 9,088 bytes free.
- Added source-asset regression coverage and excluded invisible markers from future OAM estimates. See `previews/v19_hole_marker_before_after.png` for the capture/source comparison.


## v1.8 — HUD priority correction from user mGBA footage

- Reviewed the 117.5-second user-supplied mGBA capture, covering title, W1-L1, onions, blocks, pipes, Volvy/Swolvy states, Big Hand movement and vertical sections.
- Identified foreground OBJ/HUD overlap: gameplay sprites were at OBJ priority 0, tying BG0's priority; GBA OBJ wins same-priority ties.
- Moved gameplay player, world actors, bonus actors, boss, boss rock and boss onions to OBJ priority 1. BG0 HUD remains priority 0; BG1 scene remains priority 1. The title-screen Volvy keeps priority 0 for its intended title composition.
- Added a dedicated HUD/OAM priority regression test and updated the 8bpp OBJ layout test.
- Host C regression suite: PASS. ARMv4T/Thumb code generation: PASS (40 translation units with test-only headers).
- Experimental Clang/LLD ROM: PASS structural checks, 2 MiB image, 1,240,964-byte payload, 41 C files.
- devkitARM/libgba build and v1.8 mGBA boot: NOT VERIFIED.


## v1.7 — scroll stability, source-font rendering and boss VRAM safety

- Staged and displayed 4bpp BG windows now track separate origins; scroll is calculated against the **committed** window rather than the upcoming streamed window.
- Added camera clamping during incremental streaming, plus one-frame BG blackout for hard teleports, including cache-only window switches.
- Relocated Shrewy boss onions to OBJ VRAM bytes 6,144–7,168 to avoid overlapping the 5,120-byte player reservation.
- Recovered the `fnt_small` original 39-character GameMaker font atlas, converted the original ink pixels to 40×8×8 GBA 4bpp tiles, and integrated them into the HUD.
- Added source-derived old/new HUD comparison PNG and original Volvy walking GIF (not emulator screenshots).
- Added `test_native_presentation.c`, `test_hud_font_asset_v17.py`, `test_obj_vram_layout_v17.py`, and the source font extraction tool.
- Host logic/asset tests: PASS; ARMv4T/Thumb code generation: PASS (40 units using test headers); Clang/LLD experimental ROM header/ELF verification: PASS (41 C files).
- v1.7 ROM boot in mGBA: NOT YET TESTED. The later user-supplied mGBA capture shows a running build but does not identify the exact ROM revision.


## v1.6 — native-pixel visual fidelity pass

- Switched the active player/actor rendering package from 2:1 downscaled sprites to 1:1 source-pixel sprites.
- Expanded the native sprite set to 56 animation sets, 501 metasprite parts and a 71-color OBJ palette.
- Added a 5,120-byte player frame reservation and a streamed one-frame-per-animation cache for 26 static actor animation sets (18,816 bytes).
- Added source sprites for hivefly, Crowly, Vinechomp, spikes, cannon, button, holes, rail slices, background-onion props and level-exit holes.
- Expanded World 1 object tables to render hivefly, hole, plant, worm and level-exit actors.
- Converted W1 L1/L2/L3, Bonus Race and Shrewy scenes to native 1:1 4bpp tilemaps with 15 scene palette banks and a reserved HUD bank.
- Added a 32×32 / 1,024-tile BG cache, H/V flip flags, per-tile palette banks and bounded VBlank uploads.
- Verified all five native scene outputs by rendering the generated tile/map/palette data back to PNG: zero differing pixels after BGR555 quantization.
- Added native-resolution OAM audits and tests for actual generated scene assets and cache windows.
- Rebuilt the experimental Clang/LLD ROM image and passed structural GBA/ELF checks.

### v1.6 verification status

- Host C regression suite: PASS.
- ARMv4T/Thumb code generation: PASS, 39 translation units with test-only headers.
- Experimental Clang/LLD ROM: structurally valid 2 MiB image; boot not verified.
- Standard devkitARM/libgba build: NOT VERIFIED.
- mGBA / physical GBA execution: NOT VERIFIED.


## v1.5 — Experimental ARM7TDMI ROM image

v1.5 experimental Clang/LLD cartridge build
- Added isolated ARM7TDMI bare-metal build, startup, data/BSS setup and custom linker layout.
- Added direct-register GBA graphics/keys/VBlank DMA bridge and minimal C runtime.
- Added ARM EABI division/modulo stubs plus host regression tests.
- Added binary header, ROM padding, memory-limit, ELF-structure and checksum checks.
- Fixed 8-bit writes to palette RAM in the regular initialization pathway; initial
  player OBJ palette now uses 32-bit DMA.
- First 1 MiB experimental .gba test file generated with 599,500-byte code/data payload.
- Real mGBA boot, physical GBA gameplay and devkitARM/libgba build NOT verified.


## v1.4 — Source-derived bonus moving platforms

- Extracted original moving-platform and cloud-platform animation resources.
- Added a source-accurate wider moving-platform OBJ graphic derived deterministically from the original 48×8 sprite and source instance 4/3 scale.
- Implemented two source-anchored moving one-way platform actors with prototype movement, descending landing, player carry, and reset.
- Removed moving-platform tiles from the static collision grid to prevent invisible platforms left behind.
- Added dedicated tests and actual C-trace-driven motion previews.
- Kept authentic trajectories and gameplay rules explicitly unverified.

Verification: host tests PASS, 33 ARMv4T/Thumb translation units PASS with test-only headers; **no real devkitARM build or mGBA boot**.

## v1.3 — Source-anchored bunny prototypes and bumper geometry

- Implemented separate, deterministic movement state for two original `obj_bunny` instances. Nine `obj_bunny_ground` placements stay static; no original racing AI or prizes are implied.
- Added small ±44px patrol and periodic hops with fixed-point tile probes; updates are limited to an active race.
- Added provisional collision and rebound for six square and four corner bumpers. Generated collision AABBs preserve source transforms including rotated corners.
- New host tests cover bunny initialization, movement, hopping/reset, square and diagonal bumpers, cooldown suppression and rotated hitboxes.
- Compiled host bunny-motion CSV and original-asset side-by-side preview; the preview is not an emulator screenshot.
- ARMv4T/Thumb source checks: 32 units with test headers. No real GBA ROM or mGBA boot is claimed.


## v1.2 — Original bunny, bumper, Swolvy blocks and scrolling QA

- Recovered four additional original source sprites from user-supplied GameMaker data: four-frame bunny hop, Swolvy-only block, square bumper and triangular bumper.
- Updated source sprite exporter with `--only-sprites` and regenerated a shared 41-set/32-color 8bpp OBJ package; converted frames were integrated into the on-screen bonus actor cache.
- Added source-art render support for two bunny and nine bunny-ground placements, two large Swolvy blocks, six square and four triangular bumper placements. Bunny and bumper behavior remains intentionally **not** inferred from absent source scripts.
- Added dedicated `TILE_SWOLVY_BLOCK=9`; normal Volvy can no longer break it, while powered-up Swolvy can. Eight collision cells belong to this type.
- The provisional automatic five-onion Swolvy test trigger is disabled in the bonus race; the supplied room traces record onions but no Swolvy frames.
- Extracted the 64×32 BG streaming window origin/packing into testable core functions and added viewport coverage tests for W1 and tall bonus room scenes.
- Added full integer-camera static actor density audit: 969,217 views, maximum 101 estimated hardware OAM entries including 8 reserved player parts (128 available). The test does not model per-scanline OBJ cycles.
- Added previews reconstructed from compiled GBA 8bpp background tile/palette/map data and original source sprite images. All previews explicitly **not emulator screenshots**.

Verification: host C tests PASS; 30 ARMv4T/Thumb units codegen/relocatable linking PASS with stubs; no real devkitARM/libgba ROM build or mGBA boot performed.

## v1.1 — World 1 bonus/race gameplay integration

- Added a `GAME_BONUS` mode reachable from the development title selector.
- Loaded the source-derived 332×70 bonus-room collision grid and implemented two-axis 64×32 BG map streaming.
- Added source-instance onion collection and rendering for 397 placements.
- Added 7 springboard interactions with a provisional -4.5 px/frame launch and 16-frame cooldown.
- Added 42 action-block animations and Big Hand rebound with a provisional -2.5 px/frame response and 24-frame cooldown.
- Added static squirrel-walk rendering for six source placements; competitor AI is not implemented.
- Added source-instance breakable removal for standard, one-way and Swolvy-only block AABBs.
- Added one-way landing and breakable-one-way ground-pound behavior.
- Added a local race timer triggered by source start-gate overlap and a finish X-plane derived from the trace event.
- Added regression tests for springboards, action blocks, standard/Swolvy block removal, onions, gates and timer behavior.
- Updated collision provenance, source audit, architecture, verification and preview documentation.

### Verification

- Host C tests and syntax checks: PASS.
- ARMv4T/Thumb code generation and relocatable link with test-only headers: PASS (29 translation units).
- Makefile parser check with a parser-only `gba_rules` stub: PASS.
- Real devkitARM/libgba ROM build: NOT VERIFIED.
- mGBA / physical GBA execution: NOT VERIFIED.

## v1.9.1 — Static OBJ animation-frame synchronization

- Preserve the verified v1.9 game, world layouts, source graphics and invisible marker fix.
- Synchronize the frame selected for static actor OAM with its VBlank tile upload.
- Reserve the 6144-byte shared DMA budget deterministically; defer full frames that do not fit.
- Add stand-alone host scheduler unit tests and renderer integration regression.
- Rebuild/test experimental ARMv4T cartridge image; **emulator boot still unverified**.
