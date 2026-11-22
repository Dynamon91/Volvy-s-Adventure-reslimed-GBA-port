/* Bundtcake Burrows source-derived world state tests. No emulator required. */
#include <assert.h>
#include <stdio.h>
#include "core/level.h"
#include "core/player.h"
#include "core/world_interactions.h"
#include "w1_l1_objects.h"
#include "w1_l2_objects.h"
#include "w1_l3_objects.h"
#include "rm_w1_l3_scene.h"

static int first_kind(const Level *level, int kind)
{
    int i;
    for (i = 0; i < level_world_object_count(level); ++i)
        if (level_world_object(level, i)->kind == kind) return i;
    return -1;
}

int main(void)
{
    Level level;
    Player player;
    int i, x, y, w, h, onions = 0, broken = 0, springs = 0, action = 0;
    int first_onion, first_break, first_spring, first_exit;
    int hit = 0;

    level_init_world1_stage(&level, 3);
    assert(level.world1_active && level.active_stage == 3);
    assert(W1_L3_OBJECT_COUNT == 495);
    assert(W1_L3_OBJECT_COUNT < LEVEL_MAX_OBJECTS);
    assert(level_world_object_count(&level) == W1_L3_OBJECT_COUNT);
    assert(W1_L3_ONION_COUNT == 318);
    assert(level_world_object(&level, -1) == NULL);
    assert(level_world_object(&level, W1_L3_OBJECT_COUNT) == NULL);

    for (i = 0; i < level_world_object_count(&level); ++i) {
        const WorldObject *obj = level_world_object(&level, i);
        if (obj->kind == WORLD_ONION) ++onions;
        if (obj->kind == WORLD_BREAKABLE) ++broken;
        if (obj->kind == WORLD_SPRINGBOARD) ++springs;
        if (obj->kind == WORLD_ACTION_BLOCK) ++action;
    }
    assert(onions == 318 && broken == 157 && springs == 3 && action == 15);

    /* The player starts on a provisional safe platform, not the original
       (49,382) pivot, whose runtime relocation is still unknown. */
    assert(level_get_tile_px(&level, 25, 151) == TILE_EMPTY);
    assert(level_is_solid(level_get_tile_px(&level, 25, 152)));

    first_onion = first_kind(&level, WORLD_ONION);
    first_break = first_kind(&level, WORLD_BREAKABLE);
    first_spring = first_kind(&level, WORLD_SPRINGBOARD);
    first_exit = first_kind(&level, WORLD_EXIT_RIGHT);
    assert(first_onion >= 0 && first_break >= 0 && first_spring >= 0 && first_exit >= 0);

    assert(level_world_actor_rect(&level, first_onion, &x, &y, &w, &h));
    hit = level_collect_world_onions(&level, x, y, w, h);
    assert(hit >= 1 && level.removed[first_onion] == 1);
    assert(level_collect_world_onions(&level, x, y, w, h) == 0);

    /* A source breakable object may share coarse 8px cells with other
       objects; find a demonstrably breakable, non-solid cell. */
    hit = 0;
    for (i = 0; i < level_world_object_count(&level) && !hit; ++i) {
        const WorldObject *o = level_world_object(&level, i);
        int tx, ty;
        if (o->kind != WORLD_BREAKABLE) continue;
        for (ty = o->top / 8; ty <= (o->top + o->height - 1) / 8 && !hit; ++ty)
            for (tx = o->left / 8; tx <= (o->left + o->width - 1) / 8 && !hit; ++tx)
                if (level_get_tile(&level, tx, ty) == TILE_BREAKABLE) {
                    assert(level_break_tile(&level, tx, ty));
                    assert(!level_object_active(&level, i));
                    hit = 1;
                }
    }
    assert(hit);

    assert(level_world_actor_rect(&level, first_spring, &x, &y, &w, &h));
    player_init(&player, x, y - 10);
    player.grounded = 0;
    player.vy = FX_ONE;
    assert(world_interact_player(&level, &player, y-2, 1) & WORLD_INTERACT_SPRING);
    assert(player.vy < 0);

    assert(level_world_actor_rect(&level, first_exit, &x, &y, &w, &h));
    assert(level_reached_world_exit(&level, x, y, w, h));

    assert(RM_W1_L3_SCENE_MAP_WIDTH == 142);
    assert(RM_W1_L3_SCENE_MAP_HEIGHT == 23);
    assert(RM_W1_L3_SCENE_TILE_COUNT == 473);
    assert(RM_W1_L3_SCENE_TILE_COUNT * 64u <= 32768u);

    level_init_world1_stage(&level, 2);
    assert(level.active_stage == 2 && level_world_object_count(&level) == W1_L2_OBJECT_COUNT);
    level_init_world1_stage(&level, 1);
    assert(level.active_stage == 1 && level_world_object_count(&level) == W1_L1_OBJECT_COUNT);
    level_init_world1_stage(&level, 3);
    assert(level_object_active(&level, first_onion));
    assert(level.actor_frame == 0);

    puts("World 1 Level 3 room/collision/interaction tests: PASS");
    return 0;
}
