#include "core/static_frame_scheduler.h"

void static_frame_scheduler_begin(StaticFrameScheduler *scheduler,
                                  uint16_t vblank_budget_bytes)
{
    unsigned int i;
    scheduler->visited = 0u;
    scheduler->remaining_bytes = vblank_budget_bytes;
    for (i = 0; i < STATIC_FRAME_SCHEDULER_MAX_ANIMS; ++i)
        scheduler->chosen[i] = 0u;
}

unsigned int static_frame_scheduler_choose(StaticFrameScheduler *scheduler,
                                           unsigned int animation_id,
                                           unsigned int desired_frame,
                                           unsigned int uploaded_frame,
                                           unsigned int required_upload_bytes)
{
    uint32_t mask;
    if (animation_id >= STATIC_FRAME_SCHEDULER_MAX_ANIMS)
        return uploaded_frame;

    mask = (uint32_t)1u << animation_id;
    if (scheduler->visited & mask)
        return scheduler->chosen[animation_id];

    scheduler->visited |= mask;
    scheduler->chosen[animation_id] = (uint16_t)uploaded_frame;
    if (desired_frame != uploaded_frame &&
        required_upload_bytes <= scheduler->remaining_bytes) {
        scheduler->chosen[animation_id] = (uint16_t)desired_frame;
        scheduler->remaining_bytes = (uint16_t)
            (scheduler->remaining_bytes - required_upload_bytes);
    }
    return scheduler->chosen[animation_id];
}
