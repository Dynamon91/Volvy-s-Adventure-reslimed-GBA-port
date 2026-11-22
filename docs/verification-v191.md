# v1.9.1 verification — 22 Nov 2026

The supplied v1.9 ROM is preserved unchanged (SHA-256 `caab13e2988eca40aade3145643249df592ab7e5e4ab4b9d65ccde19e22ca7f2`).

New experimental ROM: `Volvy_Reslimed_GBA_v1.9.1_experimental.gba`.
SHA-256: `1ecd8ce07c64525207d079fa27455ca7b70a939100edaf0bf8843310ee3bf431`.

## Code and test results

- `sh tests/run_host_tests.sh`: **PASS**, including the new unit and renderer integration checks.
- `sh tests/run_arm_codegen.sh`: **PASS**, 41 C translation units with test-only ARM headers.
- `sh tests/run_experimental_clang.sh`: **PASS**, 42 linked C source files, ROM header/ELF structure and 2 MiB output.
- Original v1.9 ROM still matches user-supplied SHA-256.

## Limitations

- ROM is an experimental Clang/LLD bare-metal build using test headers, **not** a devkitARM/libgba build.
- No mGBA run, frame screenshot or physical-GBA boot occurred in this environment.
- The static OBJ frame/OAM mismatch is addressed in code; its visual effect needs a real emulator video.
- The preview collage uses source-derived graphics from the archived project. It is not a gameplay capture from the new ROM.

## Suggested v1.9.1 mGBA video

Open the new `.gba` in mGBA; capture a continuous 1–2 minute run that demonstrates moving/jumping in W1-L1, onion/squirrel/springboard animations, the vertical camera transition, HUD at the top edge, and the location of the old green hole marker. If possible, switch to Bonus Race to show bunny/platform animations. Check for single-frame shape/offset flicker when animating objects.
