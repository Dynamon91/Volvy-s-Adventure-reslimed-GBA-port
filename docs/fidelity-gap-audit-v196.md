# Fidelity gap audit — v1.9.6 walkthrough evidence

## Current implemented coverage

The current experimental GBA source tree contains five native scenes:

- World 1 Level 1
- World 1 Level 2
- World 1 Level 3
- World 1 Bonus Race
- Shrewy arena

The project also has a player controller, native-resolution sprite sets, a 4bpp BG streaming renderer, collision/object interactions, a provisional Shrewy state machine and the v1.9.6 HUD timer correction.

## Newly confirmed from the supplied walkthrough

- Main menu: logo plus Start Game / Settings / Quit.
- Standard gameplay HUD: onion icon/count at upper-left and stopwatch at upper-right.
- Timer presentation: `M:SS:CC` (centiseconds inferred from the 60 Hz update cadence and observed digits).
- Combo counter and 1st/2nd placement indicators appear in relevant courses.
- Results presentation: purple illustrated stage-clear / boss-clear screens with Volvy thumbs-up artwork, green splat and score/ranking text.
- Selection/hub presentation: multiple handheld-style screens and an outdoor park/hub.
- Named selection/results include Fuzz Forest, Bonus Stage, Rigatoni Rooftops (verified at 57:00), Macaroni Metro (rechecked 81:30 crop, still not confirmed against original game text assets), Fishpig, Bundtcake Burrows and Master Gator.
- Boss results explicitly show Shrewy and Fishpig. A later mechanical boss encounter is visible but its name cannot be read confidently.
- The video spans many more environments than the current five scenes.

## Gaps that remain open

| Area | Current state | Required work |
|---|---|---|
| Worlds / levels | Only World 1 L1–L3 plus Bonus Race | Reconstruct missing stage layouts, object placements, collision and routing |
| Hub / selection | Title-menu scaffold exists; walkthrough card art has a private conversion pipeline, but no runtime card state | Rebuild handheld selection screens, entries, input flow, course routing and park/hub |
| Stage results | Generic text only | Recreate illustrated Stage Clear, five-thumb ranking and score metrics |
| Boss results | Generic `BOSS CLEAR` text | Boss-specific names, completion times, rankings and original transitions |
| Fishpig | Not implemented | Recover boss sprite/state/arena/attacks and collision |
| Master Gator | Selection card observed; encounter not verified from title alone | Identify exact encounter and reconstruct behavior |
| Late mechanical boss | Encounter visible; name unreadable | Identify from better source frames/source assets and reconstruct |
| HUD icons | Counter/timer text now present; original icons not yet integrated | Add source-accurate onion and stopwatch icon tiles |
| Combo / placement | Not implemented as a source-faithful system | Recover scoring rules, combo decay, rank thresholds and timing |
| Enemy AI | Many World 1 actors are provisional | Decode/reconstruct original AI, damage, hit reactions and timing |
| Camera/aspect | GBA viewport is 240×160; reference capture is 16:9 | Decide per-scene crop/letterbox policy without stretching source art |

## Work completed in v1.9.6

The HUD no longer displays prototype-only `ONIONS`, `W1-Lx`, `SHREWY HP`,
`HAND ATTACK` or `RACE` labels. It now displays `X###` and a stage timer.
Standard stages and Shrewy tick a 60 Hz timer while active; pausing freezes it.
Bonus Race retains its existing gate-triggered timer.

The new timer formatter has C unit coverage. Host tests, ARMv4T/Thumb stub
codegen and experimental Clang/LLD ROM structure pass. The ROM has not been
booted in mGBA or tested on hardware, so the rendered HUD is not yet verified.

## Reconstruction priority

1. Make the current HUD/result/menu layers match the reference before expanding
   the number of scenes.
2. Reconstruct one complete missing course from the walkthrough, including
   camera, tile layout, hazards, objects, pickups, exit and clear transition.
3. Add Fishpig as a separate boss mode only after its arena and attack phases
   are mapped from the recording.
4. Recover combo/placement/ranking rules from repeated runs and result screens.
5. Continue through remaining courses and boss encounters, keeping every
   source-derived fact separate from provisional assumptions.

**The walkthrough is valuable visual and timing evidence, but it cannot reveal
hidden scripts or exact collision masks on its own.** Source-faithful behavior
still needs either the original GameMaker data or enough controlled gameplay
frames to reconstruct it.
