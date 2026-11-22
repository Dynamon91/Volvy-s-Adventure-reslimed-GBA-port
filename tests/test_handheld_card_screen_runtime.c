#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <gba.h>
#include "platform/handheld_card_screen.h"

u16 test_bg_colors[256];
u8 test_vram[240u * 160u];
u16 test_mode;
static unsigned int lz77_calls;
static const u8 valid_pixels[] = {0x10u, 0x90u, 0x7Eu, 0x00u};
static const u8 alternative_pixels[] = {0x10u, 0x90u, 0x7Eu, 0x00u};
static const u8 bad_header[] = {0x20u, 0x90u, 0x7Eu, 0x00u};
static const u8 bad_size[] = {0x10u, 0x40u, 0x3Fu, 0x00u};
static const u16 palette[] = {0x1234u, 0x7FFFu, 0x0000u, 0x4321u};
static const u16 alternative_palette[] = {0x4567u, 0x001Fu};
static const HandheldCardAsset card = {valid_pixels, 4u, palette, 4u};

void LZ77UnCompWram(const void *source, void *destination)
{
    unsigned int y, x;
    u8 *pixels = destination;
    assert(source == valid_pixels || source == alternative_pixels);
    for (y = 0u; y < 135u; ++y)
        for (x = 0u; x < 240u; ++x)
            pixels[y * 240u + x] = (u8)(1u + (x % 3u));
    ++lz77_calls;
}

int main(void)
{
    unsigned int y, x;
    HandheldCardAsset invalid = {bad_header, 4u, palette, 4u};
    HandheldCardAsset reused = card;
    assert(!handheld_card_screen_prepare(0));
    assert(!handheld_card_screen_prepare(&invalid));
    invalid.lz77 = bad_size;
    assert(!handheld_card_screen_prepare(&invalid));
    invalid.lz77 = valid_pixels;
    invalid.palette_entries = 0;
    assert(!handheld_card_screen_prepare(&invalid));
    invalid.palette_entries = 257;
    assert(!handheld_card_screen_prepare(&invalid));
    invalid.palette_entries = 4;
    invalid.lz77_bytes = 3;
    assert(!handheld_card_screen_prepare(&invalid));
    assert(lz77_calls == 0u);

    handheld_card_screen_commit();
    assert(test_mode == 0u);
    assert(handheld_card_screen_prepare(&card) == 1);
    assert(handheld_card_screen_prepare(&card) == 1);
    assert(lz77_calls == 1u);
    handheld_card_screen_commit();

    assert(test_mode == (MODE_4 | BG2_ON));
    for (x = 0u; x < 4u; ++x)
        assert(test_bg_colors[x] == palette[x]);
    for (x = 4u; x < 256u; ++x)
        assert(test_bg_colors[x] == 0u);
    for (y = 0u; y < 12u; ++y)
        for (x = 0u; x < 240u; ++x)
            assert(test_vram[y * 240u + x] == 0u);
    for (y = 12u; y < 147u; ++y)
        for (x = 0u; x < 240u; ++x)
            assert(test_vram[y * 240u + x] == (u8)(1u + (x % 3u)));
    for (y = 147u; y < 160u; ++y)
        for (x = 0u; x < 240u; ++x)
            assert(test_vram[y * 240u + x] == 0u);

    /* Browsing different private cards can reuse the same mutable descriptor.
     * A pointer-only descriptor cache would silently show the previous card. */
    assert(handheld_card_screen_prepare(&reused) == 1);
    assert(lz77_calls == 1u);
    reused.lz77 = alternative_pixels;
    assert(handheld_card_screen_prepare(&reused) == 1);
    assert(lz77_calls == 2u);
    assert(handheld_card_screen_prepare(&reused) == 1);
    assert(lz77_calls == 2u);
    reused.lz77 = valid_pixels;
    reused.palette = alternative_palette;
    reused.palette_entries = 2u;
    assert(handheld_card_screen_prepare(&reused) == 1);
    assert(lz77_calls == 3u);
    handheld_card_screen_commit();
    assert(test_bg_colors[0] == 0x4567u);
    assert(test_bg_colors[1] == 0x001Fu);
    assert(test_bg_colors[2] == 0u);

    handheld_card_screen_restore();
    assert(test_mode == (MODE_0 | OBJ_ON | OBJ_1D_MAP));
    puts("Handheld card runtime: PASS (validation, payload-aware cache, palette, 12/13 letterbox and Mode 0 restore)");
    return 0;
}
