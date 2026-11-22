#include "core/game.h"

#include "core/camera.h"
#include "core/boss_shrewy.h"
#include "core/bonus_race.h"
#include "core/input.h"
#include "core/level.h"
#include "core/player.h"
#include "core/world_interactions.h"
#include "platform/render.h"
#include "rm_w1_l1_scene.h"
#include "shrewy_room.h"
#include "rm_w1_l2_scene.h"
#include "rm_w1_l3_scene.h"

static Level s_level;
static Player s_player;
static Camera s_camera;
static ShrewyBoss s_shrewy;
static BonusRace s_bonus_race;
static GameMode s_mode;
static GameMode s_resume_mode;
static uint32_t s_anim_tick;

#ifndef VOLVY_DEV_STAGE_SELECT
#define VOLVY_DEV_STAGE_SELECT 1
#endif

static void reset_world1(uint8_t stage)
{
    level_init_world1_stage(&s_level, stage);
    /* Provisional safe spawn: original room object at (64,384) requires script verification. */
    player_init(&s_player, 25, stage == 3 ? 142 : 150);
    camera_init(&s_camera);
    render_select_stage(stage);
    s_mode = GAME_PLAYING;
    s_resume_mode = GAME_PLAYING;
    s_anim_tick = 0;
}

static void reset_boss_shrewy(void)
{
    level_init_shrewy(&s_level);
    /* Safe floor-aligned spawn; source obj_volvy pivot (64,160) is below the
       144px room and its room-create relocation has not yet been decoded. */
    player_init_native(&s_player, 50, SHREWY_ARENA_FLOOR_Y - 19);
    camera_init(&s_camera);
    shrewy_init(&s_shrewy);
    render_select_boss();
    s_mode = GAME_BOSS;
    s_resume_mode = GAME_BOSS;
    s_anim_tick = 0;
}

static void reset_bonus_race(void)
{
    level_init_bonus_race(&s_level);
    /* Source trace clip 1 begins at pivot (264.3,535.0) source px.
     * Convert to GBA scale and preserve the normal player's (7,6) pivot. */
    player_init(&s_player, 125, 261);
    camera_init(&s_camera);
    /* Start with the trace-derived spawn already framed on the 240x160 view. */
    s_camera.x = fx_from_int(27);
    s_camera.y = fx_from_int(186);
    bonus_race_init(&s_bonus_race);
    render_select_bonus();
    s_mode = GAME_BONUS;
    s_resume_mode = GAME_BONUS;
    s_anim_tick = 0;
}

static uint8_t next_title_stage(uint8_t current)
{
    if (current == 1) return 2;
    if (current == 2) return 3;
    if (current == 3) return 5;
    return 1;
}

static void select_title_stage(uint8_t stage)
{
    if (stage == 5) {
        level_init_bonus_race(&s_level);
        bonus_race_init(&s_bonus_race);
        render_select_bonus();
    } else {
        level_init_world1_stage(&s_level, stage);
        render_select_stage(stage);
    }
    camera_init(&s_camera);
}

void game_init(void)
{
    level_init_world1(&s_level);
    /* Provisional safe spawn: original room object at (64,384) requires script verification. */
    player_init(&s_player, 25, 150);
    camera_init(&s_camera);
    s_mode = GAME_TITLE;
    s_resume_mode = GAME_PLAYING;
    s_anim_tick = 0;

    render_init(&s_level);
}

