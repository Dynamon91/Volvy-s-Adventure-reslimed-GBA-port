#ifndef VOLVY_CORE_SCENE_WINDOW_H
#define VOLVY_CORE_SCENE_WINDOW_H
#include <stdint.h>

#define SCENE_WINDOW_TILES_W 64
#define SCENE_WINDOW_TILES_H 32
#define SCENE_WINDOW_VIEW_W 240
#define SCENE_WINDOW_VIEW_H 160

/* Select an origin for a 64x32 GBA BG map, in source tile coordinates.
 * In particular, the 70-tile-tall bonus room needs a sliding Y origin:
 * jumping Y by 32 tiles would reveal an invalid 160-pixel viewport edge.
 */
void scene_window_origin(int camera_px_x, int camera_px_y,
                         int source_tiles_w, int source_tiles_h,
                         int *out_tile_x, int *out_tile_y);

/* Copy source tiles to GBA BG_SIZE_1 (64x32) screen-block order. */
void scene_window_copy(const uint16_t *source,
                       int source_tiles_w, int source_tiles_h,
                       int origin_tile_x, int origin_tile_y,
                       uint16_t destination[SCENE_WINDOW_TILES_W * SCENE_WINDOW_TILES_H]);
#endif
