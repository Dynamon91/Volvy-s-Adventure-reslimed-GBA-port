# GBA fidelity audit — v1.9.5

## Confirmed in the active runtime

| Area | What is present | What is still uncertain or missing |
| --- | --- | --- |
| World 1 | Levels 1–3, three 283x45 native 4bpp BG maps, converted sprites and source-derived placed objects | Many original interactions, enemy AI and room scripts |
| Shrewy | 32x18 native room, source sprite and boulder art, prototype boss controller | Precise HP, attack timings, damage rules and room sequence |
| Bonus Race | 664x140 native room and solo prototype using trace-derived spawn | Bunny opponent AI, exact gates, progression/reward rules |
| Player | Source-trace-based movement speeds, hand actions and native-source sprite sizes | Control mapping and transformation trigger (five-onion threshold is provisional) |
| UI/graphics | Source-derived font, HUD, native scenes, GBA OBJ palettes | Full original title/intro/pause pages; measured same-frame video comparison |
| Audio | Earlier inventory of original OGG theme names | Original music/sfx not converted and scheduled for GBA playback |

## Main gaps preventing a 1:1 port

- **Missing playable content:** Worlds 2, 3 and 4, their level routes, bosses,
  minigames and progression are not packaged as playable native GBA stages.
  The active renderer has only five native scene assets.
- **Original game scripts:** The packaged Windows `data.win` source inventory
  found names and objects, not decompiled GameMaker scripts; most runtime
  rules remain provisional. Its room list includes W1/W2/W3 references but
  no confirmed W4 room dataset.
- **Measured video correspondence:** The past walkthrough YouTube page was
  not available as a machine-readable video in this session. The earlier
  user mGBA gameplay video documented in `docs/mgba-video-review-v19.md`
  does not identify the tested ROM hash.
- **Device verification:** Host tests and ROM header/ELF checks do not
  guarantee runtime speed, sound, animation timing or an mGBA boot.

## Recommended accuracy milestones

1. Build per-level frame/timestamp references at matching in-game positions
   and compare player, backgrounds, HUD, and camera separately.
2. Reproduce collision, dash, jump, punch and ground-pound trajectories using
   the original CSV frame traces before tuning unknown controller rules.
3. Integrate missing rooms and enemy behavior from the supplied source
   material and update only when the new behavior is grounded by evidence.
4. Add original menu sequences, sound effects/music conversion and full
   stage/boss progression.
5. Verify every ROM in mGBA and ideally real hardware; record hashes,
   screenshots and controlled gameplay captures for regressions.

The filenames `test_world2.c` / `test_world3.c` in this project refer to
World 1 **Level 2** and World 1 **Level 3**, not playable Worlds 2/3.
The historical **44%** project estimate is not a measured fidelity score.
