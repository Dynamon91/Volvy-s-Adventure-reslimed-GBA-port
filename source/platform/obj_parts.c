#include "platform/obj_parts.h"
#include "core/sprite_visibility.h"

#define OBJ_8BPP_BIT (1u << 13)
#define OBJ_HFLIP_BIT (1u << 12)
#define OBJ_PRIORITY_SHIFT 10u
#define MAX_PLAYER_PARTS 8
#define VOLVY_OAM_COUNT 128

/* Player, Shrewy and rock sprites can legitimately be far from the camera
 * (notably during room transitions). Without clipping, the GBA's 9-bit X /
 * 8-bit Y OBJ coordinates wrap back into view as phantom sprites. Keep each
 * part's original tile offset when culling so the streamed 8bpp frame and
 * attr2 layout remain synchronized. Return the *actual* occupied OAM count. */
int volvy_obj_draw_frame_parts(const GbaSpriteFrame *frame, OBJATTR *oam,
                            int first_oam, int base_x, int base_y,
                            int facing_left, int base_tile64, unsigned int priority)
{
    int tile_index = 0;
    int next_oam = first_oam;
    int i;
    int count = frame->part_count;
    if (count > MAX_PLAYER_PARTS) count = MAX_PLAYER_PARTS;
    for (i = 0; i < count && next_oam < VOLVY_OAM_COUNT; ++i) {
        const GbaSpritePart *part = &frame->parts[i];
        int x = base_x + part->offset_x;
        int y = base_y + part->offset_y;
        if (tile_index & 1) ++tile_index;
        if (facing_left)
            x = base_x + frame->width - part->offset_x - part->hw_w;
        if (sprite_part_intersects_viewport(x, y, part->hw_w, part->hw_h)) {
            oam[next_oam].attr0 = (u16)((y & 0x00FF) | OBJ_8BPP_BIT |
                                         ((u16)part->shape << 14));
            oam[next_oam].attr1 = (u16)((x & 0x01FF) |
                                         (facing_left ? OBJ_HFLIP_BIT : 0) |
                                         ((u16)part->size_code << 14));
            /* OBJ attr2 addresses 32-byte units; 8bpp payloads are 64B. */
            oam[next_oam].attr2 = (u16)(((base_tile64 + tile_index) * 2) |
                                             ((priority & 3u) << OBJ_PRIORITY_SHIFT));
            oam[next_oam].dummy = 0;
            ++next_oam;
        }
        tile_index += part->tile_bytes / 64;
    }
    return next_oam;
}
