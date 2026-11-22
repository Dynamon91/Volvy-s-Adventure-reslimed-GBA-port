#include "core/title_menu.h"

uint8_t title_menu_move(uint8_t current, int direction)
{
    if (current >= TITLE_MENU_ITEM_COUNT)
        current = TITLE_MENU_START_GAME;

    if (direction < 0 && current > TITLE_MENU_START_GAME)
        --current;
    else if (direction > 0 && current < TITLE_MENU_QUIT)
        ++current;

    return current;
}

int title_menu_can_start(uint8_t selection)
{
    return selection == TITLE_MENU_START_GAME;
}
