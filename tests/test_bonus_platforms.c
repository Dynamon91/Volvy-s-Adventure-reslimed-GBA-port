#include <assert.h>
#include <stdio.h>

#include "bonus_w1_room.h"
#include "core/bonus_platforms.h"
#include "core/fixed.h"

static int player_left(const Player *p) { return fx_to_int(p->x); }

int main(void)
{
    Level level;
    Player player;
    BonusMovingPlatform platforms[BONUS_MOVING_PLATFORM_COUNT];
    unsigned int i, found = 0;
    int top, old_player_x, old_player_bottom;
    fx32 before;

    level_init_bonus_race(&level);
    bonus_platforms_init(platforms);
    for (i = 0; i < BONUS_W1_ROOM_INSTANCE_COUNT; ++i) {
        const BonusW1RoomInstance *source = &bonus_w1_room_instances[i];
        const BonusMovingPlatform *platform;
        if (source->kind != BONUS_W1_ROOM_OBJ_MOVING_PLATFORM) continue;
        platform = bonus_platform_for_instance(platforms, i);
        assert(platform);
        assert(platform->source_instance_index == i);
        assert(fx_to_int(platform->x) == source->x / 2);
        assert(fx_to_int(platform->y) == source->y / 2);
        assert(platform->width == (found == 0 ? 32 : 24));
        assert(platform->height == 4);
        ++found;
    }
    assert(found == 2);

    /* The static collision grid excludes the moving decks. Their positions
     * are now dynamic AABBs and must not leave stale solid/one-way tiles. */
    for (i = 0; i < BONUS_MOVING_PLATFORM_COUNT; ++i) {
        int x = fx_to_int(platforms[i].x);
        int y = fx_to_int(platforms[i].y) - platforms[i].height;
        assert(level_get_tile_px(&level, x, y) != TILE_ONEWAY);
    }

    top = fx_to_int(platforms[0].y) - platforms[0].height;
    player_init(&player, fx_to_int(platforms[0].x) - 8, top - 10);
    player.grounded = 0;
    player.vy = FX_ONE;
    old_player_x = player_left(&player);
    old_player_bottom = top - 1;
    player.y += FX_ONE;
    bonus_platforms_update(platforms, &player,
                           old_player_x, old_player_bottom, 1);
    assert(player.grounded);
    assert(player.vy == 0);
    assert(fx_to_int(player.y) + player.height == top);
    assert(platforms[0].x > platforms[0].home_x);

    /* A standing player is carried by fractional platform motion. */
    before = player.x;
    old_player_x = player_left(&player);
    old_player_bottom = fx_to_int(player.y) + player.height;
    bonus_platforms_update(platforms, &player,
                           old_player_x, old_player_bottom, 1);
    assert(player.x > before);
    assert(player.grounded);

    /* One-way semantics: ascending through the platform never attaches. */
    player_init(&player, fx_to_int(platforms[1].x) - 8,
                fx_to_int(platforms[1].y) - 2);
    player.grounded = 0;
    player.vy = -FX_ONE;
    bonus_platforms_update(platforms, &player,
                           player_left(&player),
                           fx_to_int(player.y) + player.height, 0);
    assert(!player.grounded);
    assert(player.vy < 0);

    /* Source-anchored horizontal motion stays in the provisional ±24px zone. */
    for (i = 0; i < 600; ++i) {
        int b;
        player.vy = -FX_ONE;
        bonus_platforms_update(platforms, &player, player_left(&player),
                               fx_to_int(player.y) + player.height, 0);
        for (b = 0; b < BONUS_MOVING_PLATFORM_COUNT; ++b) {
            assert(platforms[b].x >= platforms[b].home_x - fx_from_int(24));
            assert(platforms[b].x <= platforms[b].home_x + fx_from_int(24));
            assert(platforms[b].direction == 1 || platforms[b].direction == -1);
        }
    }
    bonus_platforms_init(platforms);
    for (i = 0; i < BONUS_MOVING_PLATFORM_COUNT; ++i) {
        assert(platforms[i].x == platforms[i].home_x);
    }
    puts("Bonus moving platform source anchors, 2:1 sprite bounds, carry, one-way and reset tests: PASS");
    return 0;
}
