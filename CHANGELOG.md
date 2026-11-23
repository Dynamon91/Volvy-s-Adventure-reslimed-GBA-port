## v1.9.13 — first standalone 9-card GBA reference ROM

- Built a 256 KiB GBA image containing all nine source-derived 256-color title and handheld selection screens; ARM7TDMI/Thumb ELF link, ROM header and checksum PASS.
- Added reproducible private ZIP-based builder, asset hashes, independent LZ77 validation and a public synthetic-card full ROM link regression to GitHub Actions.
- Added L/R and D-pad left/right browsing; START returns to the first/title screen. This is a technical viewer, not a playable game or the original hub.
- Corrected the experimental GBA shim's R keypad bit (bit 8) and added L (bit 9).
- Actual rendered output in mGBA and physical GBA remains untested; the original game's full private-asset ROM still cannot be linked from the public repository alone. Port completeness estimate stays at 44%.

## v1.9.12 — Mode 4 reference-card viewer and VRAM restore repair

- Add a generic GBA 256-color Mode 4 renderer for the private selection-card assets, decompressing 240x135 8bpp content directly into a 240x160 EWRAM buffer with 12/13px letterbox margins.
- Add optional dev-only title shortcut L to preview the Fuzz Forest card; B/START returns to title. The release build has DEV_CARD_PREVIEW=0 and no private card references.
- Repair Stage Clear and card-preview exits: invalidate the BG tile cache and reload the HUD font, icon graphics and BG color banks after Mode 4 overwrites shared VRAM.
- Add public runtime C and ARM7TDMI module tests and source-contract checks.
- No new course gameplay, selection hub, or complete .gba/mGBA build is claimed.

## v1.9.11 — walkthrough handheld-card asset pipeline

- Rejoined and reopened the six supplied walkthrough segments; extracted full-resolution 640×360 reference frames for the title and handheld selection cards.
- Corrected the 57:00 selection-card title from the previous guess “Big City Rooftops” to the directly legible “Rigatoni Rooftops”.
- Added a reproducible converter for 240×135 16:9 content letterboxed to the GBA 240×160 screen, with 4bpp/16-color and 8bpp/256-color modes, BGR555 palette generation, BIOS-LZ77 compression, previews and source-hash manifests.
- Prefer 8bpp/256-color output for the complex handheld cards; the 16-color test loses significant device/course-preview detail.
- Add public host regression coverage for both pixel formats, LZ77 round-trip, palette bounds and 12/13px letterbox geometry.
- Source-derived card images and generated binaries remain private. The converter is not yet wired into a runtime selection state; no course routing or Fuzz Forest-to-W1 mapping is claimed.

## v1.9.10 — title-menu navigation scaffold

- Match the three title entries directly observed in the supplied walkthrough: Start Game, Settings and Quit.
- Add bounded Up/Down navigation and a visible focus bar using the existing original-font dash glyph.
- Connect A/Start to the existing Start Game path.
- Leave Settings and Quit non-dispatched pending source behavior evidence and a verified GBA mapping; no placeholder settings controls or unsafe reset/exit behavior are fabricated.
- Add public host C tests for navigation boundaries, invalid indices and dispatch.

## v1.9.9 — fatal-contact interaction guard

- Stop the world-interaction pass immediately after a fatal collision marks Volvy dead. A later overlapping action block can no longer apply a bounce in the same frame.
- Add an asset-independent host C harness with synthetic World 1 objects.
- Cover springboard crossing and cooldown, enemy stomp, side damage, invulnerability, forward punch, action-block cooldown, inactive mode and fatal-frame termination.
- Public host regression and isolated ARM7TDMI Stage Clear module checks are tracked in GitHub Actions. Full ROM build and mGBA gameplay remain unverified.

## Post-v1.9.8 development correction — release input fidelity

- Disable the development-only SELECT stage-cycling shortcut in normal builds.
- Compile the helper functions only when `VOLVY_DEV_STAGE_SELECT=1`, avoiding unused-function warnings in release builds.
- Expose an explicit Make variable: `make DEV_STAGE_SELECT=1` for test builds; default `make` keeps the original-game input surface free of the debug shortcut.
- Add a public source-contract regression to prevent the debug selector from silently becoming the default again.

# Changelog

## v1.9.8 test hardening — Stage Clear runtime coverage

- Add a host-side C runtime test for the actual Stage Clear module.
- Verify one-time preparation, low/high nibble expansion, 240×160 frame layout, 12px / 13px letterbox rows, palette and bitmap transfers, and Mode 0 restoration.
- Use a deterministic LZ77 test stub for the runtime module test; the separate asset test continues to validate the real BIOS-LZ77 stream.
- Make the framebuffer address overrideable only by the host test shim; the production default remains GBA VRAM at 0x06000000.
- GCC host runtime test, GCC syntax check and isolated ARM7TDMI/Thumb module code generation: PASS.
- Full ROM build, full renderer link and mGBA boot/render: NOT VERIFIED.

## v1.9.8 — walkthrough-derived Stage Clear screen

- Add a static Stage Clear result frame sampled from the user's walkthrough at 04:00.
- Convert the 240×135 reference frame to 16-color 4bpp pixels, BIOS-LZ77 compress it and include the binary asset in the project.
- Expand the packed pixels into a 240×160 Mode 4 bitmap with 12px / 13px purple margins.
- Gate the screen to the provisional W1-L1 victory route; do not reuse it for boss or bonus victories.
- Queue bitmap/palette writes for VBlank and restore Mode 0 before the next scene streams in.
- Add asset decompression, palette, dimensions and renderer-transition regression tests.
- BIOS-LZ77 round-trip, palette/layout inspection, renderer source-contract inspection, isolated C syntax and ARM7TDMI/Thumb object generation for the Stage Clear module/asset: PASS. Full ROM build and mGBA rendering: NOT VERIFIED.
- The captured result values/art remain static; dynamic combo, rank and score calculations are still missing.

## v1.9.7 — walkthrough-derived HUD icon pass

- Add 16×16 onion and stopwatch icons sampled from the supplied gameplay walkthrough.
- Pack both icons into eight 4bpp BG tiles immediately after the 40 source-font tiles.
- Preserve palette index 0 transparency and index 1 white for the original font.
- Add one-tile HUD margins and keep the timer group right-aligned.
- Use quotient/remainder centisecond conversion for the HUD cache key to avoid early multiplication overflow.
- Add icon-payload/layout regression coverage and extend the HUD contract.
- UI syntax with minimal GBA stubs and the two patch-level Python tests: PASS.
- Source-only update: no v1.9.7 ROM, ARM codegen, devkitARM build, mGBA boot or hardware test is claimed.
- Icon graphics are reconstructed from compressed walkthrough footage and are not claimed as bit-identical source sprites.
