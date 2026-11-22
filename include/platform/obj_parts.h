#ifndef VOLVY_PLATFORM_OBJ_PARTS_H
#define VOLVY_PLATFORM_OBJ_PARTS_H

#include <gba.h>
#include "player_sprites.h"

/* Pack only visible OBJ parts into OAM, preserving the source frame tile
 * offsets for the next VBlank upload. Result is the next free OAM slot.
 * This function changes neither source art nor animation frame indices. */
int volvy_obj_draw_frame_parts(const GbaSpriteFrame *frame, OBJATTR *oam,
                               int first_oam, int base_x, int base_y,
                               int facing_left, int base_tile64,
                               unsigned int priority);

#endif
