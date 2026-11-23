# Volvy's Adventure GBA — walkthrough-card viewer

This is an independent **technical ROM**, not the playable port or the original
selection hub. It uses the same 8bpp/256-color Mode 4 renderer as the port but
contains only the nine selection/title frames already converted from the
user-provided 2h22 walkthrough. There is no platforming, physics or audio.

**Controls:** R or RIGHT goes to the next frame; L or LEFT goes to the previous
frame; START returns to the first/title frame. The ordering follows recording
timestamps; it is not a claim about original hub navigation or stage routes.

## Build with the private frame pack

From the project's root:

    python3 experimental/handheld-card-viewer/build.py \
      --assets-zip /path/to/volvy_gba_v1911_handheld_card_assets.zip \
      --out build-handheld-card-viewer

Requires clang (ARMv4T), ld.lld and llvm-objcopy. The builder validates BIOS
LZ77 headers, 240x135 8bpp source geometry and 256-color palette metadata for
all nine cards. It creates an ELF, a valid-header cartridge image (.gba), a map
and a JSON SHA-256 manifest. The tool reuses the project's ARM startup,
linker, libc shim and reusable Mode 4 screen module. Source-derived assets are
**not** committed to this public repository.

A standalone ROM build confirms only the ARM link, size and header. It does not
establish boot/rendering in mGBA or hardware. It does not replace the last
verified main-game release or complete any new gameplay stage.
