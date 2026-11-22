# Volvy GBA v1.9.2 experimental ROM status

- Base: recovered v1.9 → v1.9.1 sprite frame coherence → v1.9.2 off-screen frame budget fix.
- Compiler: experimental Clang/LLD ARM7TDMI bare-metal pipeline (not devkitARM/libgba).
- Expected ROM image size: 2,097,152 bytes (2 MiB).
- Host logic/asset tests: PASS (see tests/run_host_tests.sh).
- ARMv4T/Thumb code generation: PASS with test headers/stubs.
- Experimental ROM GBA header, checksum, ELF sections: PASS.
- devkitARM/libgba build: NOT VERIFIED.
- mGBA boot and animation/HUD retest: NOT TESTED.
- Physical GBA: NOT TESTED.

The source-derived preview is **not** an emulator screenshot; it cannot prove
actual frame timing, scrolling quality or execution speed.
