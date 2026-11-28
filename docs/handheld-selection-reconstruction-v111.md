# Handheld selection-screen reconstruction — v1.9.11

## Source frames checked

The six user-supplied MP4 chunks were rejoined and checked as a 640×360, 30 FPS
H.264 walkthrough lasting 2:22:14.076. The following timestamps are offsets in
that recording, not in-game clocks.

| Offset | Directly visible selection/title card | Device appearance | Visible progress/state |
|---|---|---|---|
| 00:00 | Volvy title screen; Start Game / Settings / Quit | Framed title artwork | Start Game pointer |
| 10:29 | Matcha Meadows | Green horizontal handheld | 3/4 |
| 10:30–10:33 | Fudge Forest | Green horizontal handheld | 0/4 |
| 16:30 | Bonus Stage | Green horizontal handheld | ×20 reward |
| 57:00 | Rigatoni Rooftops | Pink handheld shell with YOLVY wordmark | 0/4 |
| 81:30 | Course title most plausibly reads Macaroni Metro (81:30 zoom, provisional) | Pink handheld shell with YOLVY wordmark | 1/4 |
| 111:30 | Bonus Stage | Teal handheld | 1/1 |
| 114:00 | Fishpig | Pink handheld shell with YOLVY wordmark | Boss card |
| 124:00 | Bundtcake Burrows | Green horizontal handheld | 4/4 |
| 126:30 | Master Gator | Teal handheld | Boss card |

The 57:00 frame resolves a previous documentation error: the card reads
**Rigatoni Rooftops**, not Big City Rooftops. The 81:30 course title remains
better reads “Macaroni Metro” in the re-extracted 640×360 frame. Compression makes individual letters ambiguous; definitive spelling still requires original text assets.

The source cards use visibly different handheld shells. Their course previews,
progress fractions, reward markers, medal/thumb rows, side counters and device
controls are part of the captured screen. The meaning and input mapping of every
side counter/button have not yet been reverse-engineered.

## GBA asset pipeline added in v1.9.11

tools/build_handheld_select_asset.py converts a supplied walkthrough frame to
a private GBA bitmap asset:

- Requires a 16:9 source frame; preserves the full frame without cropping or
  stretching.
- Resizes to 240×135 with nearest-neighbour sampling.
- Adds 12 top and 13 bottom rows to fit the GBA 240×160 display.
- Supports packed 4bpp/16-color output and 8bpp/256-color output.
- Selection-card artwork uses 8bpp/256 colors because the 16-color test loses
  substantial detail in the device shell, course preview and small text.
- Converts the palette to GBA BGR555 and moves the closest source-corner color
  to palette index 0 for the letterbox rows.
- BIOS-LZ77 compresses the 240×135 content; the 8bpp raw content is 32,400
  bytes. The tested walkthrough cards compress to approximately 20–25 KB.
- Generates a .bin, palette .c/.h, 240×160 PNG preview, 3× nearest preview
  and JSON manifest containing the source SHA-256 and conversion data.

Example (run with private keyframes; generated artwork is not public):

python tools/build_handheld_select_asset.py private_keyframes/volvy_630s.jpg --name fuzz_forest --out assets/converted/ui/fuzz_forest --source-time 630 --colors 256

The palette source/header are written to assets/converted/; the compressed
binary and previews are written beside the output prefix under
assets/converted/ui/.

## Verification and scope

The new host regression tests both 4bpp and 8bpp output, BIOS-LZ77 round trips,
BGR555 palette bounds, 240×160 preview dimensions and the 12/13px letterbox.
The actual private frame set was converted into nine local card/title previews.

**This milestone is an asset-extraction pipeline, not runtime integration.**
No new selection state, card navigation, stage routing, or gameplay scene is
claimed. In particular, Fudge Forest is not mapped to World 1 Level 2 without
transition evidence. The source-derived images, binaries and per-card palettes
remain in the private asset bundle; the public repository receives only the
reproducible converter, tests and documentation.

## v1.9.12 — Development-only Mode 4 preview in the engine

