#include <gba.h>
#include <stdint.h>
#include "platform/handheld_card_screen.h"
#include "generated_cards.h"

/* STANDALONE reference-card browser: not the full game or original hub.
 * Shows private walkthrough-derived 8bpp screens in chronological order.
 * R/RIGHT = next, L/LEFT = previous, START = first/title screen. */
int main(void)
{
    unsigned int selected = 0u;
    uint16_t previous = 0u;
    REG_IME = 0u;
    SetMode(MODE_0);

    if (handheld_card_screen_prepare(&g_viewer_cards[selected])) {
        VBlankIntrWait();
        handheld_card_screen_commit();
    }

    for (;;) {
        uint16_t keys, pressed;
        unsigned int next = selected;
        VBlankIntrWait();
        keys = (uint16_t)((~REG_KEYINPUT) & 0x03FFu);
        pressed = (uint16_t)(keys & (uint16_t)(~previous));
        previous = keys;

        if (pressed & (KEY_R | KEY_RIGHT)) {
            next = selected + 1u;
            if (next >= G_VIEWER_CARD_COUNT) next = 0u;
        } else if (pressed & (KEY_L | KEY_LEFT)) {
            next = selected > 0u ? selected - 1u : G_VIEWER_CARD_COUNT - 1u;
        } else if (pressed & KEY_START) {
            next = 0u;
        }

        if (next != selected) {
            /* Decompression may span VBlank. Never write graphics into
             * VRAM until the next VBlank begins. */
            if (handheld_card_screen_prepare(&g_viewer_cards[next])) {
                VBlankIntrWait();
                handheld_card_screen_commit();
                selected = next;
            }
        }
    }
}
