# Shrewy boss encounter — v0.6 prototype

## Source resources recovered

The supplied GameMaker archive contains a dedicated room `rm_shrewy`, the boss object `obj_boss_shrewy`, the projectile object `obj_boss_shrewy_boulder`, and original Shrewy sprite resources. A separate room `rm_w1_b1_00` is now classified as a race-style bonus challenge from its race gates, finish, podium, bunny/crowd placements and recorded race event. It is not part of the Shrewy arena and is not yet connected to the minigame runner.

The `rm_shrewy` room is 256×144 source pixels and contains 24 placed instances:

- 1 `obj_boss_shrewy` at source pivot `(184,104)`;
- 1 `obj_volvy` at source pivot `(64,160)`;
- 3 `obj_solid` instances;
- 1 `obj_owp` one-way platform;
- 4 onions;
- 1 cage;
- 2 autoholes;
- 4 cone emitters;
- 4 adjustable autodigger markers;
- 1 parallax control, 1 instance-killer and the room controller.

The player source pivot lies below the room's nominal height. The source room-create relocation logic has not been decoded. The prototype therefore uses a safe floor-aligned player top-left at `(50,93)`; its resulting sprite pivot is `(64,104)`. This is a provisional spawn, not a claim about the original intro.

Shrewy's idle sprite is 48×48 with origin `(25,39)`. Volvy's source idle sprite is 24×19 with origin `(14,11)`. The arena uses native-resolution sprite art and a native-resolution player physics profile.

## Converted graphics and collision

| Asset | Result |
|---|---:|
| Boss room scene | 256×144 pixels |
| Tile map | 32×18 8×8 tiles |
| Unique BG tiles | 417 |
| BG tile data | 26,688 bytes |
| BG opaque colors | 33 |
| Boss/actor OBJ palette | 81 opaque colors |
| Boss/room props exported | 11 |

The scene fits the 32 KiB available in BG character blocks 1+2. The v1.6 renderer uses the native 4bpp `shrewy_native.c/.h` scene and a 32×32 BG1 cache window; the 32×18 arena map occupies its upper-left region.

The boss room collision grid is 32×18. It is derived from three `obj_solid` sprite AABBs and the top surface of one `obj_owp` AABB. The one-way floor surface rounds to y=112. This is coarse AABB collision, not a decoded pixel-perfect mask or a verified reproduction of the original platform collision script.

Four onion instances are collected through the same player pickup path as the platform stages. Their removal flags are stored in the `Level` state and the renderer hides collected onion sprites.

## Native-resolution player

The platform stages use the 2:1 downscaled controller profile:

- normal hitbox 12×10, origin 7×6;
- Swolvy hitbox 22×13, origin 10×9.

The Shrewy room uses native-resolution source dimensions:

- normal hitbox 24×19, origin 14×11;
- Swolvy hitbox 44×26, origin 19×17.

The existing fixed-point physics constants are doubled in native-scale mode, preserving the same source-pixel motion units. Sprite-pivot alignment is preserved when the player changes form.

## Boss state machine

`source/core/boss_shrewy.c` implements these states:

1. `IDLE`
2. `WINDUP`
3. `JUMP`
4. `STOMP`
5. `DROPKICK`
6. `TORNADO`
7. `HURT`
8. `RECOVER`
9. `DEFEATED`

The prototype cycles through jump, dropkick and tornado attacks. The tornado state launches a rock projectile using the original `spr_shrewy_rock` art. The renderer selects original Shrewy animations for idle, wind-up, jump, stomp, dropkick, tornado and hurt states.

The boss can be damaged by Big Hand attacks during idle/recovery windows and by a descending stomp. The boss HUD displays a three-hit test health value. Player contact during the active stomp/dropkick/tornado states or rock projectile currently causes a provisional one-hit game-over.

The source `SCPT` resource includes `gml_Room_rm_shrewy_Create`, ten room pre-create entries, and multiple anonymous-function entries associated with the boss and boulder Create events. These names have been inventoried, but the bytecode is not decompiled.

