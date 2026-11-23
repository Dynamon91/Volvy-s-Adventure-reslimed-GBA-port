# Public CI scope

The public GitHub repository intentionally excludes the private GameMaker-derived room, collision, sprite and tile bundles. As a result, tests/run_host_tests.sh and the complete experimental cartridge builder require the private generated-asset package and are not suitable as public CI jobs.

The public workflow therefore runs two narrower, honest checks:

1. tests/run_public_tests.sh checks release input flags (the non-original SELECT stage-cycling shortcut must remain disabled by default), compiles asset-independent gameplay utilities and scene-cache/presentation tests; tests the three-entry title-menu navigation/dispatch module; runs the World 1 interaction module against synthetic public objects to cover springboards, stomps, damage, invulnerability, punches, action cooldowns and fatal-frame termination; exercises the OBJ-parts helper with a test-only declaration header that mirrors the public sprite generator's struct layout (no private sprite payload is included); runs the Stage Clear C module against deterministic host-side GBA memory/DMA and LZ77 stubs; syntax-checks the public Stage Clear module; and runs source/asset regression tests whose inputs are present in this repository.
2. tests/run_public_arm_check.sh compiles the Stage Clear and handheld-card C modules and its embedded LZ77 asset for ARM7TDMI/Thumb using the experimental GBA shim.

These checks do not compile or link the complete **game** ROM or verify boot/rendering in mGBA or on hardware. A separate nine-card **asset-viewer** ROM is now linked using synthetic inputs in the ARM job; this does not contain original game scenes, collisions, objects or gameplay. The private full test suite and complete ROM build remain separate validation gates when the generated asset bundle and devkitARM/libgba toolchain are available.

The public host suite also compiles and runs the 256-color handheld-card
Mode 4 runtime against synthetic image/palette data, and checks that optional
card-preview controls stay behind DEV_CARD_PREVIEW=1. This does not confirm
the complete private-asset ROM or mGBA video output.

The synthetic standalone viewer regression independently links an ARM7TDMI
cartridge with nine fake 8bpp screens, checks title/game code, header checksum,
power-of-two ROM size and reproducibility manifest, and checks rejection of a
truncated BIOS-LZ77 stream. The same source also built a real-private-frame
256 KiB reference-card viewer outside GitHub. That separate artifact is not
a full game build and has not been run in mGBA or on hardware.
