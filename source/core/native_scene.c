#include <string.h>
#include "core/native_scene.h"

void native_scene_presentation_reset(NativeScenePresentation *presentation)
{
    presentation->visible_start_x = 0;
    presentation->visible_start_y = 0;
    presentation->has_visible_map = 0;
}

void native_scene_presentation_commit(NativeScenePresentation *presentation,
                                      int start_x, int start_y)
{
    presentation->visible_start_x = start_x;
    presentation->visible_start_y = start_y;
    presentation->has_visible_map = 1;
}

static int clamp_range(int value, int lo, int hi)
{
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}

int native_scene_presentation_camera(const NativeScenePresentation *presentation,
                                     int desired_x, int desired_y,
                                     int pending_start_x, int pending_start_y,
                                     int *out_x, int *out_y)
{
    int xmin, ymin, xmax, ymax;
    int gap_x, gap_y;
    if (!presentation->has_visible_map) {
        *out_x = desired_x;
        *out_y = desired_y;
        return 0;
    }
    xmin = presentation->visible_start_x * 8;
    ymin = presentation->visible_start_y * 8;
    xmax = xmin + (NATIVE_SCENE_WINDOW_W * 8 - 240);
    ymax = ymin + (NATIVE_SCENE_WINDOW_H * 8 - 160);
    *out_x = clamp_range(desired_x, xmin, xmax);
    *out_y = clamp_range(desired_y, ymin, ymax);

    /* A teleport can swap the map to a region disjoint from the old camera.
     * In that exceptional frame the BG remains hidden until the renderer
     * positions both BG scroll and OBJ using the new committed map. */
    gap_x = *out_x - pending_start_x * 8;
    gap_y = *out_y - pending_start_y * 8;
    return gap_x < 0 || gap_y < 0 ||
           gap_x > NATIVE_SCENE_WINDOW_W * 8 - 240 ||
           gap_y > NATIVE_SCENE_WINDOW_H * 8 - 160;
}

void native_scene_cache_reset(NativeSceneCache *cache)
{
    unsigned int i;
    for (i = 0; i < NATIVE_SCENE_CACHE_TILES; ++i)
        cache->slot_to_source[i] = NATIVE_SCENE_INVALID_TILE;
    for (i = 0; i < NATIVE_SCENE_MAX_SOURCE_TILES; ++i) {
        cache->source_to_slot[i] = -1;
        cache->needed[i] = 0;
    }
}

void native_scene_window_origin(int camera_px_x, int camera_px_y,
                                int source_tiles_w, int source_tiles_h,
                                int *out_tile_x, int *out_tile_y)
{
    const int slack_x = NATIVE_SCENE_WINDOW_W * 8 - 240;
    const int slack_y = NATIVE_SCENE_WINDOW_H * 8 - 160;
    int start_x, start_y, max_x, max_y;

    if (camera_px_x < 0) camera_px_x = 0;
    if (camera_px_y < 0) camera_px_y = 0;

    /* Keep the camera inside a 256x256px map window with at most 16px
       horizontal and 96px vertical slack. */
    start_x = camera_px_x > slack_x
        ? (camera_px_x - slack_x + 7) / 8 : 0;
    start_y = camera_px_y > slack_y
        ? (camera_px_y - slack_y + 7) / 8 : 0;

    max_x = source_tiles_w - NATIVE_SCENE_WINDOW_W;
    max_y = source_tiles_h - NATIVE_SCENE_WINDOW_H;
    if (max_x < 0) max_x = 0;
    if (max_y < 0) max_y = 0;
    if (start_x > max_x) start_x = max_x;
    if (start_y > max_y) start_y = max_y;

    *out_tile_x = start_x;
    *out_tile_y = start_y;
}

