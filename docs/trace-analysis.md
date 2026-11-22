# Source trace analysis — Volvy's Adventure: Reslimed

## Files actually read from the supplied RAR

- 19 files under `segs/`.
- `shadow_w1_b1_00.csv` (6,000 player-frame records plus 18 event records).
- `shadow_rm_w1_b1_00.json`.
- `shadow_rm_w2_b1_00.json`.
- `shadow_rm_w3_b1_00.json`.

The segment files contain 15,304 player-frame records and 1,761 event records in total. Event records use a separate `E,...` schema and must not be treated as player rows.

## Measured movement values

The trace rows show the following observed values in the original game's coordinate system:

| Mechanic / animation | Observed value |
|---|---:|
| Standard walk/run speed | commonly ±2.30 px/frame |
| Initial jump vertical speed | -3.45 px/frame in a representative jump |
| Jump gravity | +0.10 px/frame² in that same sequence |
| Float animation descent | commonly +0.35 px/frame |
| Horizontal dash | commonly ±4.00 px/frame |
| Dash-up | -4.00 px/frame |
| Ground-pound | commonly +6.00 px/frame |
| Dig movement | commonly 0 or ±2.00 px/frame |
| Punch state | typically about 10 frames |
| Dash state | commonly about 20 frames |

A representative jump sequence records `vspd=-3.45`, then -3.35, -3.25, -3.15, etc., consistent with +0.10 px/frame² gravity.

The `shadow_w1_b1_00.csv` event records also expose source tuning values:

- `jump_buffer = 8`
- `punch_buffer = 6`
- `gp_cancel = 2`
- `gp_cancel_jump = 4`
- `drill = 2`
- `spawn_copter = 1`

These values are source-derived; the meaning of `drill` and `spawn_copter` should be checked against the original video before finalizing their GBA button mapping.

## Confirmed initial GBA scale

The extracted GameMaker background resources identify 18 original tilesets. Their tile dimensions are consistently **16x16 source pixels**. The GBA renderer uses 8x8 character tiles, so the initial asset and physics conversion is **2:1**, not the earlier provisional 4:1 estimate.

The selected Volvy/Swolvy sprites also support this scale: normal idle is 24x19 source pixels (12x10 GBA pixels), while Swolvy idle is 44x26 (22x13 GBA pixels).

At 1/2 scale, initial reference values become:

| Mechanic | GBA prototype value |
|---|---:|
| Run speed | 1.15 px/frame |
| Initial jump speed | -1.725 px/frame |
| Gravity | +0.05 px/frame² |
| Float descent | 0.175 px/frame |
| Horizontal dash | 2.0 px/frame |
| Dash-up | -2.0 px/frame |
| Ground-pound | 3.0 px/frame |
| Dig movement | 1.0 px/frame |

The prototype now uses these as its initial calibration. Level geometry and camera framing still require reconstruction from the original rooms/traces.

## Source tuning and sprite extraction

The `data.win` resource table contains 726 sprite resources and 18 background tilesets. A private extraction pass successfully exported 29 player/Swolvy-related sprite animations and all 18 tilesets, with zero extraction errors.

The earlier 37-set player/world-actor package shared 29 opaque colors after 2:1 nearest-neighbor downscaling. The GBA asset generator preserves these colors in one 8bpp OBJ palette and streams each animation frame into OBJ VRAM.

## World/room references

The shadow room metadata gives recorded segment anchors:

- World 1 room `rm_w1_b1_00`: segment anchors at (256,496), (1136,432), (1664,496), and (2432,352).
- World 2 room `rm_w2_b1_00`: segment anchor at (464,640).
- World 3 room `rm_w3_b1_00`: multiple segments and object references, including a flip wall, race gates, rails and a chain-reaction block.

These are segment/recording anchors, not complete collision maps. They are useful for aligning trace sequences but cannot alone reconstruct room geometry.

## Important gaps

- The available traces contain no observed `swole=1` frame; the Swolvy threshold and altered physics are not calibrated from these recordings.
- The archive includes `mu_ow4.ogg`, but no World 4 room metadata or World 4 trace CSV was found.
- The user-linked YouTube playthrough is identified by title in the browser page, but its actual frames were not available to this session for direct measurement.
- The temporary test room is not an original level reconstruction.

## `rm_w1_b1_00` race-room trace extension (v0.9)

The supplied room data identifies `rm_w1_b1_00` as a race-style bonus challenge: it has two race-start objects, a race-finish object, a podium, crowd placements, bunny objects and four segment markers. Its four trace clips contain 2,983 player frames and 478 event rows. A `race` event is recorded in segment 4 near the finish object.

The four clips' `time_ms` domains overlap; segment 2 ends at 21,964 ms while segment 3 begins at 17,891 ms. Therefore no full race duration is calculated from these files. Segment 4 alone runs 33,682 ms from its first frame to the race event. The clips do not contain bunny opponent positions or AI state, and none of their player frames records Swolvy.
