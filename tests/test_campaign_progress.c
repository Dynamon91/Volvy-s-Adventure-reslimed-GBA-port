#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "core/campaign_progress.h"

int main(void)
{
    CampaignProgress progress;
    RunStats run;
    const CampaignRecord *record;
    campaign_progress_reset(&progress);
    run_stats_reset(&run, 0, 0);
    run.onions_collected = 23u;
    run.enemies_defeated = 4u;
    assert(campaign_progress_get(&progress, 0) == 0);
    assert(campaign_progress_get(&progress, 6) == 0);
    assert(campaign_progress_get(&progress, 1)->has_clear == 0);
    campaign_progress_record_clear(&progress, 0, 800u, &run);
    campaign_progress_record_clear(&progress, 6, 800u, &run);
    assert(campaign_progress_get(&progress, 1)->clear_count == 0u);

    campaign_progress_record_clear(&progress, 1, 800u, &run);
    run.onions_collected = 8u;
    run.enemies_defeated = 1u;
    campaign_progress_record_clear(&progress, 1, 920u, &run);
    campaign_progress_record_clear(&progress, 1, 450u, &run);
    record = campaign_progress_get(&progress, 1);
    assert(record->clear_count == 3u && record->best_clear_frames == 450u);
    assert(record->total_onions_collected == 39u);
    assert(record->total_enemies_defeated == 6u);
    assert(campaign_progress_get(&progress, 2)->clear_count == 0u);
    campaign_progress_record_clear(&progress, 4, 365u, &run);
    campaign_progress_record_clear(&progress, 5, 420u, &run);
    assert(campaign_progress_get(&progress, 4)->best_clear_frames == 365u);
    assert(campaign_progress_get(&progress, 5)->best_clear_frames == 420u);

    /* Complete-game milestones and counters cannot silently wrap. */
    progress.scenes[4].clear_count = UINT32_MAX;
    progress.scenes[4].total_onions_collected = UINT32_MAX - 3u;
    run.onions_collected = 8u;
    campaign_progress_record_clear(&progress, 5, 500u, &run);
    assert(progress.scenes[4].clear_count == UINT32_MAX);
    assert(progress.scenes[4].total_onions_collected == UINT32_MAX);
    assert(progress.scenes[4].best_clear_frames == 420u);

    campaign_progress_record_clear(&progress, 5, 0, 0);
    campaign_progress_record_clear(0, 5, 0, &run);
    campaign_progress_reset(0);
    campaign_progress_reset(&progress);
    assert(campaign_progress_get(&progress, 1)->has_clear == 0u);
    puts("Campaign records: PASS (five scenes, best runs, accumulated pickups, reset, overflow)");
    return 0;
}
