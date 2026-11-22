# Volvy's Adventure: ReSlimed — walkthrough reference audit v1.0

**Reference supplied by the user:** six sequential chunks named
`Volvys-Adventure-Reslimed-SnapYT.App.mp4.001` through `.006`.

## Media verification

- Segments joined in numeric order without gaps or duplicate container headers.
- Combined MP4 size: **601,191,189 bytes**.
- Duration: **2:22:14.076**.
- Video: H.264, **640×360**, **30 FPS**.
- Audio: AAC.
- The combined file was successfully parsed by FFprobe and sampled throughout.
- All timestamps below are offsets in this supplied recording, not in-game clocks.

## Walkthrough timeline

| Video time | Direct observation | Confidence / notes |
|---|---|---|
| 00:00 | Title screen with Volvy logo and `START GAME`, `SETTINGS`, `QUIT`. | High |
| 00:30–04:00 | Outdoor platforming: blue sky, clouds, orange/checkerboard terrain, green grass, collectibles and vertical traversal. `STAGE CLEAR!` at 04:00. | High |
| 04:30–10:00 | Further outdoor platforming with pipes, gates, moving/vertical routes and action effects. | High |
| 10:30 | Green handheld-style selection screen; title reads **Fuzz Forest**. Progress indicator `0/4`. | High |
| 11:00–15:30 | Dark forest/checkerboard platform section; onion pickups, breakable blocks and combo/action effects. | High |
| 16:30 | Green handheld-style selection screen reading **Bonus Stage**, with a `×20` reward indicator. | High |
| 17:00–24:30 | Brown/orange cave-like obstacle course with destructible blocks, combo scoring and placement/ranking presentation. | High |
| 25:30–29:00 | Industrial Shrewy arena with hazard signs and a central boss. `SHREWY DEFEATED!` at 29:30; final ranking shown as `3/5`. | High |
| 30:00–33:30 | Bright cyan sky/island course with a large tree and cloud platforms. `STAGE CLEAR!` at 33:30. | High |
| 34:00–39:30 | Purple night/water course with piers, lamps and vertical water/platform sections. `STAGE CLEAR!` at 39:30. | High |
| 40:00–44:30 | Dark green/grey industrial facility with pipes, platforms and fluid hazards. `STAGE CLEAR!` at 44:30. | High |
| 45:00–51:00 | Further industrial hazard encounter: green liquid, vertical jets and a large moving/spinning hazard or enemy. | Medium; exact encounter identity not established |
| 51:30–56:30 | Brown enclosed challenge course; a `2ND` placement indicator appears. | High |
| 57:00–65:30 | Pink handheld selection card reads **Rigatoni Rooftops**; orange city-rooftop and vertical-building course follows. `STAGE CLEAR!` at 65:30. | High for card title and course |
| 66:00–71:30 | Blue industrial/construction course with hanging chains, suspended platforms and swinging wrecking-ball hazards. | High |
| 72:00–79:30 | Dark industrial/city course with spotlights and vertical routes. `FISHPIG DEFEATED!` at 79:30. | High; boss name is **Fishpig**, not Ashpig |
| 80:00–88:30 | A handheld selection card appears at 81:30 and appears to read **Macho Metro**; the starry blue course follows, with multicolored platforms, `1ST`/`2ND` placement indicators and an `ON YOUR MARKS!` cue at 82:00. | Medium for title, high for course |
| 89:00–95:30 | Grey/white industrial course with conveyor-like structures, machinery and destructible objects. `STAGE CLEAR!`; `Highest combo: 73` at 95:30. | High |
| 96:00–105:30 | Purple night course with lamps, platforms, enemies and combo effects. | High |
| 106:00–110:30 | Bright sky/water course with floating platforms and large aquatic scenery. | High |
| 111:30 | Blue handheld selection screen reading `Bonus Stage`, with `1/1`. | High |
| 112:00–116:30 | Industrial/boss sequence; a selection card reads **Fishpig** at 114:00, followed by the encounter and `FISHPIG DEFEATED!` at 116:30. | High |
| 117:00–120:00 | Brown cave/checkerboard course with clouds and thorny hazards. `STAGE CLEAR!` at 120:00. | High |
| 120:30–122:00 | Shrewy industrial arena again. `SHREWY DEFEATED!` at 122:00. | High |
| 123:30–126:00 | Outdoor park/hub scene with an NPC, bench/awning and a tall mechanical structure. | High |
| 126:30 | Blue handheld selection screen reading **Master Gator**. | High |
| 127:00–133:30 | Blue industrial/sky construction course with suspended platforms, chains and enemies. `STAGE CLEAR!` at 133:30. | High |
| 134:00–136:30 | Starry blue/purple platform course with spikes and placement indicators. | High |
| 137:00–142:14 | Dark city/industrial boss encounter. A boss is enclosed in a blue energy field/mechanical vehicle; the preceding selection-card title at 138:30 is not reliably legible. | High for encounter, low for boss name |

