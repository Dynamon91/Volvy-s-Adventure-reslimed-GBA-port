# Volvy GBA — v1.9.1 (recovered v1.9 baseline)

**Latest recovered source-backed milestone:** v1.9.1. The user-provided v1.9 archive and experimental ROM were used as the starting point. This commit does not restart the port from the earlier v1.5 baseline.

## Direct GitHub downloads

- [Complete v1.9.1 source, assets, tests and ROM](../releases/Volvy_Reslimed_GBA_v1.9.1_source_and_test.zip) — SHA-256 `f06923952e9b47a0d399729d8551ead36c5fc2a29857162766f926445d0b60f9`
- [Experimental v1.9.1 ROM (.gba)](../releases/Volvy_Reslimed_GBA_v1.9.1_experimental.gba) — SHA-256 `1ecd8ce07c64525207d079fa27455ca7b70a939100edaf0bf8843310ee3bf431`
- [Source-asset preview PNG (not an mGBA screenshot)](../previews/Volvy_GBA_v1.9.1_source_preview.png)

## Fix and verification

The new static-frame scheduler co-ordinates OBJ graphics and OAM geometry during VBlank and defers full sprite-frame transfers when the static-actor 6144-byte transfer budget is exhausted. Host tests, ARMv4T/Thumb codegen and experimental ROM compilation passed locally. **An actual mGBA boot, gameplay fidelity, sprite flicker, GBA hardware and devkitARM/libgba builds remain unverified.**

For details see [release notes](../RELEASE_NOTES_v1_9_1.md) and [verification](./verification-v191.md).

The former `main` head was backed up to branch `backup-v1.5-pre-v1.9.1` before this milestone was published.
