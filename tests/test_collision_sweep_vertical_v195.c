/* Directly exercise the private collision sweep on synthetic high-velocity
 * impulses. The current player tuning normally caps velocities below 8 px,
 * but springs and boss scripts must not tunnel when calibrated later. */
#include <assert.h>
#include <stdio.h>
#include "../source/core/player.c"

int main(void)
{
    Level level;
    Player p;
    level_init_test_room(&level);
    level_set_tile(&level, 3, 9, TILE_GROUND);
    player_init(&p, 24, 50);
    p.vy = fx_from_int(20);
    move_vertical(&p, &level);
    assert(p.y == fx_from_int(62));
    assert(p.grounded && p.vy == 0);

    level_init_test_room(&level);
    level_set_tile(&level, 3, 9, TILE_ONEWAY);
    player_init(&p, 24, 50);
    p.vy = fx_from_int(20);
    move_vertical(&p, &level);
    assert(p.y == fx_from_int(62));
    assert(p.grounded);

    /* Passing a one-way platform from below must remain legal. */
    player_init(&p, 24, 88);
    p.vy = -fx_from_int(20);
    move_vertical(&p, &level);
    assert(p.y == fx_from_int(68));
    assert(!p.grounded);

    level_init_test_room(&level);
    level_set_tile(&level, 3, 10, TILE_GROUND);
    player_init(&p, 24, 96);
    p.vy = -fx_from_int(24);
    move_vertical(&p, &level);
    assert(p.y == fx_from_int(88));
    assert(p.vy == 0);
    puts("Vertical swept solid/one-way/underside collision: PASS");
    return 0;
}