int native_scene_cache_build(const NativeSceneAsset *scene,
                             int origin_tile_x, int origin_tile_y,
                             NativeSceneCache *cache,
                             uint16_t destination[NATIVE_SCENE_WINDOW_CELLS],
                             NativeSceneUpload uploads[NATIVE_SCENE_CACHE_TILES],
                             uint16_t *upload_count)
{
    unsigned int i;
    unsigned int free_slot = 0;
    int x, y;
    uint16_t count = 0;

    if (!scene || !cache || !destination || !uploads || !upload_count ||
        scene->tile_count == 0 ||
        scene->tile_count > NATIVE_SCENE_MAX_SOURCE_TILES)
        return 0;

    memset(cache->needed, 0, sizeof(cache->needed));

    for (y = 0; y < NATIVE_SCENE_WINDOW_H; ++y) {
        for (x = 0; x < NATIVE_SCENE_WINDOW_W; ++x) {
            int sx = origin_tile_x + x;
            int sy = origin_tile_y + y;
            uint16_t tile_id = 0;
            if (sx >= 0 && sx < scene->map_width &&
                sy >= 0 && sy < scene->map_height) {
                tile_id = scene->map[sy * scene->map_width + sx].tile_id;
                if (tile_id >= scene->tile_count) return 0;
            }
            cache->needed[tile_id] = 1;
        }
    }

    /* Retain cached tiles still referenced by the new 32x32 window. */
    for (i = 0; i < NATIVE_SCENE_CACHE_TILES; ++i) {
        uint16_t source_id = cache->slot_to_source[i];
        if (source_id == NATIVE_SCENE_INVALID_TILE) continue;
        if (source_id < scene->tile_count && cache->needed[source_id]) {
            cache->source_to_slot[source_id] = (int16_t)i;
        } else {
            if (source_id < NATIVE_SCENE_MAX_SOURCE_TILES &&
                cache->source_to_slot[source_id] == (int16_t)i)
                cache->source_to_slot[source_id] = -1;
            cache->slot_to_source[i] = NATIVE_SCENE_INVALID_TILE;
        }
    }

    /* Assign slots to newly visible source tiles. The 32x32 map has at most
       1024 cells, so its unique-tile working set always fits the cache. */
    /* Source ids are visited in ascending order; the lowest vacant slot is
     * always consumed first. A monotonic cursor gives identical allocations
     * without rescanning the already-occupied prefix for every new tile. */
    for (i = 0; i < scene->tile_count; ++i) {
        if (!cache->needed[i] || cache->source_to_slot[i] >= 0) continue;
        while (free_slot < NATIVE_SCENE_CACHE_TILES &&
               cache->slot_to_source[free_slot] != NATIVE_SCENE_INVALID_TILE)
            ++free_slot;
        if (free_slot >= NATIVE_SCENE_CACHE_TILES ||
            count >= NATIVE_SCENE_CACHE_TILES)
            return 0;
        cache->slot_to_source[free_slot] = (uint16_t)i;
        cache->source_to_slot[i] = (int16_t)free_slot;
        uploads[count].source_tile = (uint16_t)i;
        uploads[count].cache_slot = (uint16_t)free_slot;
        ++count;
        ++free_slot;
    }

    for (y = 0; y < NATIVE_SCENE_WINDOW_H; ++y) {
        for (x = 0; x < NATIVE_SCENE_WINDOW_W; ++x) {
            int sx = origin_tile_x + x;
            int sy = origin_tile_y + y;
            unsigned int output_index = (unsigned int)(y * NATIVE_SCENE_WINDOW_W + x);
            uint16_t tile_id = 0;
            uint16_t flags = 0;
            uint16_t slot;
            uint16_t entry;

            if (sx >= 0 && sx < scene->map_width &&
                sy >= 0 && sy < scene->map_height) {
                const NativeSceneCell *cell =
                    &scene->map[sy * scene->map_width + sx];
                tile_id = cell->tile_id;
                flags = cell->flags;
            }
            if (tile_id >= scene->tile_count) return 0;
            if (cache->source_to_slot[tile_id] < 0) return 0;
            slot = (uint16_t)cache->source_to_slot[tile_id];
            entry = (uint16_t)(slot |
                ((flags & 1u) ? 0x0400u : 0u) |
                ((flags & 2u) ? 0x0800u : 0u) |
                (((flags >> 2) & 15u) << 12));
            destination[output_index] = entry;
        }
    }

    *upload_count = count;
    return 1;
}
