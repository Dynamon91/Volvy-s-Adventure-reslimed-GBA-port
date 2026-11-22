#include <assert.h>
#include <stdio.h>

#include "core/level.h"
#include "core/player.h"

static InputState make_input(unsigned short held, unsigned short pressed)
{
    InputState input;
    input.held = held;
    input.pressed = pressed;
    input.released = 0;
    return input;
}

int main(void)
{
    Level level;
    Player player;
    InputState input;
    int i;
    fx32 start_x;

    level_init_test_room(&level);
    player_init(&player, 24, 134);

    /* Ground contact must remain stable when vertical velocity is zero. */
    input = make_input(0, 0);
    for (i = 0; i < 12; ++i) player_update(&player, &input, &level);
    assert(player.grounded);
    assert(player.y == fx_from_int(134));

    /* Horizontal acceleration is scaled from the original 0.30 px/frame. */
    input = make_input(KEY_RIGHT, 0);
    start_x = player.x;
    for (i = 0; i < 8; ++i) player_update(&player, &input, &level);
    assert(player.x > start_x);
    assert(player.vx <= (FX_ONE * 23 / 20));

    /* Jump starts at the 1/2-scale equivalent of the observed -3.45 velocity. */
    input = make_input(KEY_A, KEY_A);
    player_update(&player, &input, &level);
    assert(player.vy < 0);
    assert(!player.grounded);

    /* Horizontal dash: source trace shows 4 px/frame for about 20 frames. */
    player_init(&player, 20, 122);
    level_set_tile(&level, 4, 16, TILE_BREAKABLE);
    input = make_input(KEY_RIGHT | KEY_B, KEY_B);
    player_update(&player, &input, &level);
    assert(player.hand_state == HAND_DASH);
    assert(player.vx == (FX_ONE * 2));
    assert(level_get_tile(&level, 4, 16) == TILE_EMPTY);

    /* Dash-up is a separate vertical action. */
    player_init(&player, 24, 90);
    player.grounded = 0;
    input = make_input(KEY_UP | KEY_B, KEY_B);
    player_update(&player, &input, &level);
    assert(player.hand_state == HAND_DASH_UP);
    assert(player.vy == -(FX_ONE * 2));

    /* R attaches when the feet enter the test rail row. */
    player_init(&player, 112, 124);
    player.grounded = 0;
    input = make_input(KEY_R, 0);
    player_update(&player, &input, &level);
    assert(player.hand_state == HAND_GRIND);
    assert(player.y == fx_from_int(126));

    /* Hover is scaled from the observed 0.35 px/frame float speed. */
    player_init(&player, 24, 90);
    player.grounded = 0;
    player.vy = fx_from_int(1);
    input = make_input(KEY_B, 0);
    player_update(&player, &input, &level);
    assert(player.hand_state == HAND_HOVER);
    assert(player.vy <= (FX_ONE * 7 / 40));
    assert(player.hover_energy == 59);

    /* Ground-pound velocity matches the trace's 6 px/frame at 1/2 scale. */
    player_init(&player, 24, 90);
    player.grounded = 0;
    input = make_input(KEY_B | KEY_DOWN, KEY_B);
    player_update(&player, &input, &level);
    assert(player.hand_state == HAND_GROUND_POUND);
    assert(player.vy == (FX_ONE * 3));

    /* Ground-pound breaks a breakable tile on downward contact. */
    player_init(&player, 24, 120);
    level_set_tile(&level, 3, 17, TILE_BREAKABLE);
    input = make_input(KEY_B | KEY_DOWN, KEY_B);
    for (i = 0; i < 8; ++i) player_update(&player, &input, &level);
    assert(level_get_tile(&level, 3, 17) == TILE_EMPTY);

    /* Collecting the provisional five-onion test threshold triggers Swolvy. */
    player_init(&player, 48, 134);
    level_set_tile(&level, 6, 17, TILE_ONION);
    level_set_tile(&level, 7, 17, TILE_ONION);
    level_set_tile(&level, 8, 17, TILE_ONION);
    level_set_tile(&level, 9, 17, TILE_ONION);
    level_set_tile(&level, 10, 17, TILE_ONION);
    for (i = 0; i < 4; ++i) {
        player.x = fx_from_int(48 + i * 8);
        player_update(&player, &(InputState){0,0,0}, &level);
    }
    assert(player.swole);
    assert(player.width == 22);
    assert(player.height == 13);
    assert(player.origin_x == 10);
    assert(player.origin_y == 9);
    assert(fx_to_int(player.x) + player.origin_x == 79);
    assert(fx_to_int(player.y) + player.origin_y == 140);

    puts("player tests: PASS");
    return 0;
}
