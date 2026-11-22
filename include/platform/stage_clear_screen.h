#ifndef VOLVY_PLATFORM_STAGE_CLEAR_SCREEN_H
#define VOLVY_PLATFORM_STAGE_CLEAR_SCREEN_H

/* Prepare the walkthrough-derived first-stage result frame in EWRAM. */
int stage_clear_screen_prepare(void);

/* Commit the prepared 240x160 bitmap during VBlank. */
void stage_clear_screen_commit(void);

/* Restore the normal tiled Mode 0 renderer after leaving the result screen. */
void stage_clear_screen_restore(void);

#endif
