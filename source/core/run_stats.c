#include "core/run_stats.h"

/* Track positive changes instead of the on-screen onion counter itself:
 * the counter can decrease when onions are spent on transformations.
 * The 8-bit enemy count may also reset/wrap; a decrease is not treated as
 * a new streak. This is a conservative lower bound, not an original score. */
void run_stats_reset(RunStats *stats, uint16_t onions, uint8_t enemies)
{
    if (!stats) return;
    stats->active_frames = 0;
    stats->onions_collected = 0;
    stats->enemies_defeated = 0;
    stats->last_onions = onions;
    stats->last_enemies = enemies;
}

void run_stats_observe(RunStats *stats, uint16_t onions, uint8_t enemies)
{
    unsigned int delta;
    if (!stats) return;
    if (stats->active_frames != UINT32_MAX)
        ++stats->active_frames;
    if (onions > stats->last_onions) {
        delta = (unsigned int)(onions - stats->last_onions);
        stats->onions_collected = UINT32_MAX - stats->onions_collected < delta
            ? UINT32_MAX : stats->onions_collected + delta;
    }
    if (enemies > stats->last_enemies) {
        delta = (unsigned int)(enemies - stats->last_enemies);
        stats->enemies_defeated = UINT32_MAX - stats->enemies_defeated < delta
            ? UINT32_MAX : stats->enemies_defeated + delta;
    }
    stats->last_onions = onions;
    stats->last_enemies = enemies;
}
