#ifndef VOLVY_CORE_TITLE_MENU_H
#define VOLVY_CORE_TITLE_MENU_H

#include <stdint.h>

typedef enum TitleMenuItem {
    TITLE_MENU_START_GAME = 0,
    TITLE_MENU_SETTINGS = 1,
    TITLE_MENU_QUIT = 2,
    TITLE_MENU_ITEM_COUNT = 3
} TitleMenuItem;

/* Clamp one-step directional navigation to the three observed menu entries.
 * Direction: negative = up, positive = down, zero = unchanged. */
uint8_t title_menu_move(uint8_t current, int direction);

/* Only Start Game is connected to a reconstructed game mode at this stage.
 * Settings/Quit are visible in the walkthrough, but their GBA actions are
 * not yet verified, so do not silently invent behavior for them. */
int title_menu_can_start(uint8_t selection);

#endif
