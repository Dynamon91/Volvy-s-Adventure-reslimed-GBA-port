# Verification — v1.9 invisible source-marker cleanup

## v1.9 capture review and correction

- Reviewed the newly uploaded 112.766-second 1920×1080 30 FPS mGBA recording. It shows a running Volvy GBA build, but no ROM filename/checksum is visible; it cannot identify the exact revision.
- The bright-green platform square in the capture matches `spr_hole`: its 16×16 frame is 256 identical palette-index-36 pixels, with BGR555 `0x33E6` (RGB 49,255,98).
- `WORLD_HOLE` and bonus `HOLE`/`HOLE_DIR` are now excluded from runtime OBJ drawing. The source placements and source sprite remain available for auditing.
- The `spr_hole` VRAM slot was removed. Active visible static cache: 25 animations / 18,560 bytes; player reserve: 5,120 bytes; combined: 23,680 bytes; free OBJ VRAM: 9,088 bytes.
- `tests/test_invisible_marker_assets.py` checks the source marker pixel data/palette and confirms it is not mapped into the world or bonus renderer.

## v1.9 checks completed in this environment

| Check | Result |
|---|---|
| Host C regression suite | PASS |
| Source marker asset/render exclusion test | PASS |
| Native scene cache and presentation tests | PASS |
| HUD font and OAM priority regressions | PASS |
| ARMv4T/Thumb code generation | PASS — 40 translation units with test-only headers |
| Experimental Clang/LLD ROM structural verification | PASS — 2 MiB image, 1,240,652-byte payload, 41 C files |
| devkitARM/libgba build | NOT VERIFIED |
| v1.9 mGBA boot/visual retest | NOT YET TESTED |
| Physical GBA test | NOT TESTED |

The corrected visual preview in `previews/v19_hole_marker_before_after.png` is a source-scene reconstruction using a camera crop matched to the source scene and original background pixels. It is not a screenshot captured from the rebuilt v1.9 ROM.

---

# Verification — v1.8 HUD priority correction

## Historical v1.8 review and fix

- User capture reviewed: 117.5 seconds, 1920×1080 recording, mGBA window titled `VOLVY RESLIM (60 fps)`.
- Visible gameplay states include title, W1-L1, onion counter updates, breakable blocks, pipe sections, normal Volvy, Swolvy, Big Hand movement and vertical camera travel.
- Defect reproduced from captured frames: foreground OBJ sprites can cover the top HUD because gameplay OBJ priority 0 ties BG0 priority 0, and OBJ wins an equal-priority tie.
- Fix: gameplay player/world/bonus/boss OBJ entries now use priority 1. BG0 HUD stays priority 0; BG1 scene stays priority 1. Title Volvy remains priority 0.
- `tests/test_oam_priority_v18.py` verifies the priority assignments and BG ordering.

## v1.8 checks completed in this environment

| Check | Result |
|---|---|
| Host C regression suite | PASS |
| OAM/HUD priority regression | PASS |
| Existing scene/cache/collision tests | PASS |
| ARMv4T/Thumb code generation | PASS — 40 translation units with test-only headers |
| Experimental Clang/LLD build | PASS — 41 C files |
| GBA header/logo/checksum/ELF structure | PASS |
| devkitARM/libgba build | NOT VERIFIED |
| v1.8 mGBA boot and HUD visual check | NOT YET TESTED |
| Physical GBA test | NOT TESTED |

The user's MP4 validates that a build runs in mGBA, but does not prove the newly rebuilt v1.8 binary boots. The ROM version is not identifiable from the video alone because no checksum or filename is shown.

## Historical v1.7 verification — native-pixel fidelity and stability

## v1.7 new checks

