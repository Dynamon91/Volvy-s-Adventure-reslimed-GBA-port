#include "core/camera.h"
#include "core/level.h"

void camera_init(Camera *camera)
{
    camera->x = 0;
    camera->y = 0;
}

void camera_update(Camera *camera, const Player *player,
                   int world_width_px, int world_height_px)
{
    fx32 target_x = player->x + fx_from_int(player->width / 2 - 120);
    fx32 target_y = player->y + fx_from_int(player->height / 2 - 80);
    fx32 max_x = fx_from_int(world_width_px - 240);
    fx32 max_y = fx_from_int(world_height_px - 160);

    /* Modest look-ahead keeps approaching rails and ledges visible. */
    target_x += fx_from_int(player->facing * 16);

    if (max_x < 0) max_x = 0;
    if (max_y < 0) max_y = 0;

    target_x = fx_clamp(target_x, 0, max_x);
    target_y = fx_clamp(target_y, 0, max_y);

    camera->x += (target_x - camera->x) / 8;
    camera->y += (target_y - camera->y) / 8;
}
