#ifndef VOLVY_CORE_BONUS_RACE_H
#define VOLVY_CORE_BONUS_RACE_H

#include <stdint.h>
#include "core/level.h"
#include "core/player.h"
#include "core/bonus_bunnies.h"

typedef struct BonusRace {
    uint32_t elapsed_frames;
    uint8_t started;
    uint8_t finished;
    uint16_t bumper_hits;
    BonusBunny bunnies[BONUS_ACTIVE_BUNNY_COUNT];
} BonusRace;

void bonus_race_init(BonusRace *race);
int bonus_race_update(BonusRace *race, Level *level, Player *player,
                      int pre_bottom, int was_descending);

#endif
