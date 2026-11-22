#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "core/native_scene.h"
#include "w1_l1_native.h"
#include "w1_l2_native.h"
#include "w1_l3_native.h"
#include "bonus_native.h"
#include "shrewy_native.h"

static uint16_t window[NATIVE_SCENE_WINDOW_CELLS];
static NativeSceneUpload uploads[NATIVE_SCENE_CACHE_TILES];

static void verify_scene(const char *name, const NativeSceneAsset *scene)
{
    NativeSceneCache cache;
    int max_x, max_y, sx, sy;
    unsigned int i;
    unsigned int samples = 0;

    assert(scene != 0);
    assert(scene->tile_count > 0 &&
           scene->tile_count <= NATIVE_SCENE_MAX_SOURCE_TILES);
    assert(scene->tile_bytes == (uint32_t)scene->tile_count * 32u);
    assert(scene->map_width > 0 && scene->map_height > 0);

    for (i = 0; i < (unsigned int)scene->map_width * scene->map_height; ++i) {
        assert(scene->map[i].tile_id < scene->tile_count);
        assert(((scene->map[i].flags >> 2) & 15u) < 15u);
    }

    max_x = (int)scene->map_width * 8 - 240;
    max_y = (int)scene->map_height * 8 - 160;
    if (max_x < 0) max_x = 0;
    if (max_y < 0) max_y = 0;
    native_scene_cache_reset(&cache);

    for (sy = 0; sy <= max_y; sy += (max_y / 5) ? max_y / 5 : 1) {
        for (sx = 0; sx <= max_x; sx += (max_x / 7) ? max_x / 7 : 1) {
            int ox, oy;
            uint16_t upload_count = 0;
            native_scene_window_origin(sx, sy, scene->map_width,
                                       scene->map_height, &ox, &oy);
            assert(native_scene_cache_build(scene, ox, oy, &cache, window,
                                            uploads, &upload_count));
            assert(upload_count <= NATIVE_SCENE_CACHE_TILES);
            for (i = 0; i < NATIVE_SCENE_WINDOW_CELLS; ++i)
                assert((window[i] & 0x03FFu) < NATIVE_SCENE_CACHE_TILES);
            ++samples;
        }
    }
    printf("%s: %u camera-window samples, %u tiles, %u bytes PASS\n",
           name, samples, scene->tile_count, (unsigned int)scene->tile_bytes);
}

int main(void)
{
    verify_scene("W1 L1 native", &g_w1_l1_native_scene);
    verify_scene("W1 L2 native", &g_w1_l2_native_scene);
    verify_scene("W1 L3 native", &g_w1_l3_native_scene);
    verify_scene("Bonus native", &g_bonus_native_scene);
    verify_scene("Shrewy native", &g_shrewy_native_scene);
    return 0;
}
