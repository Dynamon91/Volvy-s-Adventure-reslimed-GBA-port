/* Regression: fast movement must not skip a one-tile wall, and GBA's
 * collision boundary must apply to negative fractional tile coordinates. */
#include <assert.h>
#include <stdio.h>
#include "core/level.h"
#include "core/player.h"

static InputState idle_input(void) {
    InputState input = {0,0,0};
    return input;
}

static void empty_room(Level *level) {
    level_init_test_room(level);
    /* Leave one source-like test ground row out of the collision window. */
}

int main(void)
{
    Level level;
    Player p;
    InputState input = idle_input();

    empty_room(&level);
    /* x=48..59, wall x=64..71: a 20 px dash passes the entire 8px tile
     * if the engine only inspects the final rightmost tile. */
    level_set_tile(&level, 8, 10, TILE_GROUND);
    player_init(&p, 48, 80);
    p.hand_state = HAND_DASH;
    p.hand_timer = 20;
    p.vx = fx_from_int(20);
    player_update(&p, &input, &level);
    assert(p.x == fx_from_int(52));
    assert(p.vx == 0);
    assert(p.hand_state == HAND_NONE);

    empty_room(&level);
    /* Both the breakable at x=56..63 and solid at x=64..71 need to be
     * processed in travel order: the dash destroys the first, hits second. */
    level_set_tile(&level, 7, 10, TILE_BREAKABLE);
    level_set_tile(&level, 8, 10, TILE_GROUND);
    player_init(&p, 40, 80);
    p.hand_state = HAND_DASH;
    p.hand_timer = 20;
    p.vx = fx_from_int(24);
    player_update(&p, &input, &level);
    assert(level_get_tile(&level, 7, 10) == TILE_EMPTY);
    assert(p.x == fx_from_int(52));

    empty_room(&level);
    /* Negative pixel -3 belongs to tile -1, not 0 (C / truncates).
     * Leaving the left side of the room must stop at x=0, not x=-8. */
    player_init(&p, 1, 80);
    p.hand_state = HAND_DASH;
    p.hand_timer = 20;
    p.facing = -1;
    p.vx = -fx_from_int(4);
    player_update(&p, &input, &level);
    assert(p.x == fx_from_int(0));

    empty_room(&level);
    /* Negative route is symmetric: floor a -20px dash at intermediate
     * tile x=24..31 even when the target tile x=16..23 is empty. */
    level_set_tile(&level, 3, 10, TILE_GROUND);
    player_init(&p, 41, 80);
    p.hand_state = HAND_DASH;
    p.hand_timer = 20;
    p.facing = -1;
    p.vx = -fx_from_int(20);
    player_update(&p, &input, &level);
    assert(p.x == fx_from_int(32));

    puts("Collision swept-tile wall, breakable and negative-world boundaries: PASS");
    return 0;
}
