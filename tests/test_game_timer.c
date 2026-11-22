#include <assert.h>
#include <string.h>
#include "core/game_timer.h"

int main(void)
{
    char out[9];

    assert(game_timer_format(0, out) == 7);
    assert(strcmp(out, "0:00:00") == 0);
    assert(game_timer_format(60, out) == 7);
    assert(strcmp(out, "0:01:00") == 0);
    assert(game_timer_format(90, out) == 7);
    assert(strcmp(out, "0:01:50") == 0);
    assert(game_timer_format(3599, out) == 7);
    assert(strcmp(out, "0:59:98") == 0);
    assert(game_timer_format(3600, out) == 7);
    assert(strcmp(out, "1:00:00") == 0);
    assert(game_timer_format(36000, out) == 8);
    assert(strcmp(out, "10:00:00") == 0);
    assert(game_timer_format(36000u * 120u, out) == 8);
    assert(strcmp(out, "99:00:00") == 0);
    return 0;
}