void game_update(void)
{
    const InputState *input = input_get_state();
    ++s_anim_tick;

    switch (s_mode) {
    case GAME_TITLE:
        /* Development-only level selector: enables direct tests of L2/L3.
         * This is not an original Volvy menu behavior. */
#if VOLVY_DEV_STAGE_SELECT
        if (input_was_pressed(KEY_SELECT)) {
            uint8_t next = next_title_stage(s_level.active_stage);
            select_title_stage(next);
        }
#endif
        if (input_was_pressed(KEY_START)) {
            if (s_level.bonus_room) reset_bonus_race();
            else reset_world1(s_level.active_stage);
        }
        break;

    case GAME_PLAYING:
        if (input_was_pressed(KEY_START)) {
            s_resume_mode = GAME_PLAYING;
            s_mode = GAME_PAUSED;
            break;
        }

        {
            int old_bottom = fx_to_int(s_player.y) + s_player.height;
            int descending = s_player.vy >= 0;
            level_tick_world_actors(&s_level);
            player_update(&s_player, input, &s_level);
            (void)world_interact_player(&s_level, &s_player,
                                        old_bottom, descending);
        }

        camera_update(&s_camera, &s_player,
                      s_level.active_stage == 3 ? RM_W1_L3_SCENE_SCENE_WIDTH :
                      s_level.active_stage == 2 ? RM_W1_L2_SCENE_SCENE_WIDTH :
                                                  RM_W1_L1_SCENE_SCENE_WIDTH,
                      s_level.active_stage == 3 ? RM_W1_L3_SCENE_SCENE_HEIGHT :
                      s_level.active_stage == 2 ? RM_W1_L2_SCENE_SCENE_HEIGHT :
                                                  RM_W1_L1_SCENE_SCENE_HEIGHT);

        if (s_player.dead) {
            s_mode = GAME_OVER;
        } else if (level_reached_world_exit(&s_level,
                       fx_to_int(s_player.x), fx_to_int(s_player.y),
                       s_player.width, s_player.height)) {
            s_mode = GAME_VICTORY;
        }
        break;

    case GAME_BONUS:
        if (input_was_pressed(KEY_START)) {
            s_resume_mode = GAME_BONUS;
            s_mode = GAME_PAUSED;
            break;
        }
        {
            int old_x = fx_to_int(s_player.x);
            int old_bottom = fx_to_int(s_player.y) + s_player.height;
            int descending = s_player.vy >= 0;
            player_update(&s_player, input, &s_level);
            (void)bonus_race_update(&s_bonus_race, &s_level, &s_player,
                                   old_x, old_bottom, descending);
        }
        camera_update(&s_camera, &s_player,
                      level_width_pixels(&s_level),
                      level_height_pixels(&s_level));
        if (s_player.dead) s_mode = GAME_OVER;
        else if (s_bonus_race.finished) s_mode = GAME_VICTORY;
        break;

    case GAME_BOSS:
        if (input_was_pressed(KEY_START)) {
            s_resume_mode = GAME_BOSS;
            s_mode = GAME_PAUSED;
            break;
        }
        {
            int old_bottom = fx_to_int(s_player.y) + s_player.height;
            int descending = s_player.vy >= 0;
            player_update(&s_player, input, &s_level);
            shrewy_update(&s_shrewy, &s_player, old_bottom, descending);
        }
        camera_update(&s_camera, &s_player,
                      SHREWY_ROOM_WIDTH, SHREWY_ROOM_HEIGHT);
        if (s_player.dead) s_mode = GAME_OVER;
        else if (shrewy_is_defeated(&s_shrewy)) s_mode = GAME_VICTORY;
        break;

    case GAME_PAUSED:
        if (input_was_pressed(KEY_START)) {
            s_mode = s_resume_mode;
        }
        break;

    case GAME_OVER:
        if (input_was_pressed(KEY_START)) {
            if (s_level.boss_room) reset_boss_shrewy();
            else if (s_level.bonus_room) reset_bonus_race();
            else reset_world1(s_level.active_stage);
        }
        break;
    case GAME_VICTORY:
        if (input_was_pressed(KEY_START)) {
            if (s_level.boss_room || s_level.bonus_room) {
                s_mode = GAME_TITLE;
                level_init_world1_stage(&s_level, 1);
                render_select_stage(1);
                camera_init(&s_camera);
            } else if (s_level.active_stage < 3) {
                reset_world1((uint8_t)(s_level.active_stage + 1));
            } else {
                reset_boss_shrewy();
            }
        }
        break;

    default:
        s_mode = GAME_TITLE;
        break;
    }
}

void game_render(void)
{
    render_draw(&s_level, &s_player, &s_shrewy, &s_bonus_race,
                &s_camera, s_mode, s_anim_tick);
}

void game_vblank_commit(void)
{
    render_vblank_commit(&s_level, &s_camera);
}
