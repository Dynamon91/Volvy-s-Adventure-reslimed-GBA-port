#ifndef VOLVY_CORE_PLAYER_H
#define VOLVY_CORE_PLAYER_H

#include <stdint.h>
#include "core/fixed.h"
#include "core/input.h"
#include "core/level.h"

#define VOLVY_ONIONS_TO_SWOLVY 5

typedef enum HandState {
    HAND_NONE = 0,
    HAND_HOVER,
    HAND_DASH,
    HAND_DASH_UP,
    HAND_GRIND,
    HAND_GROUND_POUND,
    HAND_PUNCH,
    HAND_PUNCH_UP,
    HAND_DIG
} HandState;

typedef struct Player {
    fx32 x;
    fx32 y;
    fx32 vx;
    fx32 vy;
    int16_t width;
    int16_t height;
    uint8_t origin_x;
    uint8_t origin_y;
    int8_t facing;
    uint8_t grounded;
    uint8_t coyote_frames;
    uint8_t jump_buffer_frames;
    uint16_t onions;
    uint8_t swole;
    uint8_t hover_energy;
    uint8_t hand_state;
    uint8_t hand_timer;
    uint8_t invulnerability;
    uint8_t dead;
    uint8_t native_scale;
    uint8_t enemies_defeated;
    uint8_t spring_activations;
} Player;

void player_init(Player *player, int x, int y);
void player_init_native(Player *player, int x, int y);
void player_update(Player *player, const InputState *input, Level *level);

#endif