## What the recording proves about the target game

1. **The target is substantially larger than the current GBA prototype.** The supplied recording contains many visual environments and encounters that are not represented by the five currently converted scenes.
2. **The HUD is not the old prototype HUD.** Gameplay frames consistently show a compact onion icon/count at top-left and a stopwatch/time display at top-right. The timer appears in `M:SS:CC` form and resets for separate stage/encounter runs. Combo and placement indicators appear in relevant sections.
3. **Results screens are distinctive.** Stage results use a purple illustrated screen, Volvy thumbs-up artwork, a green splat graphic and performance text (for example `Highest combo: 73`). Boss results name the boss and show completion time/ranking.
4. **The game has a selection/hub presentation.** The walkthrough shows multiple handheld-style selection screens, including `Fuzz Forest`, `Bonus Stage`, `Rigatoni Rooftops`, `Macho Metro` (partly legible), `Fishpig`, `Bundtcake Burrows` and `Master Gator`, plus an outdoor hub. The cards use distinct green, pink and teal handheld shells.
5. **Boss content extends beyond Shrewy.** Fishpig is explicitly defeated twice in the recording. A later dark-city mechanical boss encounter is also visible, but its name is not legible enough to assert.
6. **The video is a reference capture, not a GBA emulator test.** It must not be cited as proof that any experimental GBA ROM boots or matches the original.

## Comparison with GBA v1.9.5

The v1.9.5 source archive contains native scenes for World 1 Levels 1–3, the World 1 Bonus Race and Shrewy. It does **not** contain the later city, metro, aquatic, industrial and other courses visible in this recording. Fishpig, Master Gator and the late mechanical boss are not implemented as source-faithful encounters.

### Immediate correction made from this reference — v1.9.6

- Add a 60 Hz elapsed-stage timer for standard World 1 gameplay and Shrewy encounters.
- Display the timer in the observed `M:SS:CC` format.
- Replace the prototype `ONIONS 000` / `W1-Lx` / `SHREWY HP` / `HAND ATTACK` / `RACE` debug text with the compact `X###` onion count and stopwatch timer.
- Keep the Bonus Race's existing start-gate timer behavior.
- Add standalone C tests for timer formatting and boundary cases.

This is a HUD fidelity correction, **not** a claim that the missing worlds, result screens, selection menus, enemy AI or boss encounters have been reconstructed.

## Evidence files

- `walkthrough_keyframe_contact_sheet.jpg` — 24 timestamped frames selected from the supplied video.
- `keyframes/` — full 640×360 JPEG keyframes corresponding to the timecodes above.
- The contact sheets and keyframes are stored in the separate private Library archive `Volvy_Walkthrough_Reference_v1.0.zip`; the public GitHub repository keeps the textual index only.
- The original six uploaded chunks remain the source recording; the combined 601 MB MP4 is not duplicated in the reference pack.
