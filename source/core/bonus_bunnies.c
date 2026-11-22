#include "core/bonus_bunnies.h"

#include "bonus_w1_room.h"

/* Prototype constants. They must NOT be represented as source-tuned AI. */
#define BUNNY_STEP        (FX_ONE * 3 / 4)
#define BUNNY_GRAVITY     (FX_ONE / 8)
#define BUNNY_JUMP        (-(FX_ONE * 3 / 2))
#define BUNNY_MAX_FALL    (FX_ONE * 3)
#define BUNNY_PATROL_PX   44
#define BUNNY_FOOT_OFFSET 4

static int is_support(uint8_t tile)
{
    return tile == TILE_GROUND || tile == TILE_BREAKABLE ||
           tile == TILE_SWOLVY_BLOCK || tile == TILE_ONEWAY ||
           tile == TILE_BREAKABLE_ONEWAY;
}

const BonusBunny *bonus_bunny_for_instance(
    const BonusBunny bunnies[BONUS_ACTIVE_BUNNY_COUNT],
    unsigned int instance_index)
{
    unsigned int i;
    for (i = 0; i < BONUS_ACTIVE_BUNNY_COUNT; ++i) {
        if (bunnies[i].source_instance_index == instance_index)
            return &bunnies[i];
    }
    return 0;
}

void bonus_bunnies_init(BonusBunny bunnies[BONUS_ACTIVE_BUNNY_COUNT])
{
    unsigned int i, next = 0;
    for (i = 0; i < BONUS_ACTIVE_BUNNY_COUNT; ++i) {
        BonusBunny *bunny = &bunnies[i];
        bunny->x = bunny->y = bunny->vx = bunny->vy = 0;
        bunny->home_x = bunny->home_y = -1;
        bunny->source_instance_index = 0xFFFFu;
        bunny->frame_counter = (uint16_t)(i * 19u);
        bunny->facing = i == 0 ? 1 : -1;
        bunny->grounded = 1;
        bunny->hop_count = 0;
    }
    for (i = 0; i < BONUS_W1_ROOM_INSTANCE_COUNT; ++i) {
        const BonusW1RoomInstance *instance = &bonus_w1_room_instances[i];
        BonusBunny *bunny;
        if (instance->kind != BONUS_W1_ROOM_OBJ_BUNNY) continue;
        if (next >= BONUS_ACTIVE_BUNNY_COUNT) break;
        bunny = &bunnies[next++];
        bunny->source_instance_index = (uint16_t)i;
        bunny->x = fx_from_int(instance->x / 2);
        bunny->y = fx_from_int(instance->y / 2);
        bunny->home_x = (int16_t)(instance->x / 2);
        bunny->home_y = (int16_t)(instance->y / 2);
    }
}

void bonus_bunnies_update(BonusBunny bunnies[BONUS_ACTIVE_BUNNY_COUNT],
                          const Level *level)
{
    unsigned int i;
    for (i = 0; i < BONUS_ACTIVE_BUNNY_COUNT; ++i) {
        BonusBunny *bunny = &bunnies[i];
        int x, y, step_dir, next_x, old_foot, new_foot, ground_y;
        if (bunny->source_instance_index == 0xFFFFu) continue;
        ++bunny->frame_counter;
        x = fx_to_int(bunny->x);
        y = fx_to_int(bunny->y);

        /* Turn before running outside the small source-anchored patrol range;
         * blocked terrain or a ledge reverses the direction as well. */
        step_dir = bunny->facing;
        next_x = x + step_dir;
        if (next_x < bunny->home_x - BUNNY_PATROL_PX ||
            next_x > bunny->home_x + BUNNY_PATROL_PX ||
            (bunny->grounded &&
             (!is_support(level_get_tile_px(level, next_x + step_dir * 6,
                                            y + BUNNY_FOOT_OFFSET + 4)) ||
              is_support(level_get_tile_px(level, next_x + step_dir * 9,
                                            y - 7))))) {
            bunny->facing = (int8_t)-bunny->facing;
            step_dir = bunny->facing;
        }
        bunny->vx = step_dir * BUNNY_STEP;
        bunny->x += bunny->vx;

        /* Hops are intentionally local and staggered, not an original race
         * path. The two obj_bunny instances are not race-timer opponents. */
        if (bunny->grounded && bunny->frame_counter % 80u == 0u) {
            bunny->vy = BUNNY_JUMP;
            bunny->grounded = 0;
            ++bunny->hop_count;
        }
        if (bunny->grounded) continue;

        old_foot = fx_to_int(bunny->y) + BUNNY_FOOT_OFFSET;
        bunny->vy += BUNNY_GRAVITY;
        if (bunny->vy > BUNNY_MAX_FALL) bunny->vy = BUNNY_MAX_FALL;
        bunny->y += bunny->vy;
        new_foot = fx_to_int(bunny->y) + BUNNY_FOOT_OFFSET;
        if (bunny->vy >= 0 && new_foot >= old_foot) {
            ground_y = ((old_foot + 7) / 8) * 8;
            while (ground_y <= new_foot) {
                if (is_support(level_get_tile_px(level, fx_to_int(bunny->x),
                                                  ground_y))) {
                    bunny->y = fx_from_int(ground_y - BUNNY_FOOT_OFFSET);
                    bunny->vy = 0;
                    bunny->grounded = 1;
                    break;
                }
                ground_y += 8;
            }
        }
        /* Limit falling to the source room's vertical extent if an original
         * landing surface is not found (e.g. after a destructible block). */
        if (fx_to_int(bunny->y) >= level_height_pixels(level) + 16) {
            bunny->x = fx_from_int(bunny->home_x);
            bunny->y = fx_from_int(bunny->home_y);
            bunny->vx = bunny->vy = 0;
            bunny->grounded = 1;
        }
    }
}
