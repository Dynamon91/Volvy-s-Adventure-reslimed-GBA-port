# Technical verification for v1.9.3 (2026-11-22)

## Evidence

- Portable tests including OBJ OAM clipping: PASS (`tests/run_host_tests.sh`).
- ARMv4T/Thumb relocatable compilation: PASS, 42 C translation units, with fake headers.
- Experimental Clang/LLD ROM: PASS, 43 C units, 2,097,152 bytes.
- GBA Nintendo logo prefix, header checksum, ELF sections: PASS.
- v1.9.3 ROM SHA256: `d40a89a77b1fc5acecbbbb433e92380c57f48ed45b079b6db188a5aa249dd792`.
- Original v1.9.2 sources/ROM retained unchanged in their prior archive.

## Not verified

Actual mGBA startup, frame timing, GBA flashcart, graphic quality after the
new source changes, original-game exactness and devkitARM/libgba compilation.
The PNG preview is sourced from project assets; it does not demonstrate gameplay.
