#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <gba.h>
#include "platform/stage_clear_screen.h"

u16 test_bg_colors[256];
u8 test_vram[240u * 160u];
u16 test_mode;
static unsigned int lz77_calls;

/* Minimal valid BIOS-LZ77 header for a 16,200-byte packed 4bpp payload. */
const u8 stage_clear_v198_bin[] = { 0x10u, 0x48u, 0x3Fu, 0x00u };

void LZ77UnCompWram(const void *source, void *destination)
{
    assert(source == stage_clear_v198_bin);
    /* Each source byte becomes pixels 1,2, exercising both nibble paths. */
    memset(destination, 0x21, 16200u);
    ++lz77_calls;
}

int main(void)
{
    unsigned int y;
    assert(stage_clear_screen_prepare() == 1);
    assert(stage_clear_screen_prepare() == 1);
    assert(lz77_calls == 1u);

    stage_clear_screen_commit();
    assert(test_mode == (MODE_4 | BG2_ON));
    assert(test_bg_colors[0] == 0x488Bu);
    assert(test_bg_colors[15] == 0x1C45u);

    for (y = 0; y < 12u; ++y)
        assert(!memcmp(&test_vram[y * 240u], (u8[240]){0}, 240u));
    for (y = 12u; y < 147u; ++y) {
        unsigned int x;
        for (x = 0; x < 240u; ++x)
            assert(test_vram[y * 240u + x] == ((x & 1u) ? 2u : 1u));
    }
    for (y = 147u; y < 160u; ++y)
        assert(!memcmp(&test_vram[y * 240u], (u8[240]){0}, 240u));

    stage_clear_screen_restore();
    assert(test_mode == (MODE_0 | BG0_ON | OBJ_ON | OBJ_1D_MAP));
    puts("Stage Clear runtime module: PASS (prepare-once, nibble expansion, letterbox, palette/frame commit, Mode 0 restore)");
    return 0;
}
