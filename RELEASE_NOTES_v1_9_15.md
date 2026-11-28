# Volvy GBA v1.9.15 — broad content infrastructure and full-world reference pass

**Date:** 28 November 2026  
**Scope:** existing source reconstruction, not a rebuild from zero  
**Original-game-wide completion estimate:** 44% (unchanged, provisional)

## Working game-code changes

The five existing prototype scenes (World 1 Levels 1–3, Bonus Race, and
Shrewy) now have RAM-resident per-scene completion records. On an actual
transition into GAME_VICTORY, the game records:

- clear count per prototype scene, with saturation rather than wraparound;
- best clear time in 60 Hz frames;
- accumulated collected onions and enemies defeated from the previous
  source-derived per-run counters.

Entering a level or retrying still resets only the **current run**. The
campaign records persist in RAM across transitions back to the title.

The player now maintains independent 32-bit **monotonic event counters**
for onion pickups and defeated enemies. Results no longer rely on the
0–999 inventory display: consuming onions or filling that display
cannot erase previously collected items. World 1 stomp and hand-hit
defeats update the same event totals.
They are reset on program initialization. No original-game SRAM format,
unlock route, achievement, combo formula or leaderboard has been invented.

For game modes whose existing victory UI uses the generic Mode 0 HUD, it
now prints actual time/collected/defeated totals. Shrewy's confirmed boss
heading is "SHREWY DEFEATED". The first World 1 stage's previously added
static source-image Stage Clear remains unchanged rather than overwritten.
The result-page layout is provisional, not yet a faithful animated page.

## Reference coverage across the original game

A source-based 12-checkpoint index captures diverse environments from the
user's complete 2:22 video: Fudge Forest, sky/islands, night piers,
industrial facility, Rigatoni Rooftops, construction, Fishpig's route,
Macaroni Metro, factory, dusk towers, aquatic and a late mechanical boss.
Many titles are **descriptive** because the official source names are
not yet confirmed; the original GameMaker room mapping is likewise open.

The reproducible tool
\`tools/build_world_reference_previews.py\` reads a private user-supplied
video and \`progress/reference_scene_checkpoints.json\` to create
240x160, 8bpp, hardware BGR555-palette previews at each checkpoint, plus
a GIF, contact sheet and source SHA256 manifest.

A separate private conversation package contains 12 verified visual
previews and comparisons. **They are converted video frames, not
emulator captures or proof of completed native GBA levels.**

## Soundtrack preparation

The new \`tools/prepare_gba_audio.py\` converts privately available source
OGG/WAV audio through ffmpeg to 16,384 Hz signed 8-bit mono raw PCM by
default. It prepares aligned stream chunks and a SHA-256 manifest.
Unit tests check the exact last-sample/chunk padding and metadata.

No original soundtrack sources are redistributed in the repository.
This step does **not** implement or verify the GBA Direct Sound playback
engine, DMA ring buffer, or music/SFX mixing. The source sound tracks
must be provided privately to complete integration.

## Verification and deferred checks

- Host unit tests: run metrics, campaign progress, result formatting,
  world-reference metadata/preview geometry and audio chunk math.
- GitHub ARM7TDMI checks: core RAM record and result modules compile
  for ARMv4T/Thumb, alongside the existing bitmap modules.
- Main ROM boot and visual tests in mGBA: **deliberately deferred** by
  user request until more of the whole game is implemented.
- No playable Fudge Forest, Fishpig or late-game native levels have
  been added in this milestone. No complete new .gba cartridge is claimed.

## Next content-critical implementation gates

1. Recover complete original room data / background tilemap layers for
   Fudge Forest and subsequently the other levels, from the private
   source GameMaker data when available.
2. Integrate matching world objects, platform collision masks, enemy AI,
   exit/entrance scripts, parallax and source-camera behavior.
3. Recover all audio sources, implement GBA mixer/streaming and connect
   stage/boss tracks in the actual game.
4. Recreate original game hub/selection routing, full boss fights and
   save format without relying on unverified assumptions.
5. Perform end-to-end mGBA and real hardware checks at the later gate.

**Whole-game completion estimate remains 44%.** Tooling and source
reference previews must not be counted as fully playable native content.
