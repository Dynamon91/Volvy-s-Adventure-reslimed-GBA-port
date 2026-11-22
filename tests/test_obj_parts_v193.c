#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "platform/obj_parts.h"

#define OAM_CAPACITY 128
#define SENTINEL 0x5A5Au

static GbaSpritePart parts[3] = {
    {0, 64, 0, 0, 16, 16, 0, 1},
    {0, 64, 20, 0, 16, 16, 0, 1},
    {0, 64, 40, 0, 16, 16, 0, 1},
};
static GbaSpriteFrame frame = {parts, 3, 60, 16, 0, 0};
static OBJATTR oam[OAM_CAPACITY + 1];

static void reset_oam(void)
{
    unsigned int i;
    for (i = 0; i < OAM_CAPACITY + 1; ++i) {
        oam[i].attr0 = SENTINEL;
        oam[i].attr1 = SENTINEL;
        oam[i].attr2 = SENTINEL;
    }
}

int main(void)
{
    int used;
    reset_oam();
    /* A boss/projectile far from the camera must not wrap modulo 512. */
    used = volvy_obj_draw_frame_parts(&frame, oam, 0, 512, 48, 0, 0, 1);
    assert(used == 0 && oam[0].attr0 == SENTINEL);
    used = volvy_obj_draw_frame_parts(&frame, oam, 0, -512, 48, 0, 0, 1);
    assert(used == 0 && oam[0].attr0 == SENTINEL);
    used = volvy_obj_draw_frame_parts(&frame, oam, 0, 48, 256, 0, 0, 1);
    assert(used == 0 && oam[0].attr0 == SENTINEL);

    /* First part is invisible, next two are visible, OAM is packed. The
     * 8bpp tile indices must not move when preceding parts are skipped. */
    used = volvy_obj_draw_frame_parts(&frame, oam, 0, -25, 6, 0, 0, 1);
    assert(used == 2);
    assert((oam[0].attr1 & 511u) == (unsigned int)(-5 & 511));
    assert((oam[1].attr1 & 511u) == 15u);
    assert(oam[0].attr2 == (4u | (1u << 10)));
    assert(oam[1].attr2 == (8u | (1u << 10)));
    assert(oam[2].attr0 == SENTINEL);

    /* Flipping retains mirrored positions and each part's own VRAM tiles. */
    reset_oam();
    used = volvy_obj_draw_frame_parts(&frame, oam, 0, -25, 6, 1, 128, 1);
    assert(used == 2);
    assert((oam[0].attr1 & 511u) == 19u);
    assert((oam[1].attr1 & 511u) == 511u); /* -1 wrapped for HW register */
    assert((oam[0].attr1 & (1u << 12)) != 0);
    assert(oam[0].attr2 == ((128u * 2u) | (1u << 10)));
    assert(oam[1].attr2 == (((128u + 2u) * 2u) | (1u << 10)));
    assert(oam[2].attr0 == SENTINEL);

    /* Exactly touching the viewport edge is invisible. */
    reset_oam();
    parts[0].offset_x = 0;
    parts[1].offset_x = 0;
    parts[2].offset_x = 0;
    used = volvy_obj_draw_frame_parts(&frame, oam, 0, 240, 6, 0, 0, 1);
    assert(used == 0);
    used = volvy_obj_draw_frame_parts(&frame, oam, 0, 239, 6, 0, 0, 1);
    assert(used == 3);

    /* Never write beyond 128 hardware OAM entries, even for multi-parts. */
    reset_oam();
    used = volvy_obj_draw_frame_parts(&frame, oam, 127, 30, 6, 0, 0, 1);
    assert(used == 128);
    assert(oam[128].attr0 == SENTINEL);
    puts("GBA OBJ part culling/packed OAM/8bpp offsets/H-flip/cap: PASS");
    return 0;
}
