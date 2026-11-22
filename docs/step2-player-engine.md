# Step 2 — Player engine progress

## Implemented in this milestone

- 20.12 fixed-point player position and velocity.
- Horizontal acceleration, friction and speed cap.
- Gravity, terminal fall speed and jump impulse.
- Eight-frame jump buffer (source tune) and provisional six-frame coyote window.
- Horizontal and vertical tile collision with per-axis resolution.
- Big Hand horizontal dash, dash-up, hover, grind attach, ground-pound, punch, upward punch and ground dig states.
- Breakable tile handling for dash, ground-pound and Swolvy.
- Onion tile collection and provisional five-onion Swolvy threshold.
- Swolvy hitbox expansion with feet/center anchoring.
- Camera follow with facing look-ahead and world-bound clamping.
- Test room with platforms, rail, breakable blocks, onions, hazard and exit.
- Hardware sprite placeholders for normal and Swolvy forms.

## Provisional controls

| Input | Action |
|---|---|
| D-pad Left/Right | Move |
| A | Jump |
| B + Left/Right | Big Hand dash (about 20 frames in source traces) |
| B + Up in air | Dash-up |
| B + Up on ground | Upward punch |
| Hold B in air | Hover |
| B + Down in air | Ground-pound |
| B + Down on ground | Dig/smash |
| B on ground | Punch state |
| Hold R while touching rail | Grind |
| Start | Start / pause / resume / restart |

The initial run/jump/dash/hover/ground-pound values are now scaled from the supplied trace CSVs at a provisional 4:1 source-to-GBA ratio. Swolvy values, coyote frames, rail acceleration and exact button mapping still require confirmation.

## Temporary test-room note

The room geometry and procedural placeholder graphics exist only to exercise the engine. They are not reconstructed original levels or original Volvy art.

## Verification boundary

Host-side C syntax checks and deterministic unit tests can validate the game logic. A devkitARM/libgba cross-build, mGBA boot, and real-hardware run remain separate required checks.
