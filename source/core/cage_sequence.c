#include "core/cage_sequence.h"
#include "shrewy_cage_sequence.h"

fx32 shrewy_cage_drop_offset(uint16_t frame)
{
    if (frame >= SHREWY_CAGE_SEQUENCE_FRAMES)
        frame = SHREWY_CAGE_SEQUENCE_FRAMES - 1;
    return shrewy_cage_drop_y_q12[frame];
}

void shrewy_cage_sequence_start(ShrewyCageSequencePlayer *player)
{
    if (!player) return;
    player->frame = 0;
    player->active = 1;
    player->finished = 0;
    player->reserved = 0;
}

fx32 shrewy_cage_sequence_current_offset(const ShrewyCageSequencePlayer *player)
{
    if (!player) return 0;
    return shrewy_cage_drop_offset(player->frame);
}

fx32 shrewy_cage_sequence_update(ShrewyCageSequencePlayer *player)
{
    fx32 offset;
    if (!player) return 0;
    offset = shrewy_cage_sequence_current_offset(player);
    if (!player->active) return offset;
    if ((unsigned int)player->frame + 1u >= SHREWY_CAGE_SEQUENCE_FRAMES) {
        player->active = 0;
        player->finished = 1;
    } else {
        ++player->frame;
    }
    return offset;
}

int shrewy_cage_sequence_is_active(const ShrewyCageSequencePlayer *player)
{
    return player && player->active;
}
