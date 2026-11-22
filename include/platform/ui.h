#ifndef VOLVY_PLATFORM_UI_H
#define VOLVY_PLATFORM_UI_H
#include <stdint.h>
#include "core/game.h"
void gba_ui_init(void);
void gba_ui_update(GameMode mode, uint16_t onions, uint8_t stage,
                   uint8_t boss_health, uint32_t timer_frames);
void gba_ui_vblank_commit(void);
#endif
