# Mode 4 transfer tearing guard — 2026-11-23

The Stage Clear screen and the standalone handheld-card viewer each upload
240 x 160 = 38,400 bytes of 8bpp pixels into BG VRAM. Although the transfer
is initiated during VBlank, it cannot be assumed that an entire DMA fits
inside the VBlank period. An overrun could display a partially updated
bitmap or tear while changing cards.

Both display modules set **DISPCNT bit 7 (forced blank)** before copying
the palette and pixel buffer. **Important correction:** libgba's `dmaCopy`
starts DMA3 but does *not* itself wait until the hardware finishes. Without
an explicit barrier, clearing forced blank directly after `dmaCopy` can
reveal incomplete graphics or overlap palette/framebuffer DMA3 operations.

The shared `include/platform/bitmap_dma_sync.h` now polls the DMA3 enable
bit (I/O register `0x040000DC`, bit 31). Both screen modules wait after
the palette copy, then wait again after the 38,400-byte image copy, **before**
setting `MODE_4 | BG2_ON` without forced blank.

This is an on-demand transfer synchronization at the moment a bitmap is
presented. It does not add a periodic wait to the gameplay update loop,
physics, movement, camera or world-object processing. The exact duration
on hardware and any visible blanking/flicker are not yet measured.

This does not imply a measured constant-time VBlank transfer. A full-screen
DMA can still consume visible scanline time, but forced blank prevents
displaying an incomplete frame. The existing Mode 0 restoration and BG
cache/HUD reconstruction remain required when exiting bitmap mode.

Public host regressions model DMA3 as asynchronous. They fail if the next
DMA starts while DMA3 is busy, or if forced blank is cleared before the last
DMA completes. They also verify two successful completion waits per bitmap
and the Mode 0 restoration. The ARM7TDMI job compiles both modules; the
standalone card viewer links from synthetic data. Actual mGBA/hardware
flicker and input-latency measurements are still outstanding.

**Scope:** reliability fix only; no new playable levels. Overall port
completion estimate remains 44%.
