#include <gba.h>
#include <string.h>

#include "platform/stage_clear_screen.h"
#include "stage_clear_v198_bin.h"

#define STAGE_CLEAR_WIDTH 240u
#define STAGE_CLEAR_HEIGHT 160u
#define STAGE_CLEAR_CONTENT_HEIGHT 135u
#define STAGE_CLEAR_TOP_MARGIN 12u
#define STAGE_CLEAR_PACKED_BYTES 16200u
#define STAGE_CLEAR_FRAME_BYTES (STAGE_CLEAR_WIDTH * STAGE_CLEAR_HEIGHT)
#define STAGE_CLEAR_FRAMEBUFFER ((void *)0x06000000)

/* The 4bpp source pixels are expanded to Mode 4's 8bpp framebuffer. */
static u8 s_stage_clear_packed[STAGE_CLEAR_PACKED_BYTES] ALIGN(4);
static u8 s_stage_clear_frame[STAGE_CLEAR_FRAME_BYTES] ALIGN(4);
static u8 s_stage_clear_prepared;

/* 16-color BGR555 palette; index 0 is the purple letterbox color. */
static const u16 s_stage_clear_palette[16] = {
    0x488B, 0x2B1E, 0x22EC, 0x197A,
    0x3569, 0x3CAE, 0x488B, 0x488B,
    0x448B, 0x3CA9, 0x408C, 0x448B,
    0x448A, 0x448A, 0x408A, 0x1C45
};

int stage_clear_screen_prepare(void)
{
    u32 declared_size;
    unsigned int y, x;

    if (s_stage_clear_prepared)
        return 1;

    if (stage_clear_v198_bin[0] != 0x10u)
        return 0;

    declared_size = (u32)stage_clear_v198_bin[1] |
                    ((u32)stage_clear_v198_bin[2] << 8) |
                    ((u32)stage_clear_v198_bin[3] << 16);
    if (declared_size != STAGE_CLEAR_PACKED_BYTES)
        return 0;

    /* BIOS LZ77 expands the packed 4bpp stream into aligned EWRAM. */
    LZ77UnCompWram(stage_clear_v198_bin, s_stage_clear_packed);

    /* The source screenshot is 16:9; retain it without cropping on 3:2 GBA. */
    memset(s_stage_clear_frame, 0, sizeof(s_stage_clear_frame));
    for (y = 0; y < STAGE_CLEAR_CONTENT_HEIGHT; ++y) {
        for (x = 0; x < STAGE_CLEAR_WIDTH; ++x) {
            unsigned int pixel = y * STAGE_CLEAR_WIDTH + x;
            u8 packed = s_stage_clear_packed[pixel >> 1];
            u8 color = (pixel & 1u) ? (u8)(packed >> 4) : (u8)(packed & 0x0Fu);
            s_stage_clear_frame[(y + STAGE_CLEAR_TOP_MARGIN) * STAGE_CLEAR_WIDTH + x] = color;
        }
    }

    s_stage_clear_prepared = 1;
    return 1;
}

void stage_clear_screen_commit(void)
{
    if (!s_stage_clear_prepared)
        return;

    dmaCopy(s_stage_clear_palette, (void *)BG_COLORS,
            sizeof(s_stage_clear_palette));
    dmaCopy(s_stage_clear_frame, STAGE_CLEAR_FRAMEBUFFER,
            STAGE_CLEAR_FRAME_BYTES);
    SetMode(MODE_4 | BG2_ON);
}

void stage_clear_screen_restore(void)
{
    /* BG1 stays disabled until the native scene window is committed again. */
    SetMode(MODE_0 | BG0_ON | OBJ_ON | OBJ_1D_MAP);
}
