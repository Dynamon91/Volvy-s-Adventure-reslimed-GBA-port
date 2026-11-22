#ifndef VOLVY_PLATFORM_HANDHELD_CARD_SCREEN_H
#define VOLVY_PLATFORM_HANDHELD_CARD_SCREEN_H
#include <stdint.h>

typedef struct HandheldCardAsset {
    const uint8_t *lz77;
    uint32_t lz77_bytes;
    const uint16_t *palette;
    uint16_t palette_entries;
} HandheldCardAsset;

/* Prepare a 240x135 source-derived 8bpp card in aligned EWRAM. */
int handheld_card_screen_prepare(const HandheldCardAsset *asset);
/* Commit image and palette during VBlank only. */
void handheld_card_screen_commit(void);
/* Return to Mode 0 (caller must restore clobbered tiled BG content). */
void handheld_card_screen_restore(void);
#endif
