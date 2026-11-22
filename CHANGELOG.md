# Changelog

## v1.5 — First experimental ARM7TDMI cartridge image

- Built a 1 MiB GBA cartridge image with Clang/LLD and the user's locally converted game assets (599,500-byte payload).
- Added a standalone ARM startup, ROM/EWRAM linker layout, GBA direct-register layer, and EABI division helpers.
- Added ROM structure, checksum, RAM and host CRT regression tests.
- Fixed 8-bit palette clearing and replaced initial OBJ palette memcpy with aligned DMA.
- This ROM has NOT yet been boot-tested in mGBA or compiled with real devkitARM/libgba. The ROM and commercial converted graphics are deliberately not pushed to this public repository.


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
