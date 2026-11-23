# Mode 4 transfer tearing guard — 2026-11-23

The Stage Clear screen and the standalone handheld-card viewer each upload
240 x 160 = 38,400 bytes of 8bpp pixels into BG VRAM. Although the transfer
is initiated during VBlank, it cannot be assumed that an entire DMA fits
inside the VBlank period. An overrun could display a partially updated
bitmap or tear while changing cards.

Both display modules now set **DISPCNT bit 7 (forced blank)** before copying
the palette and pixel buffer. They clear forced blank only after both DMA
transfers have completed, by setting `MODE_4 | BG2_ON`. The LCD remains
blank during the transfer even if VBlank ends.

This does not imply a measured constant-time VBlank transfer. A full-screen
DMA can still consume visible scanline time, but forced blank prevents
displaying an incomplete frame. The existing Mode 0 restoration and BG
cache/HUD reconstruction remain required when exiting bitmap mode.

Public host regressions check the ordering of the display-mode calls, and
the ARM7TDMI job compiles the revised modules. A visual mGBA/hardware test
is still needed for flicker and input latency.

**Scope:** reliability fix only; no new playable levels. Overall port
completion estimate remains 44%.
