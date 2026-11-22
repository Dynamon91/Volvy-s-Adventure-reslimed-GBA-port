#ifndef VOLVY_PLATFORM_RENDER_H
#define VOLVY_PLATFORM_RENDER_H

#include <stdint.h>
#include "core/camera.h"
#include "core/level.h"
#include "core/player.h"
#include "core/boss_shrewy.h"
#include "core/bonus_race.h"
#include "core/game.h"

void render_init(Level *level);
void render_select_stage(uint8_t stage);
void render_select_boss(void);
void render_select_bonus(void);
void render_draw(const Level *level, const Player *player,
                 const ShrewyBoss *boss, const BonusRace *race, const Camera *camera,
                 GameMode mode, uint32_t anim_tick,
                 uint8_t title_selection);
void render_vblank_commit(Level *level, const Camera *camera);

#endif
