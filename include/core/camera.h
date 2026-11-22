#ifndef VOLVY_CORE_CAMERA_H
#define VOLVY_CORE_CAMERA_H

#include "core/fixed.h"
#include "core/player.h"

typedef struct Camera {
    fx32 x;
    fx32 y;
} Camera;

void camera_init(Camera *camera);
void camera_update(Camera *camera, const Player *player,
                   int world_width_px, int world_height_px);

#endif
