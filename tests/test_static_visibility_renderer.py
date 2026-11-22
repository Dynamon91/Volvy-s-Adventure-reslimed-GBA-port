#!/usr/bin/env python3
"""Verify world/bonus sprite occlusion is tested before VBlank scheduling.

This catches regressions where the per-set shared budget is consumed by an
animation which will not draw any OBJ parts in the current viewport.
"""
from pathlib import Path
r = (Path(__file__).resolve().parents[1] / 'source/platform/render.c').read_text()
assert '#include "core/sprite_visibility.h"' in r
assert 'sprite_part_intersects_viewport(x, y, part->hw_w, part->hw_h)' in r
assert 'if (gfx->loaded_frame >= gfx->animation->frame_count) return 0;' in r
assert 'return static_frame_visible(loaded,' in r
for start, end in [('static int draw_world_actors(', 'static const GbaSpriteAnim *choose_player_anim('),
                   ('static int draw_bonus_actors(', 'void render_draw(')]:
    body = r.split(start, 1)[1].split(end, 1)[0]
    assert body.count('if (!static_actor_visible(') == 1
    assert body.count('frame_index = request_static_frame(gfx_id, frame_index);') == 1
    assert body.index('if (!static_actor_visible(') < body.index('frame_index = request_static_frame(gfx_id, frame_index);')
    assert 'continue;' in body.split('if (!static_actor_visible(', 1)[1].split('frame_index = request_static_frame(', 1)[0]
print('Renderer off-screen OBJ upload gating: PASS (World 1 + Bonus, target/resident frame)')
