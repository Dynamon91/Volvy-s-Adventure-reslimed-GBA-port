# Public CI scope

The public GitHub repository intentionally excludes the private GameMaker-derived room, collision, sprite and tile bundles. As a result, tests/run_host_tests.sh and the complete experimental cartridge builder require the private generated-asset package and are not suitable as public CI jobs.

The public workflow therefore runs two narrower, honest checks:

1. tests/run_public_tests.sh checks release input flags (the non-original SELECT stage-cycling shortcut must remain disabled by default), compiles asset-independent gameplay utilities and scene-cache/presentation tests; runs the World 1 interaction module against synthetic public objects to cover springboards, stomps, damage, invulnerability, punches, action cooldowns and fatal-frame termination; exercises the OBJ-parts helper with a test-only declaration header that mirrors the public sprite generator's struct layout (no private sprite payload is included); runs the Stage Clear C module against deterministic host-side GBA memory/DMA and LZ77 stubs; syntax-checks the public Stage Clear module; and runs source/asset regression tests whose inputs are present in this repository.
2. tests/run_public_arm_check.sh compiles the Stage Clear C module and its embedded LZ77 asset for ARM7TDMI/Thumb using the experimental GBA shim.

These checks do not compile the complete game renderer, link a complete ROM, or verify boot/rendering in mGBA or on hardware. The private full test suite and complete ROM build remain separate validation gates when the generated asset bundle and devkitARM/libgba toolchain are available.
