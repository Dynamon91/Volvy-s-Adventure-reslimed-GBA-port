# Handheld selection-screen reconstruction — v1.9.11

## Source frames checked

The six user-supplied MP4 chunks were rejoined and checked as a 640×360, 30 FPS
H.264 walkthrough lasting 2:22:14.076. The following timestamps are offsets in
that recording, not in-game clocks.

| Offset | Directly visible selection/title card | Device appearance | Visible progress/state |
|---|---|---|---|
| 00:00 | Volvy title screen; Start Game / Settings / Quit | Framed title artwork | Start Game pointer |
| 10:30 | Fuzz Forest | Green horizontal handheld | 0/4 |
| 16:30 | Bonus Stage | Green horizontal handheld | ×20 reward |
| 57:00 | Rigatoni Rooftops | Pink handheld shell with YOLVY wordmark | 0/4 |
| 81:30 | Course title partly legible; likely Macho Metro | Pink handheld shell with YOLVY wordmark | 1/4 |
| 111:30 | Bonus Stage | Teal handheld | 1/1 |
| 114:00 | Fishpig | Pink handheld shell with YOLVY wordmark | Boss card |
| 124:00 | Bundtcake Burrows | Green horizontal handheld | 4/4 |
| 126:30 | Master Gator | Teal handheld | Boss card |

The 57:00 frame resolves a previous documentation error: the card reads
**Rigatoni Rooftops**, not Big City Rooftops. The 81:30 course title remains
uncertain; do not treat “Macho Metro” as fully verified from this frame alone.

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
claimed. In particular, Fuzz Forest is not mapped to World 1 Level 2 without
transition evidence. The source-derived images, binaries and per-card palettes
remain in the private asset bundle; the public repository receives only the
reproducible converter, tests and documentation.
