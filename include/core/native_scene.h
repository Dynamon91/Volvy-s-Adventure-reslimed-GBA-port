#ifndef VOLVY_CORE_NATIVE_SCENE_H
#define VOLVY_CORE_NATIVE_SCENE_H

#include <stdint.h>

#define NATIVE_SCENE_WINDOW_W 32
#define NATIVE_SCENE_WINDOW_H 32
#define NATIVE_SCENE_WINDOW_CELLS (NATIVE_SCENE_WINDOW_W * NATIVE_SCENE_WINDOW_H)
#define NATIVE_SCENE_CACHE_TILES 1024
#define NATIVE_SCENE_MAX_SOURCE_TILES 2048
#define NATIVE_SCENE_INVALID_TILE 0xFFFFu

typedef struct NativeSceneCell {
    uint16_t tile_id;
    uint16_t flags; /* bit0 H-flip, bit1 V-flip, bits2..5 palette bank */
} NativeSceneCell;

typedef struct NativeSceneAsset {
    const uint8_t *tiles;             /* 32 bytes per 4bpp 8x8 tile */
    const NativeSceneCell *map;
    const uint16_t *palette;          /* 16 x 16 BGR555; bank 15 reserved for HUD */
    uint16_t tile_count;
    uint16_t map_width;
    uint16_t map_height;
    uint32_t tile_bytes;
} NativeSceneAsset;

typedef struct NativeSceneCache {
    uint16_t slot_to_source[NATIVE_SCENE_CACHE_TILES];
    int16_t source_to_slot[NATIVE_SCENE_MAX_SOURCE_TILES];
    uint8_t needed[NATIVE_SCENE_MAX_SOURCE_TILES];
} NativeSceneCache;

typedef struct NativeSceneUpload {
    uint16_t source_tile;
    uint16_t cache_slot;
} NativeSceneUpload;

/* Presentation tracks the map window that is actually installed in BG VRAM.
 * Updating the desired streamed window must not move BG scroll early. */
typedef struct NativeScenePresentation {
    int visible_start_x;
    int visible_start_y;
    uint8_t has_visible_map;
} NativeScenePresentation;

void native_scene_presentation_reset(NativeScenePresentation *presentation);
void native_scene_presentation_commit(NativeScenePresentation *presentation,
                                      int start_x, int start_y);
/* Clamp the rendered camera to the window that is actually on screen while
 * new tile data/map entries are uploaded. Returns 1 if a distant camera jump
 * requires BG1 to be hidden until the new map is ready. */
int native_scene_presentation_camera(const NativeScenePresentation *presentation,
                                     int desired_x, int desired_y,
                                     int pending_start_x, int pending_start_y,
                                     int *out_x, int *out_y);

void native_scene_cache_reset(NativeSceneCache *cache);
void native_scene_window_origin(int camera_px_x, int camera_px_y,
                                int source_tiles_w, int source_tiles_h,
                                int *out_tile_x, int *out_tile_y);
int native_scene_cache_build(const NativeSceneAsset *scene,
                             int origin_tile_x, int origin_tile_y,
                             NativeSceneCache *cache,
                             uint16_t destination[NATIVE_SCENE_WINDOW_CELLS],
                             NativeSceneUpload uploads[NATIVE_SCENE_CACHE_TILES],
                             uint16_t *upload_count);

#endif