The GBA now has a generic 8bpp Mode 4 card renderer, with deterministic
host tests. For an authorized local copy of the original reference frame,
run the converter (as documented above) to produce
assets/converted/ui/fuzz_forest.bin and
assets/converted/fuzz_forest_palette.c/.h. Build with:

    make DEV_CARD_PREVIEW=1

On the title screen press L to preview Fudge Forest. Press B or START to
return to the title. Release builds use DEV_CARD_PREVIEW=0 and do not
include the private card payload. This flow is a reference viewer only;
it is intentionally not inserted into the unverified W1 stage sequence.

The bitmap is committed during VBlank, not directly during gameplay.
Mode 4 writes to BG VRAM that previously contained tiled scene graphics
and the HUD font. On exit the renderer now invalidates the tile cache,
reloads BG palette and HUD glyph/icon tiles, and streams the scene tiles
again before exposing the normal Mode 0 backgrounds. This safeguard
also applies to the existing Stage Clear result screen.

Verification: public host test checks invalid headers, LZ77 call and
frame placement, 256 BGR555 palette slots, top/bottom letterbox and
Mode 0 restore; ARM7TDMI codegen checks the new module. Public source
contract checks the optional dev-only input flow and restoration ordering.

Limitations: no complete devkitARM/libgba ROM build or mGBA visual test
with the private card assets; no new level or selection-hub progression.

## v1.9.14 — source-preserving resampling and hardware-accurate previews

The converter keeps **nearest** as its default because the original game is
pixel art. Two *opt-in* 256-color resampling modes are available for A/B testing:

- `--resample box`: averages source pixels before 240×135 quantization,
  reducing aliasing of small UI lettering and diagonal shapes, at the cost of
  softer pixel-art edges.
- `--resample lanczos`: higher-order filtered downscale; can retain line
  contrast but may introduce halos, so it is not the default.
- `--resample nearest`: existing pixel-art-preserving behavior; retains
  backwards compatibility with the v1.9.13 private card pack.

All three modes preserve the **same 16:9 framing** and the 12/13px margins.

**Preview correctness fix:** previously the PNG previews retained 24-bit
quantized colors even though the GBA palette stored only 5 bits per R/G/B
channel. Previews now use the exact generated BGR555 palette re-expanded to
RGB888, making the preview accurately represent the output palette rather
than promising unavailable colors.

Public host tests cover both 4bpp and 8bpp, all three filters, BIOS-LZ77
round-trips, letterbox pixels and exact 5-bit preview palette matching.
This is a fidelity study, **not a claim that filtering restores lost 640×360
detail**; screenshots should be checked in an emulator before selecting the
best option for gameplay.

## v1.9.14 — frame reading audit

The user-supplied six-part walkthrough was rejoined again and frames were
extracted directly at the nine selection-card timestamps, this time as PNG
rather than intermediate JPEG. At 81:30 the pixel lettering is better read
as **Macaroni Metro**, correcting earlier notes that guessed “Macho Metro”.
This is a visually supported reading, **not a claim that source string tables
have been decoded**. The internal asset filename remains
`selection_81m30` for backward compatibility.

## 28 November 2026 source-accuracy correction

New direct PNG checks at 10:29, 10:30, 10:31 and 10:33, backed by
the names already recorded in `docs/source-audit.md`, establish
**Matcha Meadows** followed by **Fudge Forest**. The older transcription
“Fuzz Forest” was incorrect. The internal binary/header identifiers
`fuzz_forest` are **legacy filenames**, retained only so the v1.9.12
developer-card preview and v1.9.13 standalone viewer continue to build.
A rename must migrate the private .bin, .c, .h and generated references
together, not break them by changing public includes in isolation.

A 32-frame, four-second **video-derived GBA palette preview** was also
generated privately from 10:36–10:40 (240×160, 256 stable BGR555 colors,
12/13px letterbox). It is not a captured GBA emulator frame and does not
constitute an implemented level. See `docs/fudge-forest-reference-audit.md`.
