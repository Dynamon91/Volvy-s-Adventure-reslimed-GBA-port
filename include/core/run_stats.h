#ifndef VOLVY_CORE_RUN_STATS_H
#define VOLVY_CORE_RUN_STATS_H
#include <stdint.h>

/* Per-run facts for eventual source-faithful results screens.
 * DO NOT derive combo/rank scores from these totals: the source game has
 * separate combo rules which are not yet reconstructed. */
typedef struct RunStats {
    uint32_t active_frames;
    uint32_t onions_collected;
    uint32_t enemies_defeated;
    uint32_t last_collected_total;
    uint32_t last_defeated_total;
} RunStats;

/* Inputs are monotonic event totals, NOT the current HUD onion inventory. */
void run_stats_reset(RunStats *stats, uint32_t collected_total,
                     uint32_t defeated_total);
void run_stats_observe(RunStats *stats, uint32_t collected_total,
                       uint32_t defeated_total);
#endif