**These mechanics are not source-verified.** The attack sequence, timers, HP, vulnerability windows, projectile velocity, contact damage and knockback are test parameters. The source `SCPT` resource includes Shrewy-related script/anonymous-function entries, but the original GML bytecode has not yet been decompiled or behavior-matched.

## Runtime integration

- W1 L3 stage clear → press Start → Shrewy arena.
- Start pauses/resumes the boss fight.
- Start after game-over restarts the boss arena.
- Defeating Shrewy → boss-clear screen; Start returns to the title prototype.
- `GAME_BOSS` is a distinct game mode; the boss room uses stage ID 4.

The v1.2 title-screen Select development shortcut cycles W1 L1/L2/L3 and Bonus Race. The Shrewy arena is reached through normal prototype progression after W1 L3.

## OBJ VRAM allocation

The boss encounter uses a separate 8bpp OBJ palette and mode-specific VRAM windows:

| Region | Use |
|---|---|
| 0–4 KiB | Native Volvy/Swolvy animation frame upload |
| 4–5 KiB | Four onion animation frames |
| 8–16 KiB | Shrewy animation frame upload (maximum tested frame 2,304 bytes) |
| 24–25 KiB | Shrewy rock frame |

The normal platform-stage actor cache begins at 2 KiB and overlaps the boss onion window. It is not used during the boss encounter and is rebuilt when switching back to a platform stage.

## Verification

`tests/test_boss_shrewy.c` checks:

- 256×144 room bounds and 32×18 collision map;
- one-way floor and native-scale player dimensions;
- boss source pivot and initial state;
- onion collection/removal;
- hand-attack and stomp damage windows;
- deterministic startup → wind-up → jump → stomp → recovery sequence;
- tornado rock spawning and movement;
- terminal boss defeat.

The complete host suite passes. Clang ARMv4T/Thumb object generation and relocatable linking pass for 26 translation units with test-only headers. Actual devkitARM/libgba linking, `.gba` generation, mGBA boot and real-hardware testing remain unverified.

## Next tasks

- Decode the room-create and boss object GML bytecode to determine authentic spawn relocation, boss health, attack order, hit windows and timing.
- Decode the `obj_boss_shrewy_boulder` behavior and collision/damage rules.
- Reconstruct cage/intro sequence, autoholes, cone emitters and room-specific effects.
- `rm_w1_b1_00` has been classified as a bonus/race room, but original boss-to-race progression still needs reconstruction.
- Validate native sprite pivots and collision masks in mGBA after a real devkitARM build.

## Static development previews

- `../previews/shrewy_boss_arena_v06.png` — 240×160 native-resolution composition from the original room scene and sprite assets; the Volvy spawn is provisional.
- `../previews/shrewy_boss_arena_v06_3x.png` — nearest-neighbor enlarged view of the same composition.
- `../previews/shrewy_animation_strip_v06.png` — source Shrewy animation frames.
- `../previews/shrewy_collision_v06.png` — AABB-derived one-way floor and boss/player bounds.

These are static asset composites, not mGBA or hardware screenshots.


## Source placement audit and transform completion (v0.7)

The room-prop exporter now preserves all 15 gameplay/editor-support placements rather than the earlier 11-item subset. The four `obj_adjustable_autodigger` instances are included. Every prop record stores the original instance ID, pivot, Q8.8 scale, rotation in tenths of a degree, and a transformed sprite AABB.

| Source object | Count | Source placement / transform |
|---|---:|---|
| `obj_onion` | 4 | x=64; y=96, 80, 64, 48 |
| `obj_cage` | 1 | pivot (128,-16), scale (1,1); 80×80 sprite AABB (88,-96) |
| `obj_autohole` | 2 | pivots (48,96), (176,96); scale (2,1); 32×16 AABBs |
| `obj_adjustable_autodigger` | 4 | pivots (56,112), (56,128), (184,112), (184,128) |
| `obj_cone_emitter` | 4 | pivots (40,120), (216,120), (4,40), (252,40); scale approximately (1/3,1.25), with source rotations retained |

