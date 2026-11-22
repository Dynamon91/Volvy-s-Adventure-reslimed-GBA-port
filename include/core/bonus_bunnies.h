#ifndef VOLVY_CORE_BONUS_BUNNIES_H
#define VOLVY_CORE_BONUS_BUNNIES_H

#include <stdint.h>
#include "core/fixed.h"
#include "core/level.h"

/* The source room has two obj_bunny instances and nine obj_bunny_ground
 * instances. Only the two obj_bunny objects use this movement prototype.
 * No original bunny GML / velocity trace is available: this is NOT source AI.
 */
#define BONUS_ACTIVE_BUNNY_COUNT 2

typedef struct BonusBunny {
    fx32 x;                 /* source GameMaker sprite pivot, /2 for GBA */
    fx32 y;
    fx32 vx;
    fx32 vy;
    int16_t home_x;
    int16_t home_y;
    uint16_t source_instance_index;
    uint16_t frame_counter;
    int8_t facing;
    uint8_t grounded;
    uint8_t hop_count;
} BonusBunny;

void bonus_bunnies_init(BonusBunny bunnies[BONUS_ACTIVE_BUNNY_COUNT]);
void bonus_bunnies_update(BonusBunny bunnies[BONUS_ACTIVE_BUNNY_COUNT],
                          const Level *level);
const BonusBunny *bonus_bunny_for_instance(
    const BonusBunny bunnies[BONUS_ACTIVE_BUNNY_COUNT],
    unsigned int instance_index);

#endif
