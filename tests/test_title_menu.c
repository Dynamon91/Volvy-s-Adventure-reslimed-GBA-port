#include <assert.h>
#include <stdio.h>
#include "core/title_menu.h"

int main(void)
{
    assert(title_menu_move(TITLE_MENU_START_GAME, -1) == TITLE_MENU_START_GAME);
    assert(title_menu_move(TITLE_MENU_START_GAME, 1) == TITLE_MENU_SETTINGS);
    assert(title_menu_move(TITLE_MENU_SETTINGS, -1) == TITLE_MENU_START_GAME);
    assert(title_menu_move(TITLE_MENU_SETTINGS, 1) == TITLE_MENU_QUIT);
    assert(title_menu_move(TITLE_MENU_QUIT, 1) == TITLE_MENU_QUIT);
    assert(title_menu_move(TITLE_MENU_QUIT, -1) == TITLE_MENU_SETTINGS);
    assert(title_menu_move(TITLE_MENU_SETTINGS, 0) == TITLE_MENU_SETTINGS);
    assert(title_menu_move(0xFFu, 1) == TITLE_MENU_SETTINGS);

    assert(title_menu_can_start(TITLE_MENU_START_GAME));
    assert(!title_menu_can_start(TITLE_MENU_SETTINGS));
    assert(!title_menu_can_start(TITLE_MENU_QUIT));
    assert(!title_menu_can_start(0xFFu));

    puts("Title menu navigation/dispatch contract: PASS");
    return 0;
}
