# Fudge Forest — original-source evidence and GBA reconstruction checkpoint

**Audit date:** 28 November 2026

The six user-provided walkthrough fragments were reassembled into the
**2:22:14.076**, 640×360, 30 fps recording. The timestamps below are
recording offsets, **not** in-game times.

## Direct source observations

| Source offset | Visible evidence | Confidence |
|---|---|---|
| 10:29 | Green handheld card reads **Matcha Meadows**; visible 3/4 progress | High |
| 10:30–10:33 | Next green handheld card reads **Fudge Forest**, with 0/4 | High |
| 10:36 | Forest gameplay starts: dark blue/green layered foliage, brown checkerboard ground, grass ledges, blue mechanical pipe and orange onion pickup icons | High |
| 10:37–10:39 | Vertical jumps alongside tall orange pillars, destructible/action blocks, hanging pickups and platform gaps | High |
| 10:41–10:44 | Source HUD displays changing onion count and a green/pink combo indicator during combat/pickups | High for visuals; unverified score rules |
| 11:25–11:50 | Additional leaf canopy, platform hazards and enemies; spike strips and changing camera view | High |
| 12:15–13:55 | Moving/jumping between checkerboard columns and grass-topped ledges; more hazards, breakable blocks and enemy encounters | High |
| 14:20–15:33 | Cloud/platform traversal and late forest route, followed by the end of the observed section | Medium for exact level boundaries |

**Correction:** Older notes incorrectly transcribed “Fuzz Forest.”
The game title is **Fudge Forest**, confirmed both by direct video
frames and the independent English localization-name inventory in
\`docs/source-audit.md\`, which already listed **Matcha Meadows** and
**Fudge Forest**. The existing private asset ID \`fuzz_forest\` is
preserved for binary/source compatibility; this is not the in-game name.

## New source-derived screenshot/GIF preview

To compare against GBA graphics limitations *before* a complete
scene conversion, a 4-second clip starting at 10:36 was sampled
at 8 frames per second (32 frames). The 640×360 source was
downsampled into 240×135 content, placed in 240×160 Mode 4 framing
with 12px top / 13px bottom black letterbox.

The 32-image sequence uses **one shared palette** of 256 BGR555 colors
(index 0 = black) to avoid per-frame palette changes/flicker.
A BOX resampling candidate was used here to protect thin edges;
this remains an A/B preview choice, not a gameplay-engine filter.

The generated GIF and four visual comparisons are delivered as
**private conversation artifacts**, not committed to this repository,
because they contain transformed source-game artwork. The previews
are **not screenshots from mGBA**, not a running gameplay ROM, and
do not demonstrate camera/collision fidelity.

## Gameplay integration in the same milestone

New \`include/core/run_stats.h\` and \`source/core/run_stats.c\`
record per-run totals from the already implemented player counters:
positive onion pickup changes and enemy defeats. The game resets
these statistics when entering/retrying World 1, Bonus Race or Shrewy,
and does not count title, paused, or card-preview frames.

This separates measured gameplay facts from **unverified combo
multipliers, decay, rankings and result screens**. The original
result panels visibly contain a per-run onion total; exact combo
and rank rules are not inferred from the footage.

## Remaining work for a real Fudge Forest course

1. Identify its specific GameMaker room(s) and full map layers from
   the original asset archive; avoid fabricating unseen geometry.
2. Extract background/tilemap and exact spawn/exit placements, enemies,
   destructible blocks, hazards, pickups and scripted transitions.
3. Implement original collision and AI from source/traces where
   available; preserve uncertainty when not.
4. Integrate the complete native 4bpp streamed BG scene, OBJ animations,
   audio and gameplay transitions.
5. Test the finished campaign together later, per the user's request
   to defer emulator verification.

**Completion:** this is a source-accuracy correction, gameplay metric
foundation and visual checkpoint. **Fudge Forest is not yet a new
playable GBA level**. The historical full-game 44% estimate is unchanged.
