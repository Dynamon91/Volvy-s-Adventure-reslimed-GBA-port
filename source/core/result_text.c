#include "core/result_text.h"

void result_text_uint32(uint32_t value, char out[11])
{
    char reverse[10];
    unsigned int i = 0, j;
    if (!out) return;
    do {
        reverse[i++] = (char)('0' + (value % 10u));
        value /= 10u;
    } while (value && i < 10u);
    for (j = 0; j < i; ++j)
        out[j] = reverse[i - 1u - j];
    out[i] = '\0';
}

const char *result_text_heading(uint8_t scene_id)
{
    /* Shrewy DEFEATED is directly confirmed by the walkthrough. */
    return scene_id == 4u ? "SHREWY DEFEATED" : "STAGE CLEAR";
}
