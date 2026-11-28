#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "core/run_stats.h"

int main(void)
{
    RunStats s;
    run_stats_reset(&s, 0, 0);
    assert(s.active_frames == 0 && s.onions_collected == 0);
    run_stats_observe(&s, 2, 1);
    run_stats_observe(&s, 7, 3);
    assert(s.active_frames == 2 && s.onions_collected == 7
        && s.enemies_defeated == 3);
    /* Onion consumption cannot create false pickups. */
    run_stats_observe(&s, 1, 3);
    assert(s.onions_collected == 7);
    run_stats_observe(&s, 4, 4);
    assert(s.onions_collected == 10 && s.enemies_defeated == 4);
    /* A reset/wrapped enemy counter cannot produce phantom defeats. */
    run_stats_observe(&s, 4, 0);
    assert(s.enemies_defeated == 4);
    run_stats_observe(&s, 4, 2);
    assert(s.enemies_defeated == 6);
    /* Starting the next course or retrying must reset the run. */
    run_stats_reset(&s, 4, 2);
    assert(s.active_frames == 0 && s.onions_collected == 0
        && s.enemies_defeated == 0);
    run_stats_observe(&s, 5, 2);
    assert(s.onions_collected == 1 && s.active_frames == 1);
    run_stats_observe(0, 0, 0);
    run_stats_reset(0, 0, 0);
    puts("RunStats: PASS (run totals, resets, no fabricated combo)");
    return 0;
}
