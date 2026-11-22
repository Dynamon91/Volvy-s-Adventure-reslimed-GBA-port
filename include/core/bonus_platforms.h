#ifndef VOLVY_CORE_BONUS_PLATFORMS_H
#define VOLVY_CORE_BONUS_PLATFORMS_H

#include <stdint.h>
#include "core/fixed.h"
#include "core/player.h"

/* Exactly two obj_moving_platform source instances occur in rm_w1_b1_00.
 * Their original per-instance setup code is unavailable. The home points,
 * sprite origins and dimensions are source-derived; the motion is provisional.
 */
#define BONUS_MOVING_PLATFORM_COUNT 2

typedef struct BonusMovingPlatform {
    fx32 x;                     /* GBA world pivot, 20.12 fixed-point */
    fx32 y;
    fx32 home_x;
    int16_t width;             /* scaled source sprite footprint in GBA pixels */
    int16_t height;
    uint16_t source_instance_index;
    int8_t direction;
    int8_t reserved;
} BonusMovingPlatform;

void bonus_platforms_init(BonusMovingPlatform platforms[BONUS_MOVING_PLATFORM_COUNT]);
void bonus_platforms_update(BonusMovingPlatform platforms[BONUS_MOVING_PLATFORM_COUNT],
                            Player *player, int old_player_x, int old_player_bottom,
                            int was_descending);
const BonusMovingPlatform *bonus_platform_for_instance(
    const BonusMovingPlatform platforms[BONUS_MOVING_PLATFORM_COUNT],
    unsigned int source_instance_index);

#endif
