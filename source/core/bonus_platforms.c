#include "core/bonus_platforms.h"

#include "bonus_w1_room.h"

/* Tunable prototype only; there is no source trace for platform trajectories. */
#define PLATFORM_RANGE_PX 24
#define PLATFORM_VELOCITY (FX_ONE / 2)
#define SOURCE_SPRITE_W 48
#define SOURCE_SPRITE_H 8

static int overlaps_horizontally(int left_a, int width_a,
                                 int left_b, int width_b)
{
    return left_a < left_b + width_b && left_a + width_a > left_b;
}

const BonusMovingPlatform *bonus_platform_for_instance(
    const BonusMovingPlatform platforms[BONUS_MOVING_PLATFORM_COUNT],
    unsigned int source_instance_index)
{
    unsigned int i;
    for (i = 0; i < BONUS_MOVING_PLATFORM_COUNT; ++i) {
        if (platforms[i].source_instance_index == source_instance_index)
            return &platforms[i];
    }
    return 0;
}

void bonus_platforms_init(BonusMovingPlatform platforms[BONUS_MOVING_PLATFORM_COUNT])
{
    unsigned int i, next = 0;
    for (i = 0; i < BONUS_MOVING_PLATFORM_COUNT; ++i) {
        BonusMovingPlatform *platform = &platforms[i];
        platform->x = platform->y = platform->home_x = 0;
        platform->width = platform->height = 0;
        platform->source_instance_index = 0xFFFFu;
        platform->direction = i == 0 ? 1 : -1;
        platform->reserved = 0;
    }
    for (i = 0; i < BONUS_W1_ROOM_INSTANCE_COUNT; ++i) {
        const BonusW1RoomInstance *instance = &bonus_w1_room_instances[i];
        BonusMovingPlatform *platform;
        if (instance->kind != BONUS_W1_ROOM_OBJ_MOVING_PLATFORM) continue;
        if (next == BONUS_MOVING_PLATFORM_COUNT) break;
        platform = &platforms[next++];
        platform->source_instance_index = (uint16_t)i;
        platform->x = fx_from_int(instance->x / 2);
        platform->y = fx_from_int(instance->y / 2);
        platform->home_x = platform->x;
        /* AABB width is scaled (48 * instance.scale_x / 2) with round-up.
         * 1.333333x instance -> 32 GBA pixels; the second -> 24 pixels. */
        platform->width = (int16_t)((SOURCE_SPRITE_W *
             (unsigned int)instance->scale_x_q8 + 511u) / 512u);
        platform->height = (int16_t)((SOURCE_SPRITE_H *
             (unsigned int)instance->scale_y_q8 + 511u) / 512u);
    }
}

void bonus_platforms_update(BonusMovingPlatform platforms[BONUS_MOVING_PLATFORM_COUNT],
                            Player *player, int old_player_x, int old_player_bottom,
                            int was_descending)
{
    unsigned int i;
    int player_x = fx_to_int(player->x);
    int player_bottom = fx_to_int(player->y) + player->height;
    for (i = 0; i < BONUS_MOVING_PLATFORM_COUNT; ++i) {
        BonusMovingPlatform *platform = &platforms[i];
        fx32 old_x, displacement;
        int old_left, new_left, top;
        int standing_before, landed_now;
        if (platform->source_instance_index == 0xFFFFu) continue;
        old_x = platform->x;
        platform->x += platform->direction * PLATFORM_VELOCITY;
        if (platform->x >= platform->home_x + fx_from_int(PLATFORM_RANGE_PX)) {
            platform->x = platform->home_x + fx_from_int(PLATFORM_RANGE_PX);
            platform->direction = -1;
        } else if (platform->x <= platform->home_x - fx_from_int(PLATFORM_RANGE_PX)) {
            platform->x = platform->home_x - fx_from_int(PLATFORM_RANGE_PX);
            platform->direction = 1;
        }
        displacement = platform->x - old_x;
        old_left = fx_to_int(old_x) - platform->width / 2;
        new_left = fx_to_int(platform->x) - platform->width / 2;
        top = fx_to_int(platform->y) - platform->height;
        /* One-way behavior: nothing from below, no attachment while jumping.
         * The old position is used to carry a player already standing on the
         * deck before its half-pixel horizontal step. */
        if (!was_descending || player->vy < 0) continue;
        standing_before = old_player_bottom >= top - 1 &&
            old_player_bottom <= top + 1 &&
            overlaps_horizontally(old_player_x, player->width,
                                  old_left, platform->width);
        landed_now = !standing_before && old_player_bottom <= top &&
            player_bottom >= top &&
            overlaps_horizontally(player_x, player->width,
                                  new_left, platform->width);
        if (!standing_before && !landed_now) continue;
        if (standing_before) {
            player->x += displacement;
            if (player->x < 0) player->x = 0;
            player_x = fx_to_int(player->x);
        }
        player->y = fx_from_int(top - player->height);
        player->vy = 0;
        player->grounded = 1;
        player->coyote_frames = 0;
        /* Only one deck can own the player on each frame. */
        break;
    }
}
