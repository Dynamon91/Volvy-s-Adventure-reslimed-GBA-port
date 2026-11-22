#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "bonus_w1_collision.h"
#include "bonus_w1_room.h"
#include "bonus_w1_bumper_bounds.h"
#include "core/bonus_race.h"
#include "core/fixed.h"
#include "core/input.h"

static const BonusW1RoomInstance *find_kind(BonusW1RoomObjectKind kind,
                                             unsigned int occurrence)
{
    unsigned int i;
    for (i = 0; i < BONUS_W1_ROOM_INSTANCE_COUNT; ++i) {
        if (bonus_w1_room_instances[i].kind == kind) {
            if (occurrence == 0) return &bonus_w1_room_instances[i];
            --occurrence;
        }
    }
    return 0;
}

int main(void)
{
    Level level;
    Player player;
    BonusRace race;
    const BonusW1RoomInstance *onion;
    const BonusW1RoomInstance *start;
    const BonusW1RoomInstance *finish;
    const BonusW1RoomInstance *spring;
    const BonusW1RoomInstance *action_block;
    unsigned int i;
    int one_way_x = -1, one_way_y = -1;
    int break_oneway_x = -1, break_oneway_y = -1;
    unsigned int solid_cells = 0;
    unsigned int breakable_cells = 0;
    unsigned int hazard_cells = 0;

    level_init_bonus_race(&level);
    assert(level.bonus_room);
    assert(!level.world1_active);
    assert(level.active_stage == 5);
    assert(level.width_tiles == 332);
    assert(level.height_tiles == 70);
    assert(level_width_pixels(&level) == 2656);
    assert(level_height_pixels(&level) == 560);
    assert(BONUS_W1_COLLISION_CELLS == 332 * 70);

    for (i = 0; i < BONUS_W1_COLLISION_CELLS; ++i) {
        switch (bonus_w1_collision[i]) {
        case TILE_GROUND: ++solid_cells; break;
        case TILE_BREAKABLE: ++breakable_cells; break;
        case TILE_HAZARD: ++hazard_cells; break;
        default: break;
        }
    }
    assert(solid_cells > 8000);
    assert(breakable_cells > 200);
    assert(hazard_cells > 100);

    for (i = 0; i < BONUS_W1_COLLISION_CELLS; ++i) {
        int tx = (int)(i % BONUS_W1_COLLISION_WIDTH);
        int ty = (int)(i / BONUS_W1_COLLISION_WIDTH);
        if (ty > 0 && bonus_w1_collision[i] == TILE_ONEWAY &&
            bonus_w1_collision[i - BONUS_W1_COLLISION_WIDTH] == TILE_EMPTY &&
            one_way_x < 0) {
            one_way_x = tx;
            one_way_y = ty;
        }
        if (ty > 0 && bonus_w1_collision[i] == TILE_BREAKABLE_ONEWAY &&
            bonus_w1_collision[i - BONUS_W1_COLLISION_WIDTH] == TILE_EMPTY &&
            break_oneway_x < 0) {
            break_oneway_x = tx;
            break_oneway_y = ty;
        }
    }
    assert(one_way_x >= 0 && one_way_y > 0);
    assert(break_oneway_x >= 0 && break_oneway_y > 0);

    /* A one-way cloud platform catches a descending player from above. */
    {
        InputState no_input;
        memset(&no_input, 0, sizeof(no_input));
        player_init(&player, one_way_x * 8, one_way_y * 8 - 10);
        player.grounded = 0;
        player.vy = FX_ONE;
        player_update(&player, &no_input, &level);
        assert(player.grounded);
        assert(fx_to_int(player.y) + player.height == one_way_y * 8);
    }

    /* A one-way destructible block breaks on ground-pound contact. */
    {
        InputState no_input;
        memset(&no_input, 0, sizeof(no_input));
        player_init(&player, break_oneway_x * 8, break_oneway_y * 8 - 10);
        player.grounded = 0;
        player.hand_state = HAND_GROUND_POUND;
        player.hand_timer = 8;
        player.vy = FX_ONE;
        player_update(&player, &no_input, &level);
        assert(level_get_tile(&level, break_oneway_x, break_oneway_y) == TILE_EMPTY);
    }

    /* Breaking a bonus-room block removes its source instance and full AABB. */
    {
        int found = 0;
        for (i = 0; i < BONUS_W1_ROOM_INSTANCE_COUNT; ++i) {
            const BonusW1RoomInstance *candidate = &bonus_w1_room_instances[i];
            int tx, ty;
            if (candidate->kind != BONUS_W1_ROOM_OBJ_DESTRUCTIBLE_BLOCK) continue;
            tx = ((int)candidate->x - 8) / 2 / 8;
            ty = ((int)candidate->y - 8) / 2 / 8;
            if (level_get_tile(&level, tx, ty) != TILE_BREAKABLE) continue;
            assert(level_break_tile(&level, tx, ty));
            assert(level.removed[i]);
            assert(level_get_tile(&level, tx, ty) == TILE_EMPTY);
            found = 1;
            break;
        }
        assert(found);
    }

    /* Swolvy-only 32x32 source blocks clear their complete 16x16 GBA footprint. */
    {
        int found = 0;
        for (i = 0; i < BONUS_W1_ROOM_INSTANCE_COUNT; ++i) {
            const BonusW1RoomInstance *candidate = &bonus_w1_room_instances[i];
            int tx, ty;
            if (candidate->kind != BONUS_W1_ROOM_OBJ_DESTRUCTIBLE_BLOCK_SWOLVY)
                continue;
            tx = ((int)candidate->x - 16) / 2 / 8;
            ty = ((int)candidate->y - 16) / 2 / 8;
            if (level_get_tile(&level, tx, ty) != TILE_SWOLVY_BLOCK) continue;
            assert(level_break_tile(&level, tx, ty));
            assert(level.removed[i]);
            assert(level_get_tile(&level, tx, ty) == TILE_EMPTY);
            found = 1;
            break;
        }
        assert(found);
    }

    /* Normal Volvy must not break the source Swolvy-only collision tile.
     * A powered-up player is allowed to destroy it. */
    {
        InputState no_input;
        memset(&no_input, 0, sizeof(no_input));
        level_init_test_room(&level);
        level_set_tile(&level, 12, 12, TILE_SWOLVY_BLOCK);
        player_init(&player, 12*8, 12*8 - 10);
        player.grounded = 0;
        player.vy = FX_ONE;
        player.hand_state = HAND_GROUND_POUND;
        player.hand_timer = 8;
        player_update(&player, &no_input, &level);
        assert(level_get_tile(&level, 12, 12) == TILE_SWOLVY_BLOCK);
        assert(player.grounded);
        player_init(&player, 12*8, 12*8 - 10);
        player.swole = 1;
        player.grounded = 0;
        player.vy = FX_ONE;
        player.hand_state = HAND_GROUND_POUND;
        player.hand_timer = 8;
        player_update(&player, &no_input, &level);
        assert(level_get_tile(&level, 12, 12) == TILE_EMPTY);
    }

    level_init_bonus_race(&level);
    /* Bonus room trace records >5 collected onions and no Swolvy frames.
     * The provisional 5-onion transformation is therefore disabled here. */
    {
        InputState no_input;
        memset(&no_input, 0, sizeof(no_input));
        level_init_test_room(&level);
        level.bonus_room = 1;
        player_init(&player, 24, 24);
        player.onions = 4;
        level_set_tile(&level, 3, 3, TILE_ONION);
        player_update(&player, &no_input, &level);
        assert(player.onions == 5);
        assert(!player.swole);
    }

    level_init_bonus_race(&level);
    /* The first trace frame places Volvy just before the lower start gate.
     * This location has an empty player body and a solid floor immediately below. */
    player_init(&player, 125, 261);
    assert(level_get_tile_px(&level, 130, 266) == TILE_EMPTY);
    assert(level_get_tile_px(&level, 130, 272) == TILE_GROUND);

    onion = find_kind(BONUS_W1_ROOM_OBJ_ONION, 0);
    assert(onion != 0);
    player.x = fx_from_int((int)onion->x / 2 - 3);
    player.y = fx_from_int((int)onion->y / 2 - 3);
    assert(level_collect_world_onions(&level, fx_to_int(player.x),
                                      fx_to_int(player.y),
                                      player.width, player.height) >= 1);
    assert(level_collect_world_onions(&level, fx_to_int(player.x),
                                      fx_to_int(player.y),
                                      player.width, player.height) == 0);

    bonus_race_init(&race);
    spring = find_kind(BONUS_W1_ROOM_OBJ_SPRINGBOARD, 0);
    assert(spring != 0);
    {
        int spring_left = ((int)spring->x - 8) / 2;
        int spring_top = ((int)spring->y - 8) / 2;
        player_init(&player, spring_left + 1, spring_top - 9);
        player.grounded = 0;
        player.vy = FX_ONE;
        assert(!bonus_race_update(&race, &level, &player,
                                  spring_top - 1, 1));
        assert(player.spring_activations == 1);
        assert(player.vy == -FX_ONE * 9 / 2);
    }

    level_init_bonus_race(&level);
    bonus_race_init(&race);
    action_block = find_kind(BONUS_W1_ROOM_OBJ_ACTION_BLOCK, 0);
    assert(action_block != 0);
    {
        int left = ((int)action_block->x - 8) / 2;
        int top = ((int)action_block->y - 8) / 2;
        player_init(&player, left - 16, top - 2);
        player.facing = 1;
        player.hand_state = HAND_PUNCH;
        (void)bonus_race_update(&race, &level, &player, top - 1, 0);
        assert(player.vy == -FX_ONE * 5 / 2);
        assert(level.actor_cooldown[
            (unsigned int)(action_block - bonus_w1_room_instances)] == 24);
    }

    /* Rotated hitboxes are baked from actual source instance transforms;
     * one 135-degree corner occupies a wider-than-16px GBA AABB. */
    {
        unsigned int b, corners = 0, squares = 0;
        int found_rotated = 0;
        for (b = 0; b < BONUS_W1_BUMPER_COUNT; ++b) {
            const BonusBumperHitbox *hit = &bonus_w1_bumpers[b];
            assert(hit->instance_index < BONUS_W1_ROOM_INSTANCE_COUNT);
            assert(hit->width >= 16 && hit->height >= 16);
            if (hit->corner) {
                ++corners;
                if (hit->width > 16 && hit->height > 16)
                    found_rotated = 1;
            } else {
                ++squares;
                assert(hit->width == 16 && hit->height == 16);
            }
        }
        assert(corners == 4 && squares == 6 && found_rotated);
    }

    /* The original room defines six square and four triangular bumpers.
     * Collision/impulse values are provisional but cooldown must be stable. */
    level_init_bonus_race(&level);
    bonus_race_init(&race);
    {
        const BonusW1RoomInstance *square =
            find_kind(BONUS_W1_ROOM_OBJ_BOUNCE_BLOCK, 0);
        const BonusW1RoomInstance *corner =
            find_kind(BONUS_W1_ROOM_OBJ_CORNER_BUMPER, 0);
        int left, top;
        unsigned int idx;
        assert(square && corner);
        left = square->x / 2 - 8;   /* source scale 2, 16x16 original */
        top = square->y / 2 - 8;
        player_init(&player, left - 7, top + 4);
        idx = (unsigned int)(square - bonus_w1_room_instances);
        assert(!bonus_race_update(&race, &level, &player, 0, 0));
        assert(player.vx == -3 * FX_ONE);
        assert(level.actor_cooldown[idx] == 12);
        assert(race.bumper_hits == 1);
        assert(!bonus_race_update(&race, &level, &player, 0, 0));
        assert(race.bumper_hits == 1); /* no immediate repeated impact */

        level_init_bonus_race(&level);
        bonus_race_init(&race);
        {
            unsigned int b;
            left = 0; top = 0;
            for (b = 0; b < BONUS_W1_BUMPER_COUNT; ++b) {
                if (bonus_w1_bumpers[b].instance_index ==
                    (unsigned int)(corner - bonus_w1_room_instances)) {
                    left = bonus_w1_bumpers[b].x;
                    top = bonus_w1_bumpers[b].y;
                    break;
                }
            }
            assert(b < BONUS_W1_BUMPER_COUNT);
        }
        player_init(&player, left - 7, top - 7);
        assert(!bonus_race_update(&race, &level, &player, 0, 0));
        assert(player.vx == -2 * FX_ONE);
        assert(player.vy == -2 * FX_ONE);
        assert(race.bumper_hits == 1);
    }

    /* Static bunny instances are initialized at exactly their original
     * source positions, but movement starts only when the race gate fires. */
    level_init_bonus_race(&level);
    bonus_race_init(&race);
    {
        unsigned int j;
        for (j = 0; j < BONUS_ACTIVE_BUNNY_COUNT; ++j) {
            assert(race.bunnies[j].frame_counter == j * 19u);
            assert(fx_to_int(race.bunnies[j].x) == race.bunnies[j].home_x);
        }
        player_init(&player, 24, 24);
        (void)bonus_race_update(&race, &level, &player, 0, 0);
        assert(!race.started);
        for (j = 0; j < BONUS_ACTIVE_BUNNY_COUNT; ++j)
            assert(race.bunnies[j].frame_counter == j * 19u);
    }

    bonus_race_init(&race);
    start = find_kind(BONUS_W1_ROOM_OBJ_RACE_START, 1);
    finish = find_kind(BONUS_W1_ROOM_OBJ_RACE_FINISH, 0);
    assert(start && finish);
    assert(start->x == 440 && start->y == 536);
    assert(finish->x == 4984 && finish->y == 840);

    player.x = fx_from_int(210);
    player.y = fx_from_int(263);
    assert(!bonus_race_update(&race, &level, &player, 0, 0));
    assert(race.started);
    assert(race.elapsed_frames == 1);
    assert(level.race_elapsed_frames == 1);

    player.x = fx_from_int(300);
    (void)bonus_race_update(&race, &level, &player, 0, 0);
    assert(race.elapsed_frames == 2);

    /* Finish is the source-derived horizontal crossing plane. */
    player.x = fx_from_int(2477);
    assert(bonus_race_update(&race, &level, &player, 0, 0));
    assert(race.finished);
    assert(race.elapsed_frames == 3);
    (void)bonus_race_update(&race, &level, &player, 0, 0);
    assert(race.elapsed_frames == 3);

    puts("Bonus race controller, source gates, onion collection and collision tests: PASS");
    return 0;
}
