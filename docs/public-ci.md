# Public CI scope

The public GitHub repository intentionally excludes the private GameMaker-derived room, collision, sprite and tile bundles. As a result, tests/run_host_tests.sh and the complete experimental cartridge builder require the private generated-asset package and are not suitable as public CI jobs.

The public workflow therefore runs two narrower, honest checks:

1. tests/run_public_tests.sh compiles asset-independent gameplay utilities, scene-cache/presentation tests and OBJ helpers; runs a host runtime test of the Stage Clear C module with deterministic GBA memory/DMA and LZ77 stubs; syntax-checks the public Stage Clear module; and runs source/asset regression tests whose inputs are present in this repository.
2. tests/run_public_arm_check.sh compiles the Stage Clear C module and its embedded LZ77 asset for ARM7TDMI/Thumb using the experimental GBA shim.

These checks do not compile the complete game renderer, link a complete ROM, or verify boot/rendering in mGBA or on hardware. The private full test suite and complete ROM build remain separate validation gates when the generated asset bundle and devkitARM/libgba toolchain are available.
