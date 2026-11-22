#ifndef VOLVY_PLATFORM_GBA_DISPLAY_H
#define VOLVY_PLATFORM_GBA_DISPLAY_H

#include <gba.h>

#define VOLVY_SCREEN_WIDTH  240
#define VOLVY_SCREEN_HEIGHT 160
#define VOLVY_OAM_COUNT     128

void gba_display_init(void);
void gba_display_set_camera(int x, int y);
void gba_display_commit_oam(void);
OBJATTR *gba_display_oam_shadow(void);

#endif
