#include <assert.h>
#include <stdio.h>
#include "core/player.h"
#include "core/level.h"
#include "core/camera.h"

int main(void)
{
    Level level;
    Player player;
    Camera cam;
    InputState none = {0,0,0};
    int i;
    level_init_world1(&level);
    /* Source onion instance pivot (264,328) -> GBA pivot (132,164). */
    player_init(&player, 125, 154);
    player_update(&player, &none, &level);
    assert(player.onions >= 1);
    assert(player.onions < VOLVY_ONIONS_TO_SWOLVY);
    /* First source breakable block pivot (312,296) -> GBA tile (19,18). */
    level_init_world1(&level);
    player_init(&player, 135, 143);
    player.grounded = 0;
    {
        InputState dash = { KEY_RIGHT|KEY_B, KEY_B, 0 };
        player_update(&player, &dash, &level);
        dash.pressed = 0;
        for (i=0;i<12;++i) player_update(&player, &dash, &level);
        assert(level_get_tile(&level,19,18) == TILE_EMPTY);
    }
    camera_init(&cam);
    player.x = fx_from_int(850);
    player.y = fx_from_int(140);
    for (i=0;i<90;++i) camera_update(&cam,&player,1129,180);
    assert(fx_to_int(cam.x)>500);
    assert(fx_to_int(cam.x)<=889);
    assert(fx_to_int(cam.y)<=20);
    puts("World 1 player/camera gameplay integration tests: PASS");
    return 0;
}
