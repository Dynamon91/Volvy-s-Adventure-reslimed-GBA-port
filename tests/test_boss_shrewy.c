#include <assert.h>
#include <stdio.h>
#include "core/boss_shrewy.h"
#include "shrewy_room.h"
#include "w1_l1_objects.h"
#include "w1_l2_objects.h"
#include "w1_l3_objects.h"

int main(void)
{
    Level level;
    Player player;
    ShrewyBoss boss;
    InputState none = {0, 0, 0};
    int i;

    level_init_shrewy(&level);
    {
        int counts[6] = {0, 0, 0, 0, 0, 0};
        int i;
        const ShrewyProp *first_hole = 0;
        const ShrewyProp *first_emitter = 0;
        const ShrewyProp *first_digger = 0;
        const ShrewyProp *cage = 0;
        assert(SHREWY_ROOM_PROP_COUNT == 15);
        for (i = 0; i < SHREWY_ROOM_PROP_COUNT; ++i) {
            const ShrewyProp *prop = &shrewy_props[i];
            assert(prop->source_id != 0);
            assert(prop->width > 0 && prop->height > 0);
            if (prop->kind <= SHREWY_PROP_AUTODIGGER) ++counts[prop->kind];
            if (prop->kind == SHREWY_PROP_AUTOHOLE && !first_hole) first_hole = prop;
            if (prop->kind == SHREWY_PROP_EMITTER && !first_emitter) first_emitter = prop;
            if (prop->kind == SHREWY_PROP_AUTODIGGER && !first_digger) first_digger = prop;
            if (prop->kind == SHREWY_PROP_CAGE) cage = prop;
        }
        assert(counts[SHREWY_PROP_ONION] == 4);
        assert(counts[SHREWY_PROP_CAGE] == 1);
        assert(counts[SHREWY_PROP_AUTOHOLE] == 2);
        assert(counts[SHREWY_PROP_EMITTER] == 4);
        assert(counts[SHREWY_PROP_AUTODIGGER] == 4);
        assert(first_hole && first_hole->source_id == 101039);
        assert(first_hole->x == 48 && first_hole->y == 96);
        assert(first_hole->left == 48 && first_hole->top == 96);
        assert(first_hole->width == 32 && first_hole->height == 16);
        assert(first_hole->scale_x_q8 == 512 && first_hole->scale_y_q8 == 256);
        assert(cage && cage->source_id == 101054);
        assert(cage->left == 88 && cage->top == -96);
        assert(first_digger && first_digger->source_id == 101055);
        assert(first_digger->x == 56 && first_digger->y == 112);
        assert(first_emitter && first_emitter->source_id == 101041);
        assert(first_emitter->rotation_tenths == -3064);
        assert(first_emitter->scale_x_q8 == 85 && first_emitter->scale_y_q8 == 320);
    }
    assert(level.boss_room && !level.world1_active);
    assert(level.active_stage == 4);
    assert(level_width_pixels(&level) == 256);
    assert(level_height_pixels(&level) == 144);
    assert(level_get_tile_px(&level, 16, 113) == TILE_GROUND);
    assert(level_get_tile_px(&level, 16, 111) == TILE_EMPTY);
    {
        int onion = -1;
        for (i = 0; i < SHREWY_ROOM_PROP_COUNT; ++i)
            if (shrewy_props[i].kind == SHREWY_PROP_ONION) { onion = i; break; }
        assert(onion >= 0);
        assert(level_collect_world_onions(&level, shrewy_props[onion].left,
                                          shrewy_props[onion].top,
                                          shrewy_props[onion].width,
                                          shrewy_props[onion].height) == 1);
        assert(level.removed[onion]);
        assert(level_collect_world_onions(&level, shrewy_props[onion].left,
                                          shrewy_props[onion].top,
                                          shrewy_props[onion].width,
                                          shrewy_props[onion].height) == 0);
    }

    player_init_native(&player, 50, SHREWY_ARENA_FLOOR_Y - 19);
    assert(player.native_scale);
    assert(player.width == 24 && player.height == 19);
    assert(player.origin_x == 14 && player.origin_y == 11);
    player_update(&player, &none, &level);
    assert(player.grounded);
    assert(fx_to_int(player.y) == SHREWY_ARENA_FLOOR_Y - 19);
    {
        InputState right = {KEY_RIGHT, KEY_RIGHT, 0};
        player_init_native(&player, 50, SHREWY_ARENA_FLOOR_Y - 19);
        player_update(&player, &right, &level);
        assert(player.vx == (FX_ONE * 3 / 10)); /* native-scale acceleration */
    }
    {
        level_init_shrewy(&level);
        player_init_native(&player, 50, SHREWY_ARENA_FLOOR_Y - 19);
        player.onions = 4;
        player_update(&player, &none, &level);
        assert(player.onions == 5 && player.swole);
        assert(player.width == 44 && player.height == 26);
        assert(fx_to_int(player.x) + player.origin_x == 64);
    }

    shrewy_init(&boss);
    assert(fx_to_int(boss.x) == SHREWY_BOSS_SOURCE_X);
    assert(fx_to_int(boss.y) == SHREWY_BOSS_SOURCE_Y);
    assert(boss.health == 3);
    assert(boss.state == SHREWY_IDLE);

    /* A forward hand punch damages Shrewy only during a vulnerable window. */
    player_init_native(&player, 130, 90);
    player.facing = 1;
    player.hand_state = HAND_PUNCH;
    boss.state = SHREWY_RECOVER;
    boss.timer = 20;
    shrewy_update(&boss, &player, 109, 0);
    assert(boss.health == 2 && boss.state == SHREWY_HURT);
    shrewy_update(&boss, &player, 109, 0);
    assert(boss.health == 2); /* no multi-hit from one held attack */

    /* A descending stomp can also connect during the recovery window. */
    boss.state = SHREWY_RECOVER;
    boss.timer = 20;
    boss.invulnerability = 0;
    player_init_native(&player, 165, 48);
    player.vy = FX_ONE;
    shrewy_update(&boss, &player, 60, 1);
    assert(boss.health == 1 && boss.state == SHREWY_HURT);
    assert(player.vy < 0);

    /* Startup -> wind-up -> jump -> stomp -> recovery is deterministic. */
    shrewy_init(&boss);
    player_init_native(&player, 40, 93);
    boss.timer = 1;
    shrewy_update(&boss, &player, 112, 0);
    assert(boss.state == SHREWY_WINDUP);
    boss.timer = 1;
    shrewy_update(&boss, &player, 112, 0);
    assert(boss.state == SHREWY_JUMP);
    for (i = 0; i < 100 && boss.state != SHREWY_RECOVER; ++i)
        shrewy_update(&boss, &player, 112, 0);
    assert(boss.state == SHREWY_RECOVER);

    /* Tornado pattern launches the original rock sprite as a projectile. */
    shrewy_init(&boss);
    player_init_native(&player, 40, 93);
    boss.state = SHREWY_TORNADO;
    boss.timer = 32;
    shrewy_update(&boss, &player, 112, 0);
    assert(boss.rock_active);
    {
        fx32 previous_x = boss.rock_x;
        shrewy_update(&boss, &player, 112, 0);
        assert(boss.rock_x != previous_x);
    }

    /* Last hit transitions into a terminal defeated state. */
    shrewy_init(&boss);
    boss.health = 1;
    boss.state = SHREWY_RECOVER;
    boss.timer = 10;
    boss.invulnerability = 0;
    player_init_native(&player, 130, 90);
    player.facing = 1;
    player.hand_state = HAND_PUNCH;
    shrewy_update(&boss, &player, 109, 0);
    assert(shrewy_is_defeated(&boss));

    puts("Shrewy boss room, native-scale player, attack cycle and damage tests: PASS");
    return 0;
}
