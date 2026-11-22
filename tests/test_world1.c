#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "core/level.h"
#include "w1_l1_objects.h"

int main(void)
{
    Level level;
    int before = 0, after = 0, i, collected;
    level_init_world1(&level);
    assert(level.world1_active);
    /* First source obj_solid x=56,y=352, scale 7x4, sprite 16x16
       origin 8x8 => source rectangle [0,112] x [320,384]. */
    assert(level_get_tile_px(&level, 16, 162) == TILE_GROUND);
    assert(level_get_tile_px(&level, 16, 154) == TILE_EMPTY);
    assert(level_get_tile_px(&level, -1, 150) == TILE_GROUND);
    assert(level_get_tile_px(&level, 10, -1) == TILE_EMPTY);
    /* Source obj_destructible_block pivot (312,296), sprite origin (8,8)
       => GBA rectangle [152,160] x [144,152]. */
    assert(level_get_tile(&level, 19, 18) == TILE_BREAKABLE);
    assert(level_break_tile(&level, 19, 18));
    assert(level_get_tile(&level, 19, 18) == TILE_EMPTY);
    assert(!level_break_tile(&level, 19, 18));
    /* A source 8x8 object at x=236 spans tile columns 29 and 30.
       Breaking its outer tile clears the entire object but preserves the
       adjacent source block at x=228 which still occupies column 29. */
    assert(level_get_tile(&level, 30, 13) == TILE_BREAKABLE);
    assert(level_get_tile(&level, 29, 13) == TILE_BREAKABLE);
    assert(level_break_tile(&level, 30, 13));
    assert(level_get_tile(&level, 30, 13) == TILE_EMPTY);
    assert(level_get_tile(&level, 29, 13) == TILE_BREAKABLE);

    for (i = 0; i < W1_L1_OBJECT_COUNT; ++i)
        if (w1_l1_objects[i].kind == WORLD_ONION &&
            level_object_active(&level, i)) ++before;
    assert(before == 84);
    /* First source onion (992,328), source sprite 13x14 origin(8,8). */
    collected = level_collect_world_onions(&level, 492, 160, 8, 8);
    assert(collected >= 1);
    assert(level_collect_world_onions(&level, 492, 160, 8, 8) == 0);
    for (i = 0; i < W1_L1_OBJECT_COUNT; ++i)
        if (w1_l1_objects[i].kind == WORLD_ONION &&
            level_object_active(&level, i)) ++after;
    assert(after == 84 - collected);

    assert(!level_reached_world_exit(&level, 24, 150, 12, 10));
    assert(level_reached_world_exit(&level, 1076, 104, 8, 8));

    level_init_world1(&level);
    assert(level_get_tile(&level, 19, 18) == TILE_BREAKABLE);
    collected = level_collect_world_onions(&level, 492, 160, 8, 8);
    assert(collected >= 1);
    level_init_test_room(&level);
    assert(!level.world1_active);
    assert(!level_collect_world_onions(&level, 492, 160, 8, 8));
    puts("World 1 collision/object tests: PASS");
    return 0;
}
