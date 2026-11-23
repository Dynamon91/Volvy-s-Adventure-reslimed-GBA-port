# Volvy GBA v1.9.13 — standalone walkthrough-card ROM milestone

**Nature of this build:** isolated 9-card GBA screenshot viewer. **NOT** a
completed/playable Volvy's Adventure port, and not original stage-selection
gameplay.

## Real private-card ROM build

Generated and linked locally with the nine 256-color reference screens from
the user's 2h22 walkthrough, then verified:

- GBA image size: **262,144 bytes (256 KiB)**.
- ARM7TDMI/Thumb ELF code generation and link: **PASS**.
- ROM header: valid standard GBA logo, title "VOLVY CARDS ", game code "VLVC",
  fixed 0x96 byte and header checksum: **PASS**.
- Private source asset payloads: **9 of 9 independently decoded**, each
  containing 32,400 bytes for 240×135 8bpp Mode 4.
- Source-derived image palette: up to 256 BGR555 entries, with 12px top and
  13px bottom letterbox, passed by public synthetic host and codegen tests.
- SHA-256 of linked ROM:
  58e1047569798bd5b64647bc5dad5235a733d9a2a0746c968021f7b9e3fcd1d0
- The ROM is not committed to this public repo because it embeds
  source-derived private screenshots. The user can download the generated ROM
  and its complete build/archive directly from the conversation.

## Controls

- R or RIGHT = next screenshot.
- L or LEFT = previous screenshot.
- START = return to first/title screenshot.

## Reproducibility

The source and builder live in
experimental/handheld-card-viewer/ (requires a private v1.9.11 asset ZIP).
GitHub Actions assembles a ROM using **synthetic images only** to validate
cartridge linking without disclosing copyrighted game assets. The image pack
is checked for truncated or invalid BIOS-LZ77 content before linking.

## Unverified

- mGBA actual boot/rendering of this standalone viewer.
- Physical GBA boot/display.
- Complete private-asset-dependent native gameplay ROM.
- Source-game-authentic hub selection, level routing, scoring and audio.

**Overall porting estimate remains 44%.** It is not a computed score of
fidelity, or evidence that this viewer is a new playable level.
