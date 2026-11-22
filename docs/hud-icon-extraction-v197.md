# HUD icon extraction notes — v1.9.7

## Reference frame

- Source: the six sequential user-uploaded walkthrough segments.
- Combined reference: 601,191,189 bytes; 2:22:14.076; H.264 640×360 at 30 FPS with AAC audio.
- Sample frame: video offset **00:02:00**.
- Direct observation: onion icon + `X213` at upper-left; stopwatch + `1:50:07` at upper-right.

## GBA representation

- Each icon is 16×16 pixels.
- Each 16×16 icon is stored as four row-major 8×8 4bpp tiles.
- Onion tiles occupy indices 40–43; stopwatch tiles occupy 44–47.
- The existing font occupies indices 0–39. A compile-time guard prevents overlap if the font tile count changes.
- Palette bank 15 uses index 0 for transparency and index 1 for the existing white font. Onion-specific colors occupy indices 2–7; stopwatch-specific colors occupy indices 8–15.
- HUD placement: onion icon at map cell (1,1), count at (3,1); stopwatch and timer are right-aligned as a single group on row 1.

## Limitations

The walkthrough is a compressed 640×360 capture. The icon tiles are sampled and reduced from that frame; they are not asserted to be bit-identical to the original GameMaker HUD sprites. The original icon sprites should replace these tiles if/when their source atlas is available.

The icons do not reconstruct the title logo, stage-clear artwork, boss-clear artwork, combo counter, placement/ranking system, selection handheld or hub. Those remain separate fidelity tasks.
