#include <string.h>
#include <gba.h>

#include "platform/gba_display.h"

static OBJATTR s_oam_shadow[VOLVY_OAM_COUNT] ALIGN(4);

void gba_display_init(void)
{
    unsigned int i;

    /* Keep display layers disabled until renderer uploads the first frame. */
    SetMode(MODE_0);

    /* GBA VRAM spans 96 KiB: 64 KiB BG + 32 KiB OBJ. */
    memset((void *)VRAM, 0, 0x18000);
    /* Palette RAM ignores/duplicates 8-bit stores on real GBA hardware.
     * Explicit 16-bit writes clear each color exactly. */
    for (i = 0; i < 256; ++i) {
        BG_COLORS[i] = 0;
        OBJ_COLORS[i] = 0;
    }

    for (i = 0; i < VOLVY_OAM_COUNT; ++i) {
        s_oam_shadow[i].attr0 = ATTR0_DISABLED;
        s_oam_shadow[i].attr1 = 0;
        s_oam_shadow[i].attr2 = 0;
        s_oam_shadow[i].dummy = 0;
    }

    /* The native 4bpp renderer configures BG1 as a 32x32 scene-window map
     * in screen block 27, with a 32 KiB tile cache at character blocks 1+2.
     * BG0 uses screen block 29 and character block 0 for original source
     * HUD glyphs; BG2 and BG3 are disabled in the current runtime. */
    BGCTRL[0] = BG_SIZE_0 | CHAR_BASE(0) | SCREEN_BASE(29) | 0;
    BGCTRL[1] = BG_SIZE_1 | CHAR_BASE(0) | SCREEN_BASE(27) | 1;
    BGCTRL[2] = BG_SIZE_0 | CHAR_BASE(0) | SCREEN_BASE(30) | 2;
    BGCTRL[3] = BG_SIZE_0 | CHAR_BASE(0) | SCREEN_BASE(31) | 3;

    BG_OFFSET[0].x = 0; BG_OFFSET[0].y = 0;
    BG_OFFSET[1].x = 0; BG_OFFSET[1].y = 0;
    BG_OFFSET[2].x = 0; BG_OFFSET[2].y = 0;
    BG_OFFSET[3].x = 0; BG_OFFSET[3].y = 0;
}

void gba_display_set_camera(int x, int y)
{
    BG_OFFSET[0].x = 0;
    BG_OFFSET[0].y = 0;

    BG_OFFSET[1].x = (u16)x;
    BG_OFFSET[1].y = (u16)y;

    BG_OFFSET[2].x = (u16)(x / 2);
    BG_OFFSET[2].y = (u16)(y / 2);

    BG_OFFSET[3].x = (u16)(x / 4);
    BG_OFFSET[3].y = (u16)(y / 4);
}

void gba_display_commit_oam(void)
{
    dmaCopy(s_oam_shadow, OAM, sizeof(s_oam_shadow));
}

OBJATTR *gba_display_oam_shadow(void)
{
    return s_oam_shadow;
}
