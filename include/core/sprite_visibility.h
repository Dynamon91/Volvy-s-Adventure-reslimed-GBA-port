#ifndef VOLVY_CORE_SPRITE_VISIBILITY_H
#define VOLVY_CORE_SPRITE_VISIBILITY_H

/* Hardware viewport bounds. Sprites that do not intersect this rectangle
 * must not reserve OBJ frame upload bandwidth: even an off-screen animation
 * shares its tile slot and the global VBlank upload budget with visible ones. */
#define VOLVY_SCREEN_WIDTH 240
#define VOLVY_SCREEN_HEIGHT 160

static inline int sprite_part_intersects_viewport(int x, int y, int w, int h)
{
    return w > 0 && h > 0 && x < VOLVY_SCREEN_WIDTH &&
           y < VOLVY_SCREEN_HEIGHT && x + w > 0 && y + h > 0;
}

#endif
