# User mGBA video review — v1.8 follow-up

## Capture facts

- Duration: approximately 117.5 seconds.
- Video dimensions: 1920×1080.
- Emulator title bar: `mGBA - VOLVY RESLIM (60 fps)`.
- The recording shows a title screen, then W1-L1 gameplay.
- Visible mechanics/objects include onion counter updates, breakable blocks, pipes, normal Volvy, Swolvy, Big Hand movement, bunny/enemy encounters and vertical camera sections.

The recording does not show a ROM filename or checksum. It demonstrates a Volvy GBA build running in mGBA, but does not independently prove which project ZIP/ROM revision was loaded.

## Visual defect found

During vertical/tall sections, foreground gameplay sprites can overlap the top HUD. The v1.7 renderer assigned OBJ priority 0 to both player and actor sprites. BG0 also has priority 0. On GBA, OBJ wins a same-priority tie against a background layer, so a gameplay OBJ can cover HUD glyphs.

## v1.8 correction

- Gameplay player and boss frame parts use OBJ priority 1.
- Static world actors and bonus-room actors use OBJ priority 1.
- Boss onions use OBJ priority 1.
- BG0 HUD remains priority 0.
- BG1 scene remains priority 1. OBJ priority 1 wins ties against BG1, so actors remain visible over the world scene.
- Title-screen Volvy stays at OBJ priority 0 so the title composition is not changed.

## Validation status

Host tests, OAM-priority source regression, ARMv4T/Thumb code generation and experimental ROM structure checks pass. The v1.8 image still requires an mGBA retest to confirm the HUD now remains unobstructed in the same vertical sections.
