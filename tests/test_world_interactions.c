#include <assert.h>
#include <stdio.h>
#include "core/player.h"
#include "core/level.h"
#include "core/world_interactions.h"
#include "w1_l1_objects.h"

static int find_kind(unsigned int kind, int nth)
{
    int i;
    for (i = 0; i < W1_L1_OBJECT_COUNT; ++i) {
        if (w1_l1_objects[i].kind == kind) {
            if (nth-- == 0) return i;
        }
    }
    return -1;
}

int main(void)
{
    Level level;
    Player player;
    int spring, squirrel, action, i;
    int x, y, w, h;
    unsigned int result;
    level_init_world1(&level);
    spring = find_kind(WORLD_SPRINGBOARD, 0);
    squirrel = find_kind(WORLD_SQUIRREL, 0);
    action = find_kind(WORLD_ACTION_BLOCK, 0);
    assert(spring >= 0 && squirrel >= 0 && action >= 0);
    assert(level_world_actor_rect(&level, spring, &x, &y, &w, &h));
    assert(x == w1_l1_objects[spring].left);
    assert(y == w1_l1_objects[spring].top);

    /* A descending player that crosses a springboard is launched once. */
    player_init(&player, x, y - 9);
    player.vy = FX_ONE;
    result = world_interact_player(&level, &player, y - 1, 1);
    assert((result & WORLD_INTERACT_SPRING) != 0);
    assert(player.vy < -FX_ONE * 4);
    assert(fx_to_int(player.y) + player.height == y);
    assert(player.spring_activations == 1);
    /* Cooldown prevents repeated activation at the same location. */
    player.y = fx_from_int(y - player.height + 1);
    result = world_interact_player(&level, &player, y - 1, 1);
    assert((result & WORLD_INTERACT_SPRING) == 0);
    for (i = 0; i < 16; ++i) level_tick_world_actors(&level);
    player.y = fx_from_int(y - player.height + 1);
    result = world_interact_player(&level, &player, y - 1, 1);
    assert((result & WORLD_INTERACT_SPRING) != 0);

    /* Grounded moving squirrel: patrol stays close to its source pivot. */
    level_init_world1(&level);
    for (i = 0; i < 160; ++i) level_tick_world_actors(&level);
    for (i = 0; i < W1_L1_OBJECT_COUNT; ++i) {
        if (w1_l1_objects[i].kind == WORLD_SQUIRREL) {
            int dx = level_world_actor_offset(&level, i);
            assert(dx >= -12 && dx <= 12);
        }
    }
    level_init_world1(&level);
    assert(level_world_actor_rect(&level, squirrel, &x, &y, &w, &h));

    /* Stomp: downward crossing defeats a squirrel and bounces. */
    player_init(&player, x + 2, y - 8);
    player.vy = FX_ONE;
    result = world_interact_player(&level, &player, y - 1, 1);
    assert((result & WORLD_INTERACT_STOMP) != 0);
    assert((result & WORLD_INTERACT_DEFEAT) != 0);
    assert(!level_object_active(&level, squirrel));
    assert(player.enemies_defeated == 1 && !player.dead);
    assert(player.vy < 0);

    /* Side contact without attack is harmful, independently of bounce logic. */
    level_init_world1(&level);
    player_init(&player, x + 2, y + 2);
    result = world_interact_player(&level, &player, y + 8, 0);
    assert(result & WORLD_INTERACT_HURT);
    assert(player.dead);

    /* Forward punch can hit an enemy in front without body overlap. */
    level_init_world1(&level);
    player_init(&player, x - 15, y + 2);
    player.hand_state = HAND_PUNCH;
    result = world_interact_player(&level, &player,
                                   fx_to_int(player.y) + player.height, 0);
    assert(result & WORLD_INTERACT_DEFEAT);
    assert(!player.dead && player.enemies_defeated == 1);

    /* Action blocks can trigger a temporary rebound, but not continuously. */
    level_init_world1(&level);
    assert(level_world_actor_rect(&level, action, &x, &y, &w, &h));
    player_init(&player, x - 11, y);
    player.hand_state = HAND_PUNCH;
    result = world_interact_player(&level, &player,
                                   fx_to_int(player.y) + player.height, 0);
    assert(result & WORLD_INTERACT_ACTION);
    assert(player.vy < 0);
    result = world_interact_player(&level, &player,
                                   fx_to_int(player.y) + player.height, 0);
    assert(!(result & WORLD_INTERACT_ACTION));

    /* Resetting World 1 clears all removed flags and runtime actor motion. */
    level_init_world1(&level);
    assert(level.actor_frame == 0);
    assert(level_world_actor_offset(&level, squirrel) == 0);
    assert(level_object_active(&level, squirrel));
    puts("World 1 springboard, patrol, stomp, hit and action tests: PASS");
    return 0;
}
