#include "core/bonus_race.h"

#include "bonus_w1_room.h"
#include "bonus_w1_bumper_bounds.h"
#include "core/fixed.h"

static int overlap(int ax, int ay, int aw, int ah,
                   int bx, int by, int bw, int bh)
{
    return ax < bx + bw && ax + aw > bx &&
           ay < by + bh && ay + ah > by;
}

static int floor_divide_two(int value)
{
    return value >= 0 ? value / 2 : -((1 - value) / 2);
}

static int hand_attacks(const Player *player)
{
    switch ((HandState)player->hand_state) {
    case HAND_DASH: case HAND_DASH_UP: case HAND_GROUND_POUND:
    case HAND_PUNCH: case HAND_PUNCH_UP: case HAND_DIG:
        return 1;
    default:
        return 0;
    }
}

static int hand_hits(const Player *player, int left, int top,
                     int width, int height)
{
    int px = fx_to_int(player->x);
    int py = fx_to_int(player->y);
    if (player->hand_state == HAND_PUNCH) {
        int hx = player->facing > 0 ? px + player->width : px - 9;
        return overlap(hx, py + 2, 9, player->height - 2,
                       left, top, width, height);
    }
    if (player->hand_state == HAND_PUNCH_UP) {
        return overlap(px + 2, py - 9, player->width - 4, 9,
                       left, top, width, height);
    }
    return overlap(px, py, player->width, player->height,
                   left, top, width, height);
}

void bonus_race_init(BonusRace *race)
{
    race->elapsed_frames = 0;
    race->started = 0;
    race->finished = 0;
    race->bumper_hits = 0;
    bonus_bunnies_init(race->bunnies);
    bonus_platforms_init(race->platforms);
}

int bonus_race_update(BonusRace *race, Level *level, Player *player,
                      int pre_x, int pre_bottom, int was_descending)
{
    int px, py, player_right, bottom;
    unsigned int i;

    if (race->finished) return 1;
    if (player->dead) return 0;
    px = fx_to_int(player->x);
    py = fx_to_int(player->y);
    player_right = px + player->width;
    bottom = py + player->height;

    /* Source springboard sprite is 16x16 with origin (8,8), i.e. 8x8
     * in this 2:1 room. Force/cooldown mirror the platform prototype. */
    for (i = 0; i < BONUS_W1_ROOM_INSTANCE_COUNT; ++i) {
        const BonusW1RoomInstance *instance = &bonus_w1_room_instances[i];
        int left, top;
        if (level->actor_cooldown[i]) --level->actor_cooldown[i];
        if (instance->kind != BONUS_W1_ROOM_OBJ_SPRINGBOARD ||
            level->actor_cooldown[i] != 0 || !was_descending) continue;
        left = floor_divide_two((int)instance->x - 8);
        top = floor_divide_two((int)instance->y - 8);
        if (pre_bottom <= top + 2 && bottom >= top &&
            px < left + 8 && player_right > left &&
            bottom <= top + 12) {
            player->y = fx_from_int(top - player->height);
            player->vy = -FX_ONE * 9 / 2;
            player->grounded = 0;
            player->coyote_frames = 0;
            player->hand_state = HAND_NONE;
            level->actor_cooldown[i] = 16;
            ++player->spring_activations;
            py = fx_to_int(player->y);
            bottom = py + player->height;
            break;
        }
    }

    /* Action blocks use the platform prototype's hand-contact rebound and
     * cooldown. The source sprite AABB converts to 8x8 GBA pixels. */
    for (i = 0; i < BONUS_W1_ROOM_INSTANCE_COUNT; ++i) {
        const BonusW1RoomInstance *instance = &bonus_w1_room_instances[i];
        int left, top;
        if (instance->kind != BONUS_W1_ROOM_OBJ_ACTION_BLOCK ||
            level->actor_cooldown[i] != 0 || !hand_attacks(player)) continue;
        left = floor_divide_two((int)instance->x - 8);
        top = floor_divide_two((int)instance->y - 8);
        if (hand_hits(player, left, top, 8, 8)) {
            player->vy = -FX_ONE * 5 / 2;
            player->grounded = 0;
            level->actor_cooldown[i] = 24;
            break;
        }
    }

    /* Conservatively rasterized bounds use source sprite dimensions,
     * per-instance scale and rotation. Impulse and cooldown remain guesses. */
    {
        unsigned int b;
        for (b = 0; b < BONUS_W1_BUMPER_COUNT; ++b) {
            const BonusBumperHitbox *hit = &bonus_w1_bumpers[b];
            int dx, dy, sx, sy;
            i = hit->instance_index;
            if (level->actor_cooldown[i] != 0) continue;
            if (!overlap(px, py, player->width, player->height,
                         hit->x, hit->y, hit->width, hit->height))
                continue;
            dx = px + player->width / 2 - (hit->x + hit->width / 2);
            dy = py + player->height / 2 - (hit->y + hit->height / 2);
            sx = dx < 0 ? -1 : 1;
            sy = dy < 0 ? -1 : 1;
            if (hit->corner) {
                player->vx = sx * FX_ONE * 2;
                player->vy = sy * FX_ONE * 2;
            } else if (dx * dx >= dy * dy) {
                player->vx = sx * FX_ONE * 3;
                player->vy = 0;
            } else {
                player->vx = 0;
                player->vy = sy * FX_ONE * 3;
            }
            player->grounded = 0;
            player->coyote_frames = 0;
            level->actor_cooldown[i] = 12;
            ++race->bumper_hits;
            break;
        }
    }

    if (!race->started) {
        for (i = 0; i < BONUS_W1_ROOM_INSTANCE_COUNT; ++i) {
            const BonusW1RoomInstance *instance = &bonus_w1_room_instances[i];
            int left, top;
            if (instance->kind != BONUS_W1_ROOM_OBJ_RACE_START) continue;
            /* obj_race_start references spr_solid: 16x16, origin (8,8).
             * GBA scene scale is 2:1, yielding an 8x8 gate AABB. */
            left = floor_divide_two((int)instance->x - 8);
            top = floor_divide_two((int)instance->y - 8);
            if (overlap(px, py, player->width, player->height,
                        left, top, 8, 8)) {
                race->started = 1;
                break;
            }
        }
    }

    if (race->started) {
        bonus_bunnies_update(race->bunnies, level);
        bonus_platforms_update(race->platforms, player,
                               pre_x, pre_bottom, was_descending);
        ++race->elapsed_frames;
        level->race_elapsed_frames = race->elapsed_frames;

        for (i = 0; i < BONUS_W1_ROOM_INSTANCE_COUNT; ++i) {
            const BonusW1RoomInstance *instance = &bonus_w1_room_instances[i];
            int finish_left;
            if (instance->kind != BONUS_W1_ROOM_OBJ_RACE_FINISH) continue;
            finish_left = floor_divide_two((int)instance->x - 8);
            /* The supplied source trace fires its race event at x=4972.59,
             * when the player's right edge reaches the finish marker's
             * horizontal AABB. Its Y is not vertically overlapping the marker,
             * so the original trigger is represented as an X-plane here. */
            if (player_right >= finish_left) {
                race->finished = 1;
            }
            break;
        }
    }

    return race->finished != 0;
}
