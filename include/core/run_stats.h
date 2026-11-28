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
    uint16_t last_onions;
    uint8_t last_enemies;
} RunStats;

void run_stats_reset(RunStats *stats, uint16_t onions, uint8_t enemies);
void run_stats_observe(RunStats *stats, uint16_t onions, uint8_t enemies);
#endif
