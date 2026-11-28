#include "core/run_stats.h"

/* These totals follow the player's monotonic pickup and defeat event
 * counters, not the 0..999 visible onion inventory. Consuming onions for
 * Swolvy or reaching the HUD counter cap must never lose pickup history.
 * Decreases are ignored; game.c resets counters on stage entry/retry. */
void run_stats_reset(RunStats *stats, uint32_t collected_total,
                     uint32_t defeated_total)
{
    if (!stats) return;
    stats->active_frames = 0;
    stats->onions_collected = 0;
    stats->enemies_defeated = 0;
    stats->last_collected_total = collected_total;
    stats->last_defeated_total = defeated_total;
}

static uint32_t sat_add(uint32_t a, uint32_t b)
{
    return UINT32_MAX - a < b ? UINT32_MAX : a + b;
}

void run_stats_observe(RunStats *stats, uint32_t collected_total,
                       uint32_t defeated_total)
{
    if (!stats) return;
    if (stats->active_frames != UINT32_MAX)
        ++stats->active_frames;
    if (collected_total > stats->last_collected_total) {
        stats->onions_collected = sat_add(
            stats->onions_collected, collected_total - stats->last_collected_total);
        stats->last_collected_total = collected_total;
    }
    if (defeated_total > stats->last_defeated_total) {
        stats->enemies_defeated = sat_add(
            stats->enemies_defeated, defeated_total - stats->last_defeated_total);
        stats->last_defeated_total = defeated_total;
    }
    /* If either counter moves backwards unexpectedly, hold the previous
     * watermark so subsequent observations cannot create phantom events. */
}
