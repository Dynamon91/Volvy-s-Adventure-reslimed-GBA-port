#include "core/game_timer.h"

uint8_t game_timer_format(uint32_t frames, char out[9])
{
    uint32_t total_seconds = frames / 60u;
    uint32_t minutes = total_seconds / 60u;
    uint32_t seconds = total_seconds % 60u;
    uint32_t centiseconds = ((frames % 60u) * 100u) / 60u;

    if (minutes > 99u) minutes = 99u;

    if (minutes < 10u) {
        out[0] = (char)('0' + minutes);
        out[1] = ':';
        out[2] = (char)('0' + seconds / 10u);
        out[3] = (char)('0' + seconds % 10u);
        out[4] = ':';
        out[5] = (char)('0' + centiseconds / 10u);
        out[6] = (char)('0' + centiseconds % 10u);
        out[7] = '\0';
        return 7;
    }

    out[0] = (char)('0' + minutes / 10u);
    out[1] = (char)('0' + minutes % 10u);
    out[2] = ':';
    out[3] = (char)('0' + seconds / 10u);
    out[4] = (char)('0' + seconds % 10u);
    out[5] = ':';
    out[6] = (char)('0' + centiseconds / 10u);
    out[7] = (char)('0' + centiseconds % 10u);
    out[8] = '\0';
    return 8;
}
