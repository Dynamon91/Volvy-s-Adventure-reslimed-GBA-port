#include <gba.h>
#include <string.h>
#include "platform/handheld_card_screen.h"

#define CARD_WIDTH 240u
#define CARD_HEIGHT 160u
#define CARD_CONTENT_HEIGHT 135u
#define CARD_TOP_MARGIN 12u
#define CARD_CONTENT_BYTES (CARD_WIDTH * CARD_CONTENT_HEIGHT)
#define CARD_FRAME_BYTES (CARD_WIDTH * CARD_HEIGHT)

#ifndef HANDHELD_CARD_FRAMEBUFFER
#define HANDHELD_CARD_FRAMEBUFFER ((void *)0x06000000)
#endif

/* Decompress directly into the middle of one Mode 4-sized EWRAM buffer.
 * There is no separate 32,400-byte scratch buffer. */
static u8 s_card_frame[CARD_FRAME_BYTES] ALIGN(4);
static u16 s_card_palette[256] ALIGN(4);
static const u8 *s_prepared_stream;
static const u16 *s_prepared_palette;
static u32 s_prepared_stream_bytes;
static u16 s_prepared_palette_entries;
static u8 s_prepared;

int handheld_card_screen_prepare(const HandheldCardAsset *asset)
{
    u32 expected;

    if (!asset || !asset->lz77 || !asset->palette ||
        asset->lz77_bytes < 4u || (asset->lz77_bytes & 3u) != 0u ||
        asset->palette_entries == 0u || asset->palette_entries > 256u)
        return 0;

    /* A caller may reuse one descriptor while browsing several cards.
     * Cache by asset data identity, not by the descriptor's address. */
    if (s_prepared && s_prepared_stream == asset->lz77 &&
        s_prepared_stream_bytes == asset->lz77_bytes &&
        s_prepared_palette == asset->palette &&
        s_prepared_palette_entries == asset->palette_entries)
        return 1;

    if (asset->lz77[0] != 0x10u)
        return 0;

    expected = (u32)asset->lz77[1] |
               ((u32)asset->lz77[2] << 8) |
               ((u32)asset->lz77[3] << 16);
    if (expected != CARD_CONTENT_BYTES)
        return 0;

    /* Generated, trusted source bytes. Offset 2880 is word-aligned and
     * BIOS LZ77 targets WRAM, never direct writes into VRAM. */
    memset(s_card_frame, 0, sizeof(s_card_frame));
    LZ77UnCompWram(asset->lz77, s_card_frame + CARD_WIDTH * CARD_TOP_MARGIN);
    memset(s_card_palette, 0, sizeof(s_card_palette));
    memcpy(s_card_palette, asset->palette,
           (unsigned int)asset->palette_entries * sizeof(u16));

    s_prepared_stream = asset->lz77;
    s_prepared_stream_bytes = asset->lz77_bytes;
    s_prepared_palette = asset->palette;
    s_prepared_palette_entries = asset->palette_entries;
    s_prepared = 1u;
    return 1;
}

void handheld_card_screen_commit(void)
{
    if (!s_prepared)
        return;

    dmaCopy(s_card_palette, (void *)BG_COLORS, sizeof(s_card_palette));
    dmaCopy(s_card_frame, HANDHELD_CARD_FRAMEBUFFER, CARD_FRAME_BYTES);
    SetMode(MODE_4 | BG2_ON);
}

void handheld_card_screen_restore(void)
{
    /* The caller is responsible for restoring damaged tiled BG data. */
    SetMode(MODE_0 | OBJ_ON | OBJ_1D_MAP);
}
