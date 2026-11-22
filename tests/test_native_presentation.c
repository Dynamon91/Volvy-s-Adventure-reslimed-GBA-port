#include <assert.h>
#include <stdio.h>
#include "core/native_scene.h"

static void incremental_scroll_checks(void)
{
    NativeScenePresentation p;
    int cx, cy, px, py, new_x, new_y;
    int previous_x, previous_y;
    native_scene_presentation_reset(&p);
    assert(p.has_visible_map == 0);
    assert(native_scene_presentation_camera(&p, 320, 150, 40, 12,
                                            &px, &py) == 0);
    assert(px == 320 && py == 150);
    native_scene_presentation_commit(&p, 0, 0);
    assert(p.has_visible_map && p.visible_start_x == 0 && p.visible_start_y == 0);

    /* Camera 17px requests BG window shifted by one source tile; display
       remains at x=16 until the window is actually committed. */
    native_scene_window_origin(17, 30, 283, 45, &new_x, &new_y);
    assert(new_x == 1 && new_y == 0);
    assert(!native_scene_presentation_camera(&p, 17, 30, new_x, new_y,
                                             &px, &py));
    assert(px == 16 && py == 30);
    native_scene_presentation_commit(&p, new_x, new_y);
    assert(!native_scene_presentation_camera(&p, 17, 30, new_x, new_y,
                                             &px, &py));
    assert(px == 17 && py == 30);

    /* Playback along a full source room: before presenting the newly staged
       map, the viewport stays completely inside the old visible 256px map. */
    previous_x = 17;
    previous_y = 30;
    for (cy = 30; cy <= 200; ++cy) {
        for (cx = 17; cx <= 2024; ++cx) {
            int desired_x = (cy - 30) & 1 ? 2024 - (cx - 17) : cx;
            native_scene_window_origin(desired_x, cy, 283, 45, &new_x, &new_y);
            assert(!native_scene_presentation_camera(&p, desired_x, cy,
                                                     new_x, new_y,
                                                     &px, &py));
            assert(px >= p.visible_start_x * 8);
            assert(px <= p.visible_start_x * 8 + 16);
            assert(py >= p.visible_start_y * 8);
            assert(py <= p.visible_start_y * 8 + 96);
            assert(px >= previous_x - 16);
            assert(py >= previous_y - 96);
            native_scene_presentation_commit(&p, new_x, new_y);
            previous_x = desired_x;
            previous_y = cy;
        }
    }
}

static void vertical_scroll_and_warp_checks(void)
{
    NativeScenePresentation p;
    int ox, oy, x, y;
    native_scene_presentation_reset(&p);
    native_scene_presentation_commit(&p, 0, 0);
    native_scene_window_origin(0, 97, 664, 140, &ox, &oy);
    assert(ox == 0 && oy == 1);
    assert(!native_scene_presentation_camera(&p, 0, 97, ox, oy, &x, &y));
    assert(x == 0 && y == 96);

    /* A sudden spawn relocation must hide BG1 during the hard map swap,
       not display a 1k-pixel jump using the previous map's scroll offset. */
    native_scene_window_origin(1200, 600, 664, 140, &ox, &oy);
    assert(native_scene_presentation_camera(&p, 1200, 600, ox, oy, &x, &y));
    assert(x == 16 && y == 96);
    native_scene_presentation_commit(&p, ox, oy);
    assert(!native_scene_presentation_camera(&p, 1200, 600, ox, oy, &x, &y));
    assert(x == 1200 && y == 600);
}

int main(void)
{
    incremental_scroll_checks();
    vertical_scroll_and_warp_checks();
    puts("Native scene presentation: PASS (committed scroll, 344k camera positions, 2D staging and warp)");
    return 0;
}
