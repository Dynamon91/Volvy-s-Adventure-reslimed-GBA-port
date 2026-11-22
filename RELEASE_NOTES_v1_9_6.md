# Volvy's Adventure: ReSlimed — GBA v1.9.6 (22 Nov 2026)

## Walkthrough-derived HUD timing correction

The user supplied a complete six-part gameplay recording. The segments were
joined and verified as a 2:22:14.076 H.264/AAC recording at 640×360/30 FPS.
The video shows a stopwatch in regular platform stages and boss encounters,
an onion counter, combo/placement indicators, distinctive stage-clear and
boss-clear screens, multiple selection screens, and content beyond the
currently reconstructed World 1.

The timestamped evidence and coverage gaps are documented in
`docs/walkthrough-reference-v1_0.md` and `docs/walkthrough-stage-index.csv`.

### Implemented

- Add `Level.elapsed_frames`, reset with each new level/arena.
- Increment the standard stage and Shrewy encounter timer at the 60 Hz game
  update cadence; the timer freezes while paused.
- Keep the Bonus Race's existing start-gate timer and display that timer in
  its own mode.
- Format the HUD timer as `M:SS:CC` (centiseconds derived from 60 Hz frames),
  expanding to `MM:SS:CC` after ten minutes.
- Replace the prototype `ONIONS 000`, `W1-Lx`, `SHREWY HP`, `HAND ATTACK` and
  `RACE` debug labels with the observed compact `X###` onion count and timer.
- Add standalone C regression tests for timer formatting, centisecond
  conversion, ten-minute expansion and the 99-minute cap.

### Verification

- Portable host C tests and source syntax: **PASS**.
- ARMv4T/Thumb code generation with test stubs: **PASS**, 43 translation units.
- Experimental Clang/LLD ROM structure: **PASS**, 44 C files compiled.
- ROM size: 2,097,152 bytes (2 MiB).
- ROM SHA-256: `ef8b7635c458156fa133d2e07a8f9f6c5dba79c2b484ed4cf5766383aa8e2ff5`.
- Actual mGBA boot, rendered HUD, devkitARM/libgba build and physical GBA:
  **NOT TESTED**.

### Remaining fidelity gaps exposed by the walkthrough

The recording shows scenes and encounters beyond the five source scenes in the
current prototype: city/rooftop, metro, sky/water, industrial and other courses;
Fishpig; Master Gator; and a later mechanical boss encounter whose title is
not legible. The current ROM does not implement those as source-faithful
content. The illustrated title/results screens, selection/hub flow, combo
scoring, ranking and full enemy/boss AI also remain incomplete.

This milestone corrects the HUD using direct video evidence. It does **not**
claim 1:1 fidelity or expand the reconstructed level set. Overall reconstruction
estimate remains 44%; this is a historical planning estimate, not a measured
fidelity score.
