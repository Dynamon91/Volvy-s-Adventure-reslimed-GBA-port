# Historical v1.5 experimental ROM note (superseded by v1.6)

The v1.5 1 MiB payload figures below refer to the earlier experimental build. Current v1.6 build status is documented in `docs/verification.md` and `docs/visual-fidelity-v16.md`.

# v1.5: experimental ARM7TDMI ROM image

This milestone provides the **first locally linked `.gba` file** for the current Volvy C source.
It is **not** a devkitARM/libgba build, and **not yet tested in mGBA or on a real GBA**. The
new `experimental/clang-gba` directory is an optional standalone bridge for situations
where devkitARM is unavailable, not a replacement for the supported SDK build target.

## Build from source (requires converted assets)

The normal libgba build remains `make` after installing devkitPro (`devkitARM`, `libgba`,
`gba_rules`). The alternative build uses Clang 17+, LLD, llvm-objcopy, Python 3 and
ELF `readelf` for structural testing:

```sh
sh tests/run_experimental_clang.sh
```

It compiles all C source/converted assets to ARMv4T/Thumb, links a tiny ARM cartridge
bootstrap, copies initialized data into EWRAM, clears BSS, and switches into Thumb
`main`. Its minimal GBA register layer uses direct MMIO, VCOUNT VBlank polling and
DMA3 for aligned sprite/palette/BG transfers. Minimal memcpy/memset and ARM-EABI
integer division helpers are included because an ARM newlib/compiler-rt installation
is not available. These components are experimental and could differ from libgba's
behavior. They have only compile, link and host tests at this point.

### Link/image checks (2026-11-22)

| Property | Local measurement |
|---|---:|
| ARM ELF entry | 0x08000000 |
| ROM startup branch target | 0x080000C0 |
| ELF class/machine | ELF32, ARM EABI |
| C translation units | 34, including standalone runtime |
| Game code | VLVY |
| ROM header fixed byte | 0x96 |
| ROM header complement | valid |
| Compiled payload | 599,500 bytes |
| Padded cartridge image | 1,048,576 bytes (1 MiB) |
| EWRAM `.data` + `.bss` | 36,608 bytes (plus stack in IWRAM) |
| Unresolved link symbols | none (link succeeds) |

`tests/verify_experimental_rom.py` checks the binary image header and ARM ELF layout.
`tests/test_experimental_minicrt.c` tests the C division/word-store routines on the host.
**These do not replace emulator execution or gameplay tests.** A ROM may have a valid
cartridge header and still hang, fail to render, or have hardware-specific bugs.

### ROM-testing checklist

1. Load `volvy_reslimed_v15_experimental.gba` in mGBA, preferably with BIOS emulation
   enabled. Record whether it reaches the title screen without a black/white screen.
2. Press Start; verify sprite colors (including HUD white) and Game Boy Advance 240×160
   framing. Move and jump in World 1 Level 1; inspect collisions and scrolling.
3. From the title, press Select to switch to Level 2, Level 3 and the bonus race. Check
   whether 32 KiB background uploads cause flicker and whether the 70-tile-high bonus
   room scrolls vertically as well as horizontally.
4. Inspect moving platforms, onion pickup, bunny animations, the race timer, Shrewy
   boss rendering and Start pause/resume behavior.
5. Capture screenshots/logs of failures, plus the exact emulator version and settings.

### Accuracy and distribution

Converted sprite, tilemap and palette arrays were produced from the game archive supplied
for this project and are intentionally excluded from the **public GitHub repository**.
The experimental ROM containing these converted assets is a **local test artifact**.
The user-facing project ZIP retains the generated arrays for continued development.
The original commercial game archive (`data.win` / RAR) is never packaged.

Production requirements remain: a genuine devkitARM/libgba compilation, mGBA execution,
console-compatible timing, collision-mask fidelity, authentic AI/hazards/boss scripts,
intro/audio/save logic, and a playable full-game QA pass.
