#include <assert.h>
#include <stdio.h>
#include "core/static_frame_scheduler.h"

static void one_animation_shared_by_many_actors(void)
{
    StaticFrameScheduler s;
    static_frame_scheduler_begin(&s, 6144);
    assert(static_frame_scheduler_choose(&s, 3, 2, 1, 1024) == 2);
    assert(s.remaining_bytes == 5120);
    /* Two objects on screen reuse the chosen frame; don't spend the budget
       a second time or silently change their shared tile allocation. */
    assert(static_frame_scheduler_choose(&s, 3, 3, 1, 2048) == 2);
    assert(s.remaining_bytes == 5120);
    assert(static_frame_scheduler_choose(&s, 4, 1, 0, 5120) == 1);
    assert(s.remaining_bytes == 0);
    assert(static_frame_scheduler_choose(&s, 5, 6, 4, 64) == 4);
}

static void no_partial_frame_upload_and_no_drift(void)
{
    StaticFrameScheduler s;
    static_frame_scheduler_begin(&s, 512);
    assert(static_frame_scheduler_choose(&s, 0, 5, 4, 513) == 4);
    assert(s.remaining_bytes == 512);
    assert(static_frame_scheduler_choose(&s, 1, 5, 4, 512) == 5);
    assert(s.remaining_bytes == 0);
    assert(static_frame_scheduler_choose(&s, 2, 1, 1, 1024) == 1);
    static_frame_scheduler_begin(&s, 512);
    /* Next VBlank can service a previously deferred animation. */
    assert(static_frame_scheduler_choose(&s, 0, 5, 4, 512) == 5);
    assert(static_frame_scheduler_choose(&s, 32, 1, 7, 32) == 7);
}

int main(void)
{
    one_animation_shared_by_many_actors();
    no_partial_frame_upload_and_no_drift();
    puts("Static OBJ frame/VBlank scheduling: PASS");
    return 0;
}