- The native BG presentation state now distinguishes a staged map origin from the origin already committed to BG1 VRAM. `tests/test_native_presentation.c` covers 344,000+ camera positions, incremental scrolling in both axes and distant teleport swaps.
- The actual source `fnt_small` GBA glyph tiles comprise 39 characters plus a blank tile (1,280 bytes), all 1:1 and using 4bpp palette index 1 for opaque pixels. `tests/test_hud_font_asset_v17.py` verifies each tile's populated pixels against its extraction manifest.
- The boss onion VRAM region [6144,7168) is now disjoint from the player region [0,5120), boss body graphics [8192,10496) and rock graphics [24576,25600). `tests/test_obj_vram_layout_v17.py` also checks 8bpp OBJ tile-index alignment and maximum original animation-frame sizes.
- Experimental Clang/LLD v1.7 build compiles 41 C files and passes the binary header, checksum and ELF section verifier. ARMv4T/Thumb code generation passes for 40 units with test-only headers.
- The user's 117.5-second MP4 shows a running Volvy GBA build in mGBA (title screen and W1-L1 gameplay; the window reports 60 fps). The video does not include a ROM hash, so the exact binary revision cannot be independently confirmed. The v1.8 ROM has not been booted in mGBA.

## Verified in this environment

| Check | Result |
|---|---|
| Host C regression suite | PASS |
| Native scene cache/window tests using all five generated scenes | PASS |
| Source 4bpp scene round-trip after BGR555 quantization | PASS — 0 differing pixels in all five scenes |
| Native player/actor asset layout | PASS — 56 animation sets, 71 OBJ colors |
| Static actor cache budget | PASS — 18,816 bytes + 5,120-byte player reserve = 23,936 bytes |
| Bonus OAM estimate | PASS — 66/128 including eight player parts |
| World 1 L1/L2/L3 OAM estimates | PASS — 58/128, 66/128 and 82/128 |
| ARMv4T/Thumb code-generation check | PASS — 40 translation units using test-only GBA headers |
| Experimental Clang/LLD bare-metal build | PASS — structurally valid 2 MiB `.gba` image |
| GBA logo/header/checksum and ELF section checks | PASS |

### Native scene round-trip

`tools/build_native_scene_gba.py --preview-out ...` reconstructs a PNG from the generated 4bpp tile bytes, map entries, H/V flip flags and BGR555 palette. All five scene outputs compare pixel-identically to their source composites after the same BGR555 quantization.

### OBJ VRAM

- Player frame reservation: 5,120 bytes.
- One-frame-per-animation static actor slots: 18,816 bytes.
- Combined static/player reservation: 23,936 of 32,768 OBJ VRAM bytes.
- Remaining OBJ VRAM: 8,832 bytes.
- The current source set includes 26 static actor animation sets. Their frame data is streamed into fixed slots in bounded VBlank batches.

### OAM estimates

The native-resolution audit counts metasprite parts, not just actor instances. It evaluates all integer camera origins for the source rooms and reserves eight OBJ entries for the player.

| Room | Estimated maximum OBJ entries |
|---|---:|
| World 1 L1 | 58 / 128 |
| World 1 L2 | 66 / 128 |
| World 1 L3 | 82 / 128 |
| Bonus Race | 66 / 128 |

These are conservative screen-rectangle/part-count estimates. The hardware's separate 32-OBJ-per-scanline limit and cycle budget have not been exhaustively verified.

### Experimental ROM

The separate Clang/LLD build compiles 41 C files and creates a 2 MiB (2,097,152-byte) GBA image with a 1,239,856-byte payload. The verifier checks the ARM entry, Nintendo logo prefix, header checksum, file sizing and ELF load sections. It is a bare-metal experimental runtime, not a `devkitARM`/`libgba` build.

## Not verified

- Standard `devkitARM` + `libgba` `make` build.
- Boot, title screen, gameplay, audio or save behavior in mGBA.
- Physical GBA boot or timing.
- Per-scanline OBJ limit and actual VBlank timing on hardware.
- Original enemy AI, damage reactions, race scoring/rewards, moving-platform paths, live parallax, full stage transitions, intro/menu, music and save persistence.

**Important:** A structurally valid `.gba` image is not proof that the game boots or is playable. Do not label the experimental image hardware-verified until mGBA or real-hardware testing succeeds.
