#ifndef VOLVY_CORE_BOSS_SHREWY_H
#define VOLVY_CORE_BOSS_SHREWY_H

#include <stdint.h>
#include "core/player.h"
#include "core/level.h"

typedef enum ShrewyState {
    SHREWY_IDLE = 0,
    SHREWY_WINDUP,
    SHREWY_JUMP,
    SHREWY_STOMP,
    SHREWY_DROPKICK,
    SHREWY_TORNADO,
    SHREWY_HURT,
    SHREWY_RECOVER,
    SHREWY_DEFEATED
} ShrewyState;

typedef struct ShrewyBoss {
    fx32 x; /* source sprite pivot in native-resolution arena pixels */
    fx32 y;
    fx32 vx;
    fx32 vy;
    fx32 rock_x;
    fx32 rock_y;
    fx32 rock_vx;
    fx32 rock_vy;
    uint8_t health;
    uint8_t state;
    uint8_t timer;
    uint8_t invulnerability;
    uint8_t facing;
    uint8_t next_attack;
    uint8_t rock_active;
    uint8_t rock_timer;
} ShrewyBoss;

void shrewy_init(ShrewyBoss *boss);
void shrewy_update(ShrewyBoss *boss, Player *player,
                   int pre_bottom, int was_descending);
int shrewy_is_defeated(const ShrewyBoss *boss);

#endif
