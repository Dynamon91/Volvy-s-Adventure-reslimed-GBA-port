#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "core/native_scene.h"

static NativeSceneCell map[40 * 40];
static uint8_t tiles[1600 * 32];
static uint16_t palette[256];
static uint16_t output[NATIVE_SCENE_WINDOW_CELLS];
static NativeSceneUpload uploads[NATIVE_SCENE_CACHE_TILES];
static NativeSceneCache cache;

static void check_entries(const NativeSceneAsset *scene, int ox, int oy)
{
    int x, y;
    for (y = 0; y < NATIVE_SCENE_WINDOW_H; ++y) {
        for (x = 0; x < NATIVE_SCENE_WINDOW_W; ++x) {
            unsigned int i = (unsigned int)(y * NATIVE_SCENE_WINDOW_W + x);
            int sx = ox + x, sy = oy + y;
            uint16_t src = 0, flags = 0, slot, expected;
            if (sx < scene->map_width && sy < scene->map_height) {
                src = scene->map[sy * scene->map_width + sx].tile_id;
                flags = scene->map[sy * scene->map_width + sx].flags;
            }
            assert(src < scene->tile_count);
            assert(cache.source_to_slot[src] >= 0);
            slot = (uint16_t)cache.source_to_slot[src];
            expected = (uint16_t)(slot |
                ((flags & 1u) ? 0x0400u : 0u) |
                ((flags & 2u) ? 0x0800u : 0u) |
                (((flags >> 2) & 15u) << 12));
            assert(output[i] == expected);
        }
    }
}

int main(void)
{
    NativeSceneAsset scene;
    uint16_t upload_count = 0;
    unsigned int i;
    int x, y, ox, oy;

    for (i = 0; i < sizeof(tiles); ++i) tiles[i] = (uint8_t)(i * 13u);
    for (y = 0; y < 40; ++y) {
        for (x = 0; x < 40; ++x) {
            NativeSceneCell *cell = &map[y * 40 + x];
            cell->tile_id = (uint16_t)(y * 40 + x);
            cell->flags = (uint16_t)(((x + y) & 3) | (((x + y) % 8) << 2));
        }
    }
    scene.tiles = tiles;
    scene.map = map;
    scene.palette = palette;
    scene.tile_count = 1600;
    scene.map_width = 40;
    scene.map_height = 40;
    scene.tile_bytes = sizeof(tiles);

    native_scene_cache_reset(&cache);
    assert(native_scene_cache_build(&scene, 0, 0, &cache, output,
                                    uploads, &upload_count));
    assert(upload_count == 1024);
    for (i = 0; i < upload_count; ++i) {
        assert(uploads[i].source_tile < scene.tile_count);
        assert(uploads[i].cache_slot < NATIVE_SCENE_CACHE_TILES);
    }
    check_entries(&scene, 0, 0);

    /* One-column scroll: retain 31 columns and upload only the new column. */
    assert(native_scene_cache_build(&scene, 1, 0, &cache, output,
                                    uploads, &upload_count));
    assert(upload_count == 32);
    check_entries(&scene, 1, 0);

    /* One-row scroll after that: only the new bottom row is uploaded. */
    assert(native_scene_cache_build(&scene, 1, 1, &cache, output,
                                    uploads, &upload_count));
    assert(upload_count == 32);
    check_entries(&scene, 1, 1);

    native_scene_window_origin(0, 0, 283, 45, &ox, &oy);
    assert(ox == 0 && oy == 0);
    native_scene_window_origin(2018, 200, 283, 45, &ox, &oy);
    assert(ox == 251 && oy == 13);
    assert(2018 - ox * 8 <= 16);
    assert(200 - oy * 8 <= 96);
    native_scene_window_origin(1000, 1000, 32, 18, &ox, &oy);
    assert(ox == 0 && oy == 0);

    puts("Native 4bpp scene cache: PASS (1024-tile working set, H/V/palette flags, incremental scroll)");
    return 0;
}
