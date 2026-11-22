#ifndef VOLVY_CORE_STATIC_FRAME_SCHEDULER_H
#define VOLVY_CORE_STATIC_FRAME_SCHEDULER_H

#include <stdint.h>

/* One 8bpp OBJ graphics slot is shared by all instances of an animation.
 * Select the OAM frame and reserve its VBlank upload together, so the
 * tile data on screen matches the sprite's origin/part layout that frame. */
#define STATIC_FRAME_SCHEDULER_MAX_ANIMS 32u

typedef struct StaticFrameScheduler {
    uint32_t visited;
    uint16_t chosen[STATIC_FRAME_SCHEDULER_MAX_ANIMS];
    uint16_t remaining_bytes;
} StaticFrameScheduler;

void static_frame_scheduler_begin(StaticFrameScheduler *scheduler,
                                  uint16_t vblank_budget_bytes);

/* An animation ID is scheduled at most once per draw. If its new frame won't
 * fit the remaining DMA budget, keep drawing the last uploaded frame. */
unsigned int static_frame_scheduler_choose(StaticFrameScheduler *scheduler,
                                           unsigned int animation_id,
                                           unsigned int desired_frame,
                                           unsigned int uploaded_frame,
                                           unsigned int required_upload_bytes);

#endif
