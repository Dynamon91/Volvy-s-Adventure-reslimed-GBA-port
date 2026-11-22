# Supplied source archive audit

Archive: `Volvy’s Adventure Reslimed.rar`

The archive is a packaged Windows/GameMaker build rather than an editable GameMaker source project.

## Confirmed archive contents

- `data.win`: 37,901,126 bytes.
- `Volvys_Adventure_6.exe`: 25,966,592 bytes.
- `localization.csv`: 70 rows, including English and Italian columns.
- `segs/`: 19 gameplay trace CSV files.
- `shadow_rm_w1_b1_00.json`, `shadow_rm_w2_b1_00.json`,
  `shadow_rm_w3_b1_00.json`: room segment metadata.
- `shadow_w1_b1_00.csv`: 6,000 recorded player-frame rows plus 18 event/tuning rows.
- OGG music tracks include `mu_title`, `mu_swolvy`, `mu_ow1` through
  `mu_ow4`, plus multiple stage and boss themes.

## Identified localization entries

Examples include:

- Matcha Meadows
- Fudge Forest
- Bundtcake Burrows
- Bubblegum Beach
- Peppermint Pier
- Sourworm Sewer
- Macaroni Metro
- Rigatoni Rooftops
- Ziti Zone

Boss-name entries include Shrewy, Master Gator, Fishpig, and Fork Parker.

## Gameplay trace schema

The 19 segment CSV files expose 15,304 player-frame records plus 1,761 event records. The separate `shadow_w1_b1_00.csv` contains 6,000 player-frame records and source tuning events. See `trace-analysis.md` for derived movement values.

The segment CSV files expose:

`t, x, y, hspd, vspd, state, inputs, onions, swole, underground, combo, time_ms, sprite, image, face`

Observed player states include normal movement, dash, grind, swim, dig,
ground-pound, punch, upward punch, bumped, and hurt.

These are useful as behavioral reference data. They are not a substitute for
the original collision map or sprite animation source.

## Resource-name scan

A GameMaker resource parse exposed 726 sprite resources and 18 background tilesets; a string scan separately exposed 441 distinct `spr_` resource identifiers and
1,941 distinct `gml_Script_` identifiers, 55 room identifiers and 227 object identifiers. This is a resource-name inventory only;
it does not mean the sprite pixels or GameMaker script source have been
extracted. The scan includes `spr_swolvy_transform`, `spr_swolvy_idle`,
`spr_swolvy_walk`, `spr_swolvy_jump`, `spr_swolvy_float`,
`spr_swolvy_grind`, `spr_swolvy_ground_pound`, and `spr_dig_swole`.

The room-name scan found `rm_w1_*`, `rm_w2_*`, and `rm_w3_*` room identifiers.
No `rm_w4_*` room identifier or corresponding World 4 trace was found in
the visible archive manifest. The existence of `mu_ow4.ogg` is confirmed,
but it does not establish the layout of World 4.

## Remaining extraction work

1. Decode sprite frames, tilesets, palettes, fonts, and room resources from
   `data.win`.
2. Identify exact frame dimensions, origins, animation speeds, and collision
   masks.
3. Reconstruct room geometry from the GameMaker room resources and compare it
   against the supplied traces.
4. Obtain World 4 room references or gameplay footage if it is part of the
   intended four-world release.
5. Convert OGG music and WAV effects into GBA-compatible formats. OGG cannot
   be played directly by the GBA hardware.

## Distribution note

Keep the original packaged game and extracted proprietary assets private
unless you have permission to redistribute them. The public project can
contain original port code, conversion tools, and asset manifests.


## Additional ROOM chunk extraction

The packaged `ROOM` chunk contains 53 room resources. A focused parse of `rm_w1_l1_00` decoded its two visible tile layers, six background/parallax layers, and 259 placed object instances. The extracted room includes 84 onions, 63 plants, 42 destructible blocks, 38 solid objects, 9 action blocks, 3 squirrels and 2 springboards.

The `obj_volvy` instance is stored at (64,384), while the room metadata height is 360. This discrepancy is preserved as source evidence; its runtime interpretation needs video or emulator confirmation.

## Dedicated Shrewy boss-room extraction (v0.6)

A focused parse of `rm_shrewy` found a 256×144 room with 24 placed instances, including `obj_boss_shrewy` at `(184,104)`, `obj_volvy` at `(64,160)`, three `obj_solid` instances, one `obj_owp`, four onions, one cage, two autoholes and four cone emitters. The source player pivot is below the room's nominal height; the runtime relocation remains unresolved.

The `SCPT` resource names include `gml_Room_rm_shrewy_Create`, ten room pre-create entries (`gml_RoomCC_rm_shrewy_*_PreCreate`), and multiple anonymous-function entries associated with `gml_Object_obj_boss_shrewy_Create_0` and `gml_Object_obj_boss_shrewy_boulder_Create_0`. The current extraction only enumerates these names; it does not decompile the GML bytecode.