The adjustable autodigger object uses `spr_ui_arrow_o`; this is retained as a source marker/transform, not drawn as a gameplay actor. The cone-emitter sprite is `spr_emitter_direction`; its rotation and scale are preserved as metadata. Neither the emitter hazard logic nor the autodigger scheduling is implemented yet. The cage AABB is fully above the room at its initial placement, so the exporter does not assume it is initially visible or static.

The generated `shrewy_room.h/.c` exposes `SHREWY_ROOM_PROP_COUNT=15`. Host tests assert the source instance IDs, transforms, object counts, autohole bounds and first emitter rotation.

## What the source archive can and cannot reveal

A fresh audit of the supplied `data.win` finds 5,510 `SCPT` resource-name entries and 65 names associated with Shrewy, its boulder, cage, autoholes, cone emitters and adjustable autodiggers. The `data.win` has no `CODE` chunk. The Windows executable contains GML event-name strings, but no source-level symbol/PDB map has been recovered. Therefore this extraction provides script/resource names, not original GML source or a bytecode decompilation. The boss AI timings, HP, vulnerability windows, projectile trajectory and hazard activation rules remain provisional.

The extracted CSV files contain 21,304 player-frame rows in the selected trace set. Their columns describe player state, input, position, sprite and animation; no row contains a Shrewy-named player state or sprite. This is not proof that a boss encounter never occurred—the trace format does not record boss variables. It does mean those files cannot calibrate Shrewy attack timing or health.


## Original cage-drop sequence extraction (v0.8)

The `SEQN` chunk is a separate resource type from GML script bytecode. Its pointer table contains seven sequence resources, including `seq_cage_drop`. The sequence parser exports the nested `obj_cage` instance track and its parameter tracks.

Source-derived metadata:

- Playback speed: 60 frames per second.
- Sequence length: 60 frames.
- `obj_cage` instance track: one keyframe spanning 60 frames.
- `position` parameter track: one keyframe with a 67-frame Bezier curve.
- X position curve: constant zero.
- Y position curve: five Bezier control points, starting at -88 source pixels and reaching approximately +40 pixels.
- `origin`: (0,0); `rotation`: 0; `scale`: (1,1).

The Y curve is sampled at frames 0–59 and baked into `shrewy_cage_sequence.c` as 20.12 fixed-point values. The helper `shrewy_cage_drop_offset(frame)` returns the raw sequence Y offset without floating-point math. Representative samples are frame 0 = -88.00, frame 20 = +30.16, frame 21 ≈ +40.00 and frame 59 ≈ +37.54 pixels.

The room contains `obj_cage` at pivot (128,-16), but the sequence instance's room-origin offset and the event that starts the sequence are not recovered. The contact-sheet preview adds the sequence offset to that room pivot as a clearly marked provisional interpretation. The curve is extracted and unit-tested, but is not yet triggered by the boss state machine.

GameMaker sequence resources are structured as tracks with keyframes and channels; the parser follows that resource model rather than treating the sequence as GML source. See the [GameMaker keyframe documentation](https://manual.gamemaker.io/lts/en/GameMaker_Language/GML_Reference/Asset_Management/Sequences/Sequence_Structs/The_Keyframe_Struct.htm) and [sequence editor documentation](https://manual.gamemaker.io/lts/en/The_Asset_Editors/Sequences.htm).


## Cage sequence playback helper (v0.9)

The extracted 60-sample `seq_cage_drop` curve now has a small fixed-point sequence player (`ShrewyCageSequencePlayer`). It advances one source sample per 60 Hz update, holds the last sample on completion, and can be restarted. The helper outputs only the raw source `position.y` offset; it does not add that value to the room cage pivot or start itself from a boss state. The sequence origin and trigger callsite are still unresolved, so no automatic boss-defeat trigger is claimed.
