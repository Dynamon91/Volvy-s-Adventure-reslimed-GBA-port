#ifndef VOLVY_CORE_CAMPAIGN_PROGRESS_H
#define VOLVY_CORE_CAMPAIGN_PROGRESS_H
#include <stdint.h>
#include "core/run_stats.h"

/* These IDs cover ONLY the five implemented prototype scenes, not the
 * original game's complete world map or its final unlock logic. */
#define VOLVY_TRACKED_SCENES 5u
#define VOLVY_FIRST_SCENE_ID 1u

typedef struct CampaignRecord {
    uint32_t clear_count;
    uint32_t best_clear_frames;
    uint32_t total_onions_collected;
    uint32_t total_enemies_defeated;
    uint8_t has_clear;
} CampaignRecord;

typedef struct CampaignProgress {
    CampaignRecord scenes[VOLVY_TRACKED_SCENES];
} CampaignProgress;

/* RAM bookkeeping for completed reconstructed scenes only.
 * No unlock rules, final rankings, save persistence or source-authentic
 * scoring are inferred by this module. */
void campaign_progress_reset(CampaignProgress *progress);
void campaign_progress_record_clear(CampaignProgress *progress,
                                    uint8_t scene_id,
                                    uint32_t elapsed_frames,
                                    const RunStats *run);
const CampaignRecord *campaign_progress_get(const CampaignProgress *progress,
                                            uint8_t scene_id);
#endif
