/* Host diagnostic only. Emits the real C prototype's platform positions.
 * Build by linking bonus_platforms.c and converted bonus_w1_room.c.
 */
#include <stdio.h>
#include <string.h>
#include "core/bonus_platforms.h"

int main(void)
{
    BonusMovingPlatform platforms[BONUS_MOVING_PLATFORM_COUNT];
    Player player;
    unsigned int frame, i;
    memset(&player, 0, sizeof(player));
    player.vy = -FX_ONE;  /* never attaches to a platform during this trace */
    bonus_platforms_init(platforms);
    puts("frame,platform,source_instance_index,x_pivot,y_pivot,width,height,direction");
    for (frame = 0; frame <= 192; ++frame) {
        if (frame == 0 || frame == 24 || frame == 48 ||
            frame == 72 || frame == 96 || frame == 144 || frame == 192) {
            for (i = 0; i < BONUS_MOVING_PLATFORM_COUNT; ++i) {
                const BonusMovingPlatform *p = &platforms[i];
                printf("%u,%u,%u,%.4f,%.4f,%d,%d,%d\n",
                       frame, i, p->source_instance_index,
                       (double)p->x / FX_ONE, (double)p->y / FX_ONE,
                       p->width, p->height, p->direction);
            }
        }
        bonus_platforms_update(platforms, &player, 0, 0, 0);
    }
    return 0;
}
