/* Diagnostic program: compile on the host to inspect two source-anchored
 * prototype bunny trajectories. Not a GameMaker source-behavior trace. */
#include <stdio.h>
#include "core/bonus_bunnies.h"
#include "core/level.h"

int main(void)
{
    Level level;
    BonusBunny bunnies[BONUS_ACTIVE_BUNNY_COUNT];
    unsigned int frame, i;
    level_init_bonus_race(&level);
    bonus_bunnies_init(bunnies);
    puts("frame,opponent,source_instance,x_gba,y_gba,facing,grounded,hop_count");
    for (frame = 0; frame <= 240; ++frame) {
        if (frame == 0 || frame == 80 || frame == 160 || frame == 240) {
            for (i = 0; i < BONUS_ACTIVE_BUNNY_COUNT; ++i) {
                const BonusBunny *b = &bunnies[i];
                printf("%u,%u,%u,%d,%d,%d,%u,%u\n", frame, i,
                       b->source_instance_index,
                       fx_to_int(b->x), fx_to_int(b->y),
                       b->facing, b->grounded, b->hop_count);
            }
        }
        bonus_bunnies_update(bunnies, &level);
    }
    return 0;
}