A separate `rm_w1_b1_00` resource is 5312×1120 and contains 1024 placed instances. It is classified as a race-style bonus challenge from two race-start gates, a finish, podium, crowd and bunny placements plus one recorded `race` trace event. Its assets are converted; a basic solo timed-race runtime was added in v1.0, while original bunny AI, reward rules and room-entry progression remain unimplemented. The room-name inventory still does not establish any World 4 room resources; no `rm_w4_*` room was found in the supplied manifest.


## Shrewy script and trace audit (v0.7)

A new audit pass over the reopened original `data.win` found 5,510 `SCPT` resource-name entries and 65 names associated with `rm_shrewy`, `obj_boss_shrewy`, its boulder, cage, autoholes, cone emitters and adjustable autodiggers. The archive has no `CODE` chunk. The script names are useful identifiers, but they are not source code and this pass does not decompile the original GML.

The dedicated `rm_shrewy` room has 24 instances. Its prop table now preserves all 15 relevant placements: four onions, one cage, two autoholes, four adjustable autodigger markers and four cone emitters. The generator stores instance IDs, pivot coordinates, Q8.8 scales, rotation in tenths of a degree, and rotated sprite AABBs. The original collision grid remains AABB-derived and is not a pixel-mask reconstruction.

The selected CSV trace set contains 21,304 player-frame rows. The trace schema records player position, velocity, state, inputs, onion count, transformation flag, underground flag, combo, sprite and frame. It contains no Shrewy-named sprite/state rows and no boss HP/attack variables, so it cannot be used to infer boss attack timings or damage rules.


## SEQN sequence-resource extraction (v0.8)

The `SEQN` chunk has version 1 and seven sequence pointers. The names include `game_begin`, `seq_cage_drop`, `seq_game_keybinds_menu`, `seq_game_menu`, `seq_level_complete`, `seq_pause_first_page` and `seq_pause_level_select_page`. `seq_cage_drop` contains an `obj_cage` instance track and nested `origin`, `position`, `rotation` and `scale` parameter tracks. Its position Y curve is a five-point Bezier curve; the original curve is preserved in JSON and sampled into 60 fixed-point values for GBA playback.

This is sequence-resource data, not decompiled GML. The original object event that starts the sequence and the coordinate offset applied by the sequence instance remain unknown, so the curve is not automatically triggered in the boss fight.


## Bonus race runtime integration (v1.1)

The `rm_w1_b1_00` room now has a 332×70 AABB-derived collision grid. `obj_solid`, destructible blocks, spikes, rails, cloud platforms, and moving-platform initial footprints use the source object sprite dimensions/origins and instance transforms parsed from `data.win`. The grid is not a pixel collision mask. One-way cloud platforms and breakable one-way blocks use downward-only collision in the player controller; moving-platform paths remain unimplemented.

The `GAME_BONUS` prototype loads the room scene and collision map, renders/collects the 397 source onions, applies provisional bounce behavior to all seven source springboards and all 42 action blocks, renders the 6 squirrel placements, removes full source breakable-block footprints on destruction, starts a local frame timer when the player overlaps either race-start marker, and finishes on the horizontal finish-marker plane. The runtime spawn uses the first source trace frame (264.3,535.0 source pixels), not the raw `obj_volvy` placement at (158,675), which lies within a large solid AABB. The trace supports the finish X-plane because its `race` event occurs near the finish marker's X coordinate, but the original controller logic is unavailable. Bunny AI, official timing/reward rules, and the exact start/finish callsites remain unverified.


## Native-pixel actor and scene audit (v1.6)

- Re-extracted the default source sprite set with the added bonus-room enemy, hazard and prop sprites. The native sprite builder now emits 56 animation sets at 1:1 source scale, 501 OBJ metasprite parts and 71 opaque OBJ palette colors.
- `prepare_player_assets.py` sorts sprite names before conversion. Re-running the extractor and converter against the same `data.win` produced byte-identical `player_sprites.c`, `player_sprites.h` and manifest outputs.
- Expanded World 1 object tables to include `obj_hivefly`, `obj_hole`, `obj_plant`, `obj_game`/worm and `obj_level_exit_hole` for rendering. Updated source object counts: L1 214, L2 367, L3 502.
- Added bonus-room visible source sprites for hivefly, Crowly, Vinechomp, spikes, cannon, button, holes, rail slices and background-onion decoration. Their AI and damage behavior are not inferred from sprite presence.
- Native scene builder output is 4bpp with 15 scene palette banks and a reserved HUD bank. The five active scenes round-trip to their source composites with zero differing pixels after BGR555 quantization.
- The old 2:1/8bpp scene and actor assets remain under the experimental legacy paths for comparison; they are not the v1.6 runtime source.
