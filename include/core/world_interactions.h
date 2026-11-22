#ifndef VOLVY_CORE_WORLD_INTERACTIONS_H
#define VOLVY_CORE_WORLD_INTERACTIONS_H

#include "core/level.h"
#include "core/player.h"

typedef enum WorldInteraction {
    WORLD_INTERACT_NONE = 0,
    WORLD_INTERACT_SPRING = 1,
    WORLD_INTERACT_STOMP = 2,
    WORLD_INTERACT_DEFEAT = 4,
    WORLD_INTERACT_HURT = 8,
    WORLD_INTERACT_ACTION = 16
} WorldInteraction;

/* Call once after player_update. pre_bottom is the player's bottom before
 * that update; was_descending records pre-update vertical direction. */
unsigned int world_interact_player(Level *level, Player *player,
                                   int pre_bottom, int was_descending);
#endif
