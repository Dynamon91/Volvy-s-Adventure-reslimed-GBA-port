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
    /* The HUD may spend onions or stop increasing at 999. Source event
     * totals remain monotonic and preserve actual pickups and defeats. */
    run_stats_observe(&s, 7, 3);
    run_stats_observe(&s, 10, 4);
    assert(s.onions_collected == 10 && s.enemies_defeated == 4);
    assert(s.active_frames == 4);

    /* Per-level reset can use nonzero cumulative source counter baselines. */
    run_stats_reset(&s, 1000, 260);
    assert(s.active_frames == 0 && s.onions_collected == 0);
    run_stats_observe(&s, 1005, 262);
    assert(s.onions_collected == 5 && s.enemies_defeated == 2);
    run_stats_observe(&s, 1005, 262);
    assert(s.onions_collected == 5 && s.enemies_defeated == 2);
    /* Unexpected backwards telemetry must not create phantom gains once
     * the older watermark is reached again. */
    run_stats_observe(&s, 999, 200);
    run_stats_observe(&s, 1005, 262);
    assert(s.onions_collected == 5 && s.enemies_defeated == 2);
    /* The player event counters cannot wrap: stats must saturate too. */
    s.onions_collected = UINT32_MAX - 1u;
    run_stats_observe(&s, 1010, 264);
    assert(s.onions_collected == UINT32_MAX);
    assert(s.enemies_defeated == 4);
    run_stats_reset(0, 0, 0);
    run_stats_observe(0, 0, 0);
    puts("RunStats: PASS (monotonic pickup/defeat events, nonzero reset and saturation)");
    return 0;
}
