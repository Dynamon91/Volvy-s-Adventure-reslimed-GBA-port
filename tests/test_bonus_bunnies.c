#include <assert.h>
#include <stdio.h>

#include "bonus_w1_room.h"
#include "core/bonus_bunnies.h"
#include "core/fixed.h"

int main(void)
{
    Level level;
    BonusBunny bunnies[BONUS_ACTIVE_BUNNY_COUNT];
    unsigned int i, found = 0;
    int moved = 0;
    int hopped = 0;
    level_init_bonus_race(&level);
    bonus_bunnies_init(bunnies);

    for (i = 0; i < BONUS_W1_ROOM_INSTANCE_COUNT; ++i) {
        const BonusW1RoomInstance *source = &bonus_w1_room_instances[i];
        const BonusBunny *bunny;
        if (source->kind != BONUS_W1_ROOM_OBJ_BUNNY) continue;
        bunny = bonus_bunny_for_instance(bunnies, i);
        assert(bunny);
        assert(bunny->source_instance_index == i);
        assert(fx_to_int(bunny->x) == source->x / 2);
        assert(fx_to_int(bunny->y) == source->y / 2);
        assert(bunny->home_x == source->x / 2);
        ++found;
    }
    assert(found == BONUS_ACTIVE_BUNNY_COUNT);

    /* The nine clustered source obj_bunny_ground placements are static
     * scenery; they must not be silently promoted into race opponents. */
    {
        int ground_count = 0;
        for (i = 0; i < BONUS_W1_ROOM_INSTANCE_COUNT; ++i) {
            if (bonus_w1_room_instances[i].kind == BONUS_W1_ROOM_OBJ_BUNNY_GROUND) {
                ++ground_count;
                assert(!bonus_bunny_for_instance(bunnies, i));
            }
        }
        assert(ground_count == 9);
    }

    for (i = 0; i < 300; ++i) {
        int b;
        bonus_bunnies_update(bunnies, &level);
        for (b = 0; b < BONUS_ACTIVE_BUNNY_COUNT; ++b) {
            const BonusBunny *bunny = &bunnies[b];
            int x = fx_to_int(bunny->x);
            int y = fx_to_int(bunny->y);
            assert(x >= bunny->home_x - 46);
            assert(x <= bunny->home_x + 46);
            assert(y >= 0 && y <= level_height_pixels(&level) + 16);
            assert(bunny->facing == 1 || bunny->facing == -1);
            if (x != bunny->home_x) moved = 1;
            if (bunny->hop_count) hopped = 1;
        }
    }
    assert(moved && hopped);
    bonus_bunnies_init(bunnies);
    for (i = 0; i < BONUS_ACTIVE_BUNNY_COUNT; ++i) {
        assert(fx_to_int(bunnies[i].x) == bunnies[i].home_x);
        assert(fx_to_int(bunnies[i].y) == bunnies[i].home_y);
        assert(bunnies[i].hop_count == 0);
    }
    puts("Bonus bunny source anchors, independent prototype movement, reset and ground-bunny separation: PASS");
    return 0;
}
