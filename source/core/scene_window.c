#include "core/scene_window.h"

void scene_window_origin(int camera_px_x, int camera_px_y,
                         int source_tiles_w, int source_tiles_h,
                         int *out_tile_x, int *out_tile_y)
{
    const int halfblock_pixels = 32 * 8;
    const int vertical_slack = SCENE_WINDOW_TILES_H * 8 - SCENE_WINDOW_VIEW_H;
    int start_x, start_y, max_x, max_y;
    if (camera_px_x < 0) camera_px_x = 0;
    if (camera_px_y < 0) camera_px_y = 0;

    /* Horizontal camera may move 0..255px before a 32-tile page shift. */
    start_x = (camera_px_x / halfblock_pixels) * 32;
    /* Vertical camera has only 96px of slack inside its 256px page. */
    start_y = camera_px_y > vertical_slack
        ? (camera_px_y - vertical_slack + 7) / 8 : 0;

    max_x = source_tiles_w - SCENE_WINDOW_TILES_W;
    max_y = source_tiles_h - SCENE_WINDOW_TILES_H;
    if (max_x < 0) max_x = 0;
    if (max_y < 0) max_y = 0;
    if (start_x > max_x) start_x = max_x;
    if (start_y > max_y) start_y = max_y;
    *out_tile_x = start_x;
    *out_tile_y = start_y;
}

void scene_window_copy(const uint16_t *source,
                       int source_tiles_w, int source_tiles_h,
                       int origin_tile_x, int origin_tile_y,
                       uint16_t destination[SCENE_WINDOW_TILES_W * SCENE_WINDOW_TILES_H])
{
    int x,y;
    for (y=0; y<SCENE_WINDOW_TILES_H; ++y) {
        for (x=0; x<SCENE_WINDOW_TILES_W; ++x) {
            const int source_x = origin_tile_x + x;
            const int source_y = origin_tile_y + y;
            const int block = (x / 32) * 1024;
            const int output_index = block + y * 32 + (x & 31);
            uint16_t tile=0;
            if (source_x >= 0 && source_x < source_tiles_w &&
                source_y >= 0 && source_y < source_tiles_h) {
                tile=source[source_y * source_tiles_w + source_x];
            }
            destination[output_index] = tile;
        }
    }
}
