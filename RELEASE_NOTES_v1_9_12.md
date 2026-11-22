# Volvy's Adventure: ReSlimed GBA — v1.9.12 source milestone

## Implemented

- Optional developer-only Mode 4 viewer for the authentic Fuzz Forest card converted from the 10:30 walkthrough frame.
- Release builds leave the preview disabled. With a locally generated private asset bundle, make DEV_CARD_PREVIEW=1 allows L on the title menu to show the card and B/START to dismiss it.
- Separate reusable 8bpp/256-color rendering module with BGR555 palette upload and 240x135 content centered on 240x160 (12/13-pixel margins).
- Direct decompression into the final aligned WRAM frame buffer, avoiding an additional 32,400-byte temporary buffer.
- Fix the existing Stage Clear return path so that BG tile caches, original HUD glyphs and HUD icon colors are restored after Mode 4 has overwritten the shared GBA VRAM.
- Cache reconstructed card data by compressed-data and palette identity rather than by mutable descriptor address.
- Public C regression tests for the card module, transition contract, asset conversion and previous gameplay systems; isolated ARM7TDMI/Thumb code generation.

## Verification

GitHub Actions run 45070191433 completed successfully (both jobs PASS):
https://github.com/Dynamon91/Volvy-s-Adventure-reslimed-GBA-port/actions/runs/45070191433

The public host tests exercise generated/synthetic data; ARM7TDMI tests compile the isolated display module. They do NOT verify a full link, running build, mGBA screen or physical GBA.

## Outstanding

- A complete devkitARM/libgba ROM build with authorized original game assets.
- Boot and gameplay tests in mGBA.
- Original handheld-card transitions and gameplay routing. The viewer deliberately does not claim Fuzz Forest equals the existing W1-L2 scene.
- Fully playable Fuzz Forest, result scoring/combos/ranks and remaining bosses.

Progress estimate remains **44%** for the full port. This milestone increases test coverage and repairs graphics transitions, but does not add a verified new level.
