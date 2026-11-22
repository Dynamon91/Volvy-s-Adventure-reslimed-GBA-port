#include <gba.h>

#include "core/game.h"
#include "core/input.h"
#include "platform/gba_display.h"

int main(void)
{
    irqInit();
    irqEnable(IRQ_VBLANK);
    REG_IME = 1;

    gba_display_init();
    input_init();
    game_init();

    for (;;) {
        input_update();
        game_update();
        game_render();

        /* All VRAM/OAM presentation updates are committed during VBlank. */
        VBlankIntrWait();
        game_vblank_commit();
    }

    return 0;
}
