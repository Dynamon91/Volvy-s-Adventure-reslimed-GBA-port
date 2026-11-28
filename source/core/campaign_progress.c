#include "core/campaign_progress.h"
#include <string.h>

static uint32_t saturating_add_u32(uint32_t a, uint32_t b)
{
    return UINT32_MAX - a < b ? UINT32_MAX : a + b;
}

void campaign_progress_reset(CampaignProgress *progress)
{
    if (!progress) return;
    memset(progress, 0, sizeof(*progress));
}

const CampaignRecord *campaign_progress_get(const CampaignProgress *progress,
                                            uint8_t scene_id)
{
    if (!progress || scene_id < VOLVY_FIRST_SCENE_ID ||
        scene_id >= VOLVY_FIRST_SCENE_ID + VOLVY_TRACKED_SCENES)
        return 0;
    return &progress->scenes[scene_id - VOLVY_FIRST_SCENE_ID];
}

void campaign_progress_record_clear(CampaignProgress *progress,
                                    uint8_t scene_id,
                                    uint32_t elapsed_frames,
                                    const RunStats *run)
{
    CampaignRecord *record;
    if (!progress || !run || !campaign_progress_get(progress, scene_id))
        return;

    record = &progress->scenes[scene_id - VOLVY_FIRST_SCENE_ID];
    if (!record->has_clear || elapsed_frames < record->best_clear_frames) {
        record->best_clear_frames = elapsed_frames;
        record->has_clear = 1u;
    }
    record->clear_count = saturating_add_u32(record->clear_count, 1u);
    record->total_onions_collected =
        saturating_add_u32(record->total_onions_collected,
                           run->onions_collected);
    record->total_enemies_defeated =
        saturating_add_u32(record->total_enemies_defeated,
                           run->enemies_defeated);
}
