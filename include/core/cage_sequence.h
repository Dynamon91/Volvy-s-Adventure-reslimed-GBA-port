#ifndef VOLVY_CORE_CAGE_SEQUENCE_H
#define VOLVY_CORE_CAGE_SEQUENCE_H
#include <stdint.h>
#include "core/fixed.h"

/* Raw source seq_cage_drop position.y curve, in source pixels (20.12). */
fx32 shrewy_cage_drop_offset(uint16_t frame);

typedef struct ShrewyCageSequencePlayer {
    uint8_t frame;
    uint8_t active;
    uint8_t finished;
    uint8_t reserved;
} ShrewyCageSequencePlayer;

/* Call once per 60 Hz update after the original trigger has been established. */
void shrewy_cage_sequence_start(ShrewyCageSequencePlayer *player);
fx32 shrewy_cage_sequence_current_offset(const ShrewyCageSequencePlayer *player);
fx32 shrewy_cage_sequence_update(ShrewyCageSequencePlayer *player);
int shrewy_cage_sequence_is_active(const ShrewyCageSequencePlayer *player);
#endif
