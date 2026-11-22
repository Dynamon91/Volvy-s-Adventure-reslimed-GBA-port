#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "core/level.h"
#include "core/player.h"
#include "core/world_interactions.h"

#define TEST_OBJECT_COUNT 3

static WorldObject s_objects[TEST_OBJECT_COUNT] = {
    { WORLD_SPRINGBOARD, 40, 60, 12, 8 },
    { WORLD_SQUIRREL, 100, 60, 12, 10 },
    { WORLD_ACTION_BLOCK, 140, 60, 12, 10 }
};

/* Minimal Level API backed by deterministic synthetic objects. */
int level_world_object_count(const Level *level)
{
    return level->world1_active ? TEST_OBJECT_COUNT : 0;
}

const WorldObject *level_world_object(const Level *level, int index)
{
    if (!level->world1_active || index < 0 || index >= TEST_OBJECT_COUNT)
        return 0;
    return &s_objects[index];
}

int level_world_actor_rect(const Level *level, int index,
                           int *left, int *top, int *width, int *height)
{
    const WorldObject *object = level_world_object(level, index);
    if (!object || level->removed[index]) return 0;
    *left = object->left + level->actor_dx[index];
    *top = object->top;
    *width = object->width;
    *height = object->height;
    return 1;
}

void level_tick_world_actors(Level *level)
{
    int i;
    for (i = 0; i < TEST_OBJECT_COUNT; ++i) {
        if (level->actor_cooldown[i]) --level->actor_cooldown[i];
    }
}

int level_object_active(const Level *level, int index)
{
    return level->world1_active && index >= 0 &&
           index < TEST_OBJECT_COUNT && !level->removed[index];
}

static void reset_level(Level *level)
{
    memset(level, 0, sizeof(*level));
    level->world1_active = 1;
    level->width_tiles = 64;
    level->height_tiles = 32;
    s_objects[1].left = 100;
}

static void init_player(Player *player, int x, int y)
{
    memset(player, 0, sizeof(*player));
    player->x = fx_from_int(x);
    player->y = fx_from_int(y);
    player->width = 12;
    player->height = 10;
    player->origin_x = 7;
    player->origin_y = 6;
    player->facing = 1;
}

int main(void)
{
    Level level;
    Player player;
    unsigned int result;
    int i;

    /* A descending crossing activates a springboard once and respects cooldown. */
    reset_level(&level);
    init_player(&player, 40, 51);
    player.vy = FX_ONE;
    result = world_interact_player(&level, &player, 59, 1);
    assert(result == WORLD_INTERACT_SPRING);
    assert(player.vy == -(FX_ONE * 9 / 2));
    assert(fx_to_int(player.y) + player.height == 60);
    assert(player.spring_activations == 1);
    result = world_interact_player(&level, &player, 59, 1);
    assert(result == WORLD_INTERACT_NONE);
    for (i = 0; i < 16; ++i) level_tick_world_actors(&level);
    result = world_interact_player(&level, &player, 59, 1);
    assert(result == WORLD_INTERACT_SPRING);
    assert(player.spring_activations == 2);

    /* A stomp is edge-triggered on a descending top crossing. */
    reset_level(&level);
    init_player(&player, 102, 52);
    player.vy = FX_ONE;
    result = world_interact_player(&level, &player, 59, 1);
    assert((result & WORLD_INTERACT_STOMP) != 0);
    assert((result & WORLD_INTERACT_DEFEAT) != 0);
    assert(level.removed[1] == 1);
    assert(player.enemies_defeated == 1 && !player.dead);
    assert(player.vy == -(FX_ONE * 2));

    /* Side contact hurts; invulnerability suppresses the fatal event. */
    reset_level(&level);
    init_player(&player, 102, 62);
    result = world_interact_player(&level, &player, 72, 0);
    assert(result == WORLD_INTERACT_HURT && player.dead);

    reset_level(&level);
    init_player(&player, 102, 62);
    player.invulnerability = 5;
    result = world_interact_player(&level, &player, 72, 0);
    assert(result == WORLD_INTERACT_NONE && !player.dead);

    /* A forward punch can defeat a squirrel without body overlap. */
    reset_level(&level);
    init_player(&player, 80, 60);
    player.hand_state = HAND_PUNCH;
    result = world_interact_player(&level, &player, 70, 0);
    assert(result == WORLD_INTERACT_DEFEAT);
    assert(level.removed[1] == 1 && player.enemies_defeated == 1);

    /* Action-block rebound is one-shot while its cooldown is active. */
    reset_level(&level);
    init_player(&player, 128, 60);
    player.hand_state = HAND_PUNCH;
    result = world_interact_player(&level, &player, 70, 0);
    assert(result == WORLD_INTERACT_ACTION);
    assert(player.vy == -(FX_ONE * 5 / 2));
    assert(level.actor_cooldown[2] == 24);
    result = world_interact_player(&level, &player, 70, 0);
    assert(result == WORLD_INTERACT_NONE);

    /* Fatal contact must stop this frame's pass before a later action block. */
    reset_level(&level);
    s_objects[1].left = 120;
    init_player(&player, 128, 60);
    player.hand_state = HAND_PUNCH;
    result = world_interact_player(&level, &player, 70, 0);
    assert((result & WORLD_INTERACT_HURT) != 0);
    assert((result & WORLD_INTERACT_ACTION) == 0);
    assert(player.dead && player.vy == 0);
    
    /* Dead players and inactive world modes do not start new interactions. */
    reset_level(&level);
    init_player(&player, 128, 60);
    player.dead = 1;
    player.hand_state = HAND_PUNCH;
    result = world_interact_player(&level, &player, 70, 0);
    assert(result == WORLD_INTERACT_NONE);

    reset_level(&level);
    level.world1_active = 0;
    init_player(&player, 128, 60);
    player.hand_state = HAND_PUNCH;
    result = world_interact_player(&level, &player, 70, 0);
    assert(result == WORLD_INTERACT_NONE);

    puts("World interaction public regressions: PASS (spring, stomp, damage, invulnerability, punch, action cooldown and fatal-frame stop)");
    return 0;
}
