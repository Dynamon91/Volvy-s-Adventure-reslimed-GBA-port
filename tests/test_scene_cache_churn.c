/* Asset-independent stress regression for the native BG1 32x32 tile window.
 * Checks the optimized monotonic free-slot allocator against an independent
 * first-fit allocation oracle over incremental scrolling and teleports. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "core/native_scene.h"

#define MAP_WIDTH 81
#define MAP_HEIGHT 69
#define SOURCE_TILES 2048
static NativeSceneCell map[MAP_WIDTH * MAP_HEIGHT];
static NativeSceneAsset scene;
static NativeSceneCache cache;
static NativeSceneCache before;
static uint16_t screen_map[NATIVE_SCENE_WINDOW_CELLS];
static NativeSceneUpload transfers[NATIVE_SCENE_CACHE_TILES];
static uint8_t wanted[SOURCE_TILES];
static uint16_t reference_slots[NATIVE_SCENE_CACHE_TILES];
static uint16_t expected_transfer_sources[NATIVE_SCENE_CACHE_TILES];
static uint16_t expected_transfer_slots[NATIVE_SCENE_CACHE_TILES];

static void verify_window(int ox, int oy)
{
    uint16_t n = 0;
    uint16_t expected_count = 0;
    int x, y;
    unsigned int i, j;
    memcpy(&before, &cache, sizeof(before));
    assert(native_scene_cache_build(&scene, ox, oy, &cache, screen_map,
                                    transfers, &n));
    memset(wanted, 0, sizeof(wanted));
    for (y = 0; y < NATIVE_SCENE_WINDOW_H; ++y)
        for (x = 0; x < NATIVE_SCENE_WINDOW_W; ++x) {
            const NativeSceneCell *cell = &map[(oy + y) * MAP_WIDTH + ox + x];
            wanted[cell->tile_id] = 1;
        }
    for (i = 0; i < NATIVE_SCENE_CACHE_TILES; ++i) {
        uint16_t src = before.slot_to_source[i];
        reference_slots[i] = (src != NATIVE_SCENE_INVALID_TILE &&
                              src < SOURCE_TILES && wanted[src])
                             ? src : NATIVE_SCENE_INVALID_TILE;
    }
    for (i = 0; i < SOURCE_TILES; ++i) {
        int resident = 0;
        if (!wanted[i]) continue;
        for (j = 0; j < NATIVE_SCENE_CACHE_TILES; ++j)
            if (reference_slots[j] == i) { resident = 1; break; }
        if (resident) continue;
        for (j = 0; j < NATIVE_SCENE_CACHE_TILES; ++j)
            if (reference_slots[j] == NATIVE_SCENE_INVALID_TILE) break;
        assert(j < NATIVE_SCENE_CACHE_TILES);
        reference_slots[j] = (uint16_t)i;
        expected_transfer_sources[expected_count] = (uint16_t)i;
        expected_transfer_slots[expected_count] = (uint16_t)j;
        ++expected_count;
    }
    assert(n == expected_count);
    for (i = 0; i < n; ++i) {
        assert(transfers[i].source_tile == expected_transfer_sources[i]);
        assert(transfers[i].cache_slot == expected_transfer_slots[i]);
    }
    for (i = 0; i < NATIVE_SCENE_CACHE_TILES; ++i) {
        uint16_t src = reference_slots[i];
        assert(cache.slot_to_source[i] == src);
        if (src != NATIVE_SCENE_INVALID_TILE)
            assert(cache.source_to_slot[src] == (int16_t)i);
    }
    for (y = 0; y < NATIVE_SCENE_WINDOW_H; ++y)
        for (x = 0; x < NATIVE_SCENE_WINDOW_W; ++x) {
            unsigned int screen_index = (unsigned int)(y * NATIVE_SCENE_WINDOW_W + x);
            const NativeSceneCell *cell = &map[(oy + y) * MAP_WIDTH + ox + x];
            uint16_t slot = (uint16_t)cache.source_to_slot[cell->tile_id];
            uint16_t expected = (uint16_t)(slot |
                ((cell->flags & 1u) ? 0x400u : 0u) |
                ((cell->flags & 2u) ? 0x800u : 0u) |
                (((cell->flags >> 2) & 15u) << 12));
            assert(screen_map[screen_index] == expected);
        }
}

int main(void)
{
    int x, y, n;
    uint32_t state = 0x92A5E194u;
    for (y = 0; y < MAP_HEIGHT; ++y)
        for (x = 0; x < MAP_WIDTH; ++x) {
            map[y * MAP_WIDTH + x].tile_id =
                (uint16_t)((x * 73 + y * 127 + (x * y) % 31) % SOURCE_TILES);
            map[y * MAP_WIDTH + x].flags =
                (uint16_t)((x + y) % 4 | (((x * 3 + y) % 16) << 2));
        }
    memset(&scene, 0, sizeof(scene));
    scene.map = map;
    scene.tile_count = SOURCE_TILES;
    scene.map_width = MAP_WIDTH;
    scene.map_height = MAP_HEIGHT;
    native_scene_cache_reset(&cache);
    /* Exactly three windows per iteration: a warp followed by one-tile
     * horizontal and vertical movements, including map-boundary cases. */
    for (n = 0; n < 180; ++n) {
        state = state * 1664525u + 1013904223u;
        x = (int)((state >> 9) % (MAP_WIDTH - NATIVE_SCENE_WINDOW_W + 1));
        state = state * 1664525u + 1013904223u;
        y = (int)((state >> 10) % (MAP_HEIGHT - NATIVE_SCENE_WINDOW_H + 1));
        verify_window(x, y);
        verify_window(x < MAP_WIDTH - NATIVE_SCENE_WINDOW_W ? x + 1 : x - 1, y);
        verify_window(x, y < MAP_HEIGHT - NATIVE_SCENE_WINDOW_H ? y + 1 : y - 1);
    }
    puts("Native 4bpp scene cache first-fit equivalence under 540 mixed scroll/warp windows: PASS");
    return 0;
}
