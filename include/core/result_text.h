#ifndef VOLVY_CORE_RESULT_TEXT_H
#define VOLVY_CORE_RESULT_TEXT_H
#include <stdint.h>

/* Minimal, bounded font-safe text formatting for measured run totals.
 * The source-game's combo/rank algorithm is NOT reproduced here. */
void result_text_uint32(uint32_t value, char out[11]);
const char *result_text_heading(uint8_t scene_id);
#endif
