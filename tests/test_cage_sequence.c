#include <assert.h>
#include <stdio.h>
#include "core/cage_sequence.h"
#include "shrewy_cage_sequence.h"

int main(void)
{
    ShrewyCageSequencePlayer player = {0, 0, 0, 0};
    fx32 start = shrewy_cage_drop_offset(0);
    fx32 at20 = shrewy_cage_drop_offset(20);
    fx32 at21 = shrewy_cage_drop_offset(21);
    fx32 end = shrewy_cage_drop_offset(59);
    fx32 last = 0;
    unsigned int i;

    assert(SHREWY_CAGE_SEQUENCE_FPS == 60);
    assert(SHREWY_CAGE_SEQUENCE_FRAMES == 60);
    assert(SHREWY_CAGE_POSITION_KEY_LENGTH == 67);
    assert(start == fx_from_int(-88));
    assert(at20 > fx_from_int(29) && at20 < fx_from_int(31));
    assert(at21 > fx_from_int(39) && at21 < fx_from_int(41));
    assert(end > fx_from_int(37) && end < fx_from_int(38));
    assert(shrewy_cage_drop_offset(60) == end);
    assert(shrewy_cage_drop_offset(255) == end);

    shrewy_cage_sequence_start(&player);
    assert(player.frame == 0 && player.active && !player.finished);
    assert(shrewy_cage_sequence_is_active(&player));
    assert(shrewy_cage_sequence_current_offset(&player) == start);
    for (i = 0; i < SHREWY_CAGE_SEQUENCE_FRAMES; ++i) {
        last = shrewy_cage_sequence_update(&player);
        if (i < SHREWY_CAGE_SEQUENCE_FRAMES - 1)
            assert(player.active && player.frame == i + 1);
    }
    assert(last == end);
    assert(player.frame == SHREWY_CAGE_SEQUENCE_FRAMES - 1);
    assert(!player.active && player.finished);
    assert(!shrewy_cage_sequence_is_active(&player));
    assert(shrewy_cage_sequence_update(&player) == end);

    shrewy_cage_sequence_start(&player);
    assert(player.frame == 0 && player.active && !player.finished);
    assert(shrewy_cage_sequence_update(&player) == start);
    assert(player.frame == 1);

    puts("Shrewy source cage-drop curve and 60 Hz sequence-player tests: PASS");
    return 0;
}
