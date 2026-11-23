#ifndef VOLVY_PLATFORM_BITMAP_DMA_SYNC_H
#define VOLVY_PLATFORM_BITMAP_DMA_SYNC_H

#include <gba.h>

/* libgba dmaCopy starts DMA3 without waiting for the transfer to finish.
 * Wait on the DMA3 enable bit before starting another DMA3 copy, and before
 * clearing the LCD's forced-blank bit.
 * Use the hardware register address because the experimental shim names the
 * DMA3 control register differently from devkitPro/libgba.
 * The host test's gba.h supplies a deterministic test override.
 */
#ifndef VOLVY_BITMAP_WAIT_DMA3
static inline void volvy_bitmap_wait_dma3(void)
{
    while ((*(volatile u32 *)0x040000DCu & (1u << 31)) != 0u) {
        /* This waits only for an active DMA3 copy, not for another frame. */
    }
}
#define VOLVY_BITMAP_WAIT_DMA3() volvy_bitmap_wait_dma3()
#endif

#endif
