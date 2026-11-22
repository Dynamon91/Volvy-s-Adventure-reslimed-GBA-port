#include <assert.h>
#include <stdio.h>
#include "core/sprite_visibility.h"

int main(void)
{
    /* Exactly touching the viewport edge is not visible. */
    assert(!sprite_part_intersects_viewport(-16, 40, 16, 16));
    assert(!sprite_part_intersects_viewport(240, 40, 16, 16));
    assert(!sprite_part_intersects_viewport(10, -32, 16, 32));
    assert(!sprite_part_intersects_viewport(10, 160, 16, 16));

    /* A partially visible actor must still get its animation upload. */
    assert(sprite_part_intersects_viewport(-15, 40, 16, 16));
    assert(sprite_part_intersects_viewport(239, 40, 16, 16));
    assert(sprite_part_intersects_viewport(10, -31, 16, 32));
    assert(sprite_part_intersects_viewport(10, 159, 16, 16));
    assert(sprite_part_intersects_viewport(20, 20, 16, 16));

    /* A distant sprite must not wrap from the OBJ coordinate register back
     * into the viewport (hardware X has 9 bits, Y has 8 bits). */
    assert(!sprite_part_intersects_viewport(-300, 80, 32, 32));
    assert(!sprite_part_intersects_viewport(540, 80, 32, 32));
    assert(!sprite_part_intersects_viewport(120, 420, 32, 32));
    assert(!sprite_part_intersects_viewport(120, -260, 32, 32));
    assert(!sprite_part_intersects_viewport(0, 0, 0, 32));
    puts("OBJ visibility/edge and wrapped-coordinate bounds: PASS");
    return 0;
}
