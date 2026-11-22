#ifndef VOLVY_CORE_GAME_TIMER_H
#define VOLVY_CORE_GAME_TIMER_H

#include <stdint.h>

/* Format a 60 Hz frame counter as M:SS:CC (centiseconds), or MM:SS:CC
 * after ten minutes. Returns the visible character count. */
uint8_t game_timer_format(uint32_t frames, char out[9]);

#endif
