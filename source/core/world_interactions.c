#include "core/world_interactions.h"


/* Provisional physics. Source object placements are extracted accurately;
 * squirrel reactions and spring force await script/video calibration. */
#define SPRING_LAUNCH (-FX_ONE * 9 / 2)
#define STOMP_REBOUND (-FX_ONE * 2)
#define ACTION_REBOUND (-FX_ONE * 5 / 2)

static int overlap(int ax, int ay, int aw, int ah,
                   int bx, int by, int bw, int bh)
{
    return ax < bx + bw && ax + aw > bx &&
           ay < by + bh && ay + ah > by;
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

static int hand_hits(const Player *player,
                     int left, int top, int width, int height)
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

unsigned int world_interact_player(Level *level, Player *player,
                                   int pre_bottom, int was_descending)
{
    unsigned int events = WORLD_INTERACT_NONE;
    int i;
    int px = fx_to_int(player->x);
    int py = fx_to_int(player->y);
    int bottom = py + player->height;
    if (!level->world1_active || player->dead) return events;

    /* A fatal contact ends this frame's interaction pass. Do not allow a later
     * overlapping object to apply a bounce/defeat after the player is dead. */
    for (i = 0; i < level_world_object_count(level) && !player->dead; ++i) {
        const WorldObject *obj = level_world_object(level, i);
        int ox, oy, ow, oh;
        if (!level_world_actor_rect(level, i, &ox, &oy, &ow, &oh)) continue;
        if (obj->kind == WORLD_SPRINGBOARD) {
            /* Edge-triggered on the descending crossing of the top surface. */
            if (was_descending && level->actor_cooldown[i] == 0 &&
                pre_bottom <= oy + 2 && bottom >= oy &&
                px < ox + ow && px + player->width > ox &&
                bottom <= oy + oh + 4) {
                player->y = fx_from_int(oy - player->height);
                player->vy = SPRING_LAUNCH;
                player->grounded = 0;
                player->coyote_frames = 0;
                player->hand_state = HAND_NONE;
                level->actor_cooldown[i] = 16;
                ++player->spring_activations;
                events |= WORLD_INTERACT_SPRING;
                bottom = oy;
                py = fx_to_int(player->y);
            }
            continue;
        }

        if (obj->kind == WORLD_SQUIRREL) {
            int body_hit = overlap(px, py, player->width, player->height,
                                   ox, oy, ow, oh);
            if (!body_hit && !(hand_attacks(player) &&
                               hand_hits(player, ox, oy, ow, oh))) continue;
            if (hand_attacks(player) && hand_hits(player, ox, oy, ow, oh)) {
                level->removed[i] = 1;
                ++player->enemies_defeated;
                events |= WORLD_INTERACT_DEFEAT;
            } else if (body_hit && was_descending &&
                       pre_bottom <= oy + 3 && bottom >= oy) {
                level->removed[i] = 1;
                ++player->enemies_defeated;
                player->y = fx_from_int(oy - player->height);
                player->vy = STOMP_REBOUND;
                player->grounded = 0;
                player->coyote_frames = 0;
                events |= WORLD_INTERACT_STOMP | WORLD_INTERACT_DEFEAT;
                bottom = oy;
                py = fx_to_int(player->y);
            } else if (body_hit && player->invulnerability == 0) {
                player->dead = 1;
                events |= WORLD_INTERACT_HURT;
            }
            continue;
        }

        if (obj->kind == WORLD_ACTION_BLOCK &&
            level->actor_cooldown[i] == 0 &&
            hand_attacks(player) && hand_hits(player, ox, oy, ow, oh)) {
            /* Temporary action-block interaction: upward bounce. */
            player->vy = ACTION_REBOUND;
            player->grounded = 0;
            level->actor_cooldown[i] = 24;
            events |= WORLD_INTERACT_ACTION;
        }
    }
    return events;
}
