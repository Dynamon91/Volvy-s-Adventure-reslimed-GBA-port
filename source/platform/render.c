#include <string.h>
#include <gba.h>

#include "platform/render.h"
#include "platform/gba_display.h"
#include "player_sprites.h"
#include "boss_sprites.h"
#include "core/boss_shrewy.h"
#include "core/scene_window.h"
#include "shrewy_room.h"
#include "shrewy_scene.h"
#include "rm_w1_l1_scene.h"
#include "rm_w1_l2_scene.h"
#include "rm_w1_l3_scene.h"
#include "w1_l1_objects.h"
#include "bonus_w1_room.h"
#include "bonus_w1_composite.h"
#include "platform/ui.h"

#define BG_MAP_MAIN ((u16 *)0x0600D800)
#define BG_CHAR_BLOCK_1 ((u8 *)0x06004000)
#define OBJ_VRAM ((u8 *)0x06010000)
#define MAX_PLAYER_PARTS 8
#define STATIC_ANIM_COUNT 11
#define MAX_STATIC_FRAMES 16
#define STATIC_OBJ_BASE_32 64 /* reserve first 2048B for animated player */
#define OBJ_8BPP_BIT (1u << 13)
#define OBJ_HFLIP_BIT (1u << 12)
#define BG_8BPP_BIT (1u << 7)
#define STREAM_MAP_W SCENE_WINDOW_TILES_W
#define STREAM_MAP_H SCENE_WINDOW_TILES_H

static u16 s_scene_map[STREAM_MAP_W * STREAM_MAP_H] ALIGN(4);
static int s_scene_start_x = -1;
static int s_scene_start_y = -1;
static int s_scene_map_dirty = 1;
static uint8_t s_stage_requested = 1;
static uint8_t s_stage_loaded = 0;
static const GbaSpriteFrame *s_pending_frame;
static const GbaSpriteFrame *s_uploaded_frame;
static const GbaSpriteFrame *s_pending_boss_frame;
static const GbaSpriteFrame *s_uploaded_boss_frame;
static const GbaSpriteFrame *s_pending_rock_frame;
static const GbaSpriteFrame *s_uploaded_rock_frame;
static u16 s_boss_onion_base32[16];
static u8 s_boss_onion_frame_count;

static const u16 *scene_map_source(void)
{
    if (s_stage_requested == 5) return bonus_w1_composite_map;
    if (s_stage_requested == 4) return shrewy_scene_map;
    if (s_stage_requested == 3) return rm_w1_l3_scene_map;
    if (s_stage_requested == 2) return rm_w1_l2_scene_map;
    return rm_w1_l1_scene_map;
}
static int scene_map_width(void)
{
    if (s_stage_requested == 5) return BONUS_W1_COMPOSITE_MAP_WIDTH;
    if (s_stage_requested == 4) return SHREWY_SCENE_MAP_WIDTH;
    if (s_stage_requested == 3) return RM_W1_L3_SCENE_MAP_WIDTH;
    if (s_stage_requested == 2) return RM_W1_L2_SCENE_MAP_WIDTH;
    return RM_W1_L1_SCENE_MAP_WIDTH;
}
static int scene_map_height(void)
{
    if (s_stage_requested == 5) return BONUS_W1_COMPOSITE_MAP_HEIGHT;
    if (s_stage_requested == 4) return SHREWY_SCENE_MAP_HEIGHT;
    if (s_stage_requested == 3) return RM_W1_L3_SCENE_MAP_HEIGHT;
    if (s_stage_requested == 2) return RM_W1_L2_SCENE_MAP_HEIGHT;
    return RM_W1_L1_SCENE_MAP_HEIGHT;
}
void render_select_stage(uint8_t stage)
{
    if (stage < 1 || stage > 3) stage = 1;
    s_stage_requested = stage;
    s_scene_start_x = -1;
    s_scene_start_y = -1;
    s_scene_map_dirty = 1;
    s_pending_boss_frame = 0;
    s_pending_rock_frame = 0;
}

void render_select_boss(void)
{
    s_stage_requested = 4;
    s_scene_start_x = -1;
    s_scene_start_y = -1;
    s_scene_map_dirty = 1;
    s_pending_frame = 0;
    s_uploaded_frame = 0;
    s_pending_boss_frame = 0;
    s_uploaded_boss_frame = 0;
    s_pending_rock_frame = 0;
    s_uploaded_rock_frame = 0;
}

void render_select_bonus(void)
{
    s_stage_requested = 5;
    s_scene_start_x = -1;
    s_scene_start_y = -1;
    s_scene_map_dirty = 1;
    s_pending_boss_frame = 0;
    s_uploaded_boss_frame = 0;
    s_pending_rock_frame = 0;
    s_uploaded_rock_frame = 0;
}

/* Static world actors share fully preloaded OBJ frame tiles. On-screen
 * instance animation selection does not require a per-instance VRAM upload. */
typedef enum StaticAnimId {
    STATIC_ONION = 0,
    STATIC_BREAKABLE,
    STATIC_ACTION,
    STATIC_ACTION_SPENT,
    STATIC_SQUIRREL_WALK,
    STATIC_SQUIRREL_IDLE,
    STATIC_SPRING,
    STATIC_SWOLVY_BLOCK,
    STATIC_BUNNY_HOP,
    STATIC_BUMPER_SQ,
    STATIC_BUMPER_TRI
} StaticAnimId;

typedef struct StaticAnimGraphic {
    const GbaSpriteAnim *animation;
    u16 frame_tile_base32[MAX_STATIC_FRAMES];
} StaticAnimGraphic;

static StaticAnimGraphic s_static_gfx[STATIC_ANIM_COUNT];

static void load_actor_graphics(void)
{
    const GbaSpriteAnim *const animations[STATIC_ANIM_COUNT] = {
        &g_spr_onion, &g_spr_destructible_block, &g_spr_action_block,
        &g_spr_action_block_spent, &g_spr_squirrel_walk,
        &g_spr_squirrel_idle, &g_spr_springboard,
        &g_spr_destructible_block_swolvy, &g_spr_bunny_hop,
        &g_spr_bumper_sq, &g_spr_bumper_tri
    };
    u16 base = STATIC_OBJ_BASE_32;
    int i, f;

    for (i = 0; i < STATIC_ANIM_COUNT; ++i) {
        const GbaSpriteAnim *anim = animations[i];
        s_static_gfx[i].animation = anim;
        for (f = 0; f < anim->frame_count && f < MAX_STATIC_FRAMES; ++f) {
            const GbaSpriteFrame *frame = &anim->frames[f];
            const GbaSpritePart *part;
            if (frame->part_count != 1) {
                s_static_gfx[i].frame_tile_base32[f] = 0;
                continue;
            }
            part = &frame->parts[0];
            /* 8bpp OBJ entries must begin on even 32-byte tile indices. */
            if (base & 1u) ++base;
            if ((unsigned int)base + part->tile_bytes / 32u > 1024u) {
                s_static_gfx[i].frame_tile_base32[f] = 0;
                continue;
            }
            s_static_gfx[i].frame_tile_base32[f] = base;
            dmaCopy(part->tiles, OBJ_VRAM + base * 32u, part->tile_bytes);
            base = (u16)(base + part->tile_bytes / 32u);
        }
    }
}

static int static_animation_for_actor(const Level *level, int index)
{
    switch (level_world_object(level, index)->kind) {
    case WORLD_ONION: return STATIC_ONION;
    case WORLD_BREAKABLE: return STATIC_BREAKABLE;
    case WORLD_ACTION_BLOCK:
        return level->actor_cooldown[index] ? STATIC_ACTION_SPENT : STATIC_ACTION;
    case WORLD_SQUIRREL:
        return level->actor_dx[index] || level->actor_frame > 8u
               ? STATIC_SQUIRREL_WALK : STATIC_SQUIRREL_IDLE;
    case WORLD_SPRINGBOARD: return STATIC_SPRING;
    default: return -1;
    }
}

static void draw_world_actors(const Level *level, const Camera *camera,
                              OBJATTR *oam, int next_oam,
                              uint32_t anim_tick)
{
    int i;
    int cam_x = fx_to_int(camera->x);
    int cam_y = fx_to_int(camera->y);
    if (!level->world1_active) return;
    for (i = 0; i < level_world_object_count(level) && next_oam < VOLVY_OAM_COUNT; ++i) {
        const WorldObject *object = level_world_object(level, i);
        const StaticAnimGraphic *gfx;
        const GbaSpriteAnim *anim;
        const GbaSpriteFrame *frame;
        const GbaSpritePart *part;
        unsigned int frame_index;
        int gfx_id, x, y;
        if (!level_object_active(level, i)) continue;
        gfx_id = static_animation_for_actor(level, i);
        if (gfx_id < 0) continue;
        gfx = &s_static_gfx[gfx_id];
        anim = gfx->animation;
        if (!anim->frame_count) continue;
        frame_index = (anim_tick / (anim->frame_duration ?
                      anim->frame_duration : 1u)) % anim->frame_count;
        if (object->kind == WORLD_SPRINGBOARD) {
            if (level->actor_cooldown[i] && anim->frame_count > 1)
                frame_index = 1u +
                    ((level->actor_cooldown[i] / 4u) % (anim->frame_count - 1u));
            else frame_index = 0;
        }
        if (frame_index >= MAX_STATIC_FRAMES) continue;
        frame = &anim->frames[frame_index];
        if (frame->part_count != 1) continue;
        part = &frame->parts[0];
        x = object->x + level_world_actor_offset(level, i) -
            frame->origin_x + part->offset_x - cam_x;
        y = object->y - frame->origin_y + part->offset_y - cam_y;
        if (x + part->hw_w <= 0 || x >= 240 ||
            y + part->hw_h <= 0 || y >= 160) continue;
        oam[next_oam].attr0 = (u16)((y & 255) | OBJ_8BPP_BIT |
                             ((u16)part->shape << 14));
        oam[next_oam].attr1 = (u16)((x & 511) |
                             (object->kind == WORLD_SQUIRREL &&
                              level->actor_dir[i] < 0 ? OBJ_HFLIP_BIT : 0) |
                             ((u16)part->size_code << 14));
        oam[next_oam].attr2 = gfx->frame_tile_base32[frame_index];
        oam[next_oam].dummy = 0;
        ++next_oam;
    }
}

static const GbaSpriteAnim *choose_player_anim(const Player *player,
                                                GameMode mode)
{
    if (mode == GAME_TITLE) return &g_spr_volvy_title;
    if (mode == GAME_OVER) return &g_spr_volvy_hurt;
    if (mode == GAME_VICTORY) return &g_spr_volvy_clear;

    if (player->swole) {
        switch ((HandState)player->hand_state) {
        case HAND_HOVER: return &g_spr_swolvy_float;
        case HAND_GRIND: return &g_spr_swolvy_grind;
        case HAND_GROUND_POUND: return &g_spr_swolvy_ground_pound;
        case HAND_PUNCH: return &g_spr_swolvy_punch;
        case HAND_PUNCH_UP: return &g_spr_swolvy_punch_up;
        case HAND_DIG: return &g_spr_dig_swole;
        case HAND_DASH_UP: return &g_spr_swolvy_jump;
        case HAND_DASH: return &g_spr_swolvy_walk;
        default: break;
        }
        if (player->grounded) {
            return player->vx ? &g_spr_swolvy_walk : &g_spr_swolvy_idle;
        }
        return player->vy < 0 ? &g_spr_swolvy_jump : &g_spr_swolvy_float;
    }

    switch ((HandState)player->hand_state) {
    case HAND_HOVER: return &g_spr_float;
    case HAND_GRIND: return &g_spr_grind;
    case HAND_GROUND_POUND: return &g_spr_ground_pound;
    case HAND_PUNCH: return &g_spr_punch;
    case HAND_PUNCH_UP: return &g_spr_punch_up;
    case HAND_DIG: return &g_spr_dig;
    case HAND_DASH: return &g_spr_dash;
    case HAND_DASH_UP: return &g_spr_dash_up;
    default: break;
    }

    if (player->grounded) {
        return player->vx ? &g_spr_walk : &g_spr_idle;
    }
    return player->vy < 0 ? &g_spr_jump : &g_spr_float;
}

static const GbaSpriteAnim *choose_boss_player_anim(const Player *player)
{
    if (player->dead) return &g_boss_spr_volvy_hurt;
    if (player->swole) {
        switch ((HandState)player->hand_state) {
        case HAND_HOVER: return &g_boss_spr_swolvy_float;
        case HAND_GRIND: return &g_boss_spr_swolvy_grind;
        case HAND_PUNCH: return &g_boss_spr_swolvy_punch;
        case HAND_PUNCH_UP: return &g_boss_spr_swolvy_punch_up;
        case HAND_GROUND_POUND: return &g_boss_spr_swolvy_ground_pound;
        case HAND_DIG: return &g_boss_spr_dig_swole;
        case HAND_DASH_UP: return &g_boss_spr_swolvy_jump;
        case HAND_DASH: return &g_boss_spr_swolvy_walk;
        default: break;
        }
        if (player->grounded)
            return player->vx ? &g_boss_spr_swolvy_walk : &g_boss_spr_swolvy_idle;
        return player->vy < 0 ? &g_boss_spr_swolvy_jump : &g_boss_spr_swolvy_float;
    }

    switch ((HandState)player->hand_state) {
    case HAND_HOVER: return &g_boss_spr_float;
    case HAND_GRIND: return &g_boss_spr_grind;
    case HAND_PUNCH: return &g_boss_spr_punch;
    case HAND_PUNCH_UP: return &g_boss_spr_punch_up;
    case HAND_GROUND_POUND: return &g_boss_spr_ground_pound;
    case HAND_DIG: return &g_boss_spr_dig;
    case HAND_DASH: return &g_boss_spr_dash;
    case HAND_DASH_UP: return &g_boss_spr_dash_up;
    default: break;
    }
    if (player->grounded)
        return player->vx ? &g_boss_spr_walk : &g_boss_spr_idle;
    return player->vy < 0 ? &g_boss_spr_jump : &g_boss_spr_float;
}

static const GbaSpriteAnim *choose_shrewy_anim(const ShrewyBoss *boss)
{
    switch ((ShrewyState)boss->state) {
    case SHREWY_WINDUP: return &g_boss_spr_shrewy_windup;
    case SHREWY_JUMP: return &g_boss_spr_shrewy_jump;
    case SHREWY_STOMP: return &g_boss_spr_shrewy_stomp;
    case SHREWY_DROPKICK: return &g_boss_spr_shrewy_dropkick;
    case SHREWY_TORNADO: return &g_boss_spr_shrewy_tornado;
    case SHREWY_HURT:
    case SHREWY_DEFEATED: return &g_boss_spr_shrewy_hurt;
    case SHREWY_IDLE:
    case SHREWY_RECOVER:
    default: return &g_boss_spr_shrewy_idle;
    }
}

static void scene_stream_update(const Camera *camera)
{
    int origin_x, origin_y;
    scene_window_origin(fx_to_int(camera->x), fx_to_int(camera->y),
                        scene_map_width(), scene_map_height(),
                        &origin_x, &origin_y);
    if (origin_x == s_scene_start_x && origin_y == s_scene_start_y &&
        !s_scene_map_dirty) return;

    scene_window_copy(scene_map_source(), scene_map_width(), scene_map_height(),
                      origin_x, origin_y, s_scene_map);
    s_scene_start_x = origin_x;
    s_scene_start_y = origin_y;
    s_scene_map_dirty = 1;
}

void render_init(Level *level)
{
    (void)level;

    /* Disable layers while the original scene tiles and palettes are installed. */
    SetMode(MODE_0);

    /* Initial W1 L1 scene; subsequent switches upload in VBlank. */
    dmaCopy(rm_w1_l1_scene_tiles, BG_CHAR_BLOCK_1,
            RM_W1_L1_SCENE_TILE_COUNT * 64u);
    dmaCopy(rm_w1_l1_scene_palette, (void *)BG_COLORS,
            sizeof(rm_w1_l1_scene_palette));
    s_stage_loaded = 1;
    s_stage_requested = 1;

    /* BG1 is an 8bpp 64x32 streaming map using character base block 1. */
    BGCTRL[1] = BG_SIZE_1 | CHAR_BASE(1) | SCREEN_BASE(27) |
                BG_8BPP_BIT | 1;

    BG_OFFSET[0].x = 0; BG_OFFSET[0].y = 0;
    BG_OFFSET[1].x = 0; BG_OFFSET[1].y = 0;
    BG_OFFSET[2].x = 0; BG_OFFSET[2].y = 0;
    BG_OFFSET[3].x = 0; BG_OFFSET[3].y = 0;

    memcpy((void *)OBJ_COLORS, g_player_obj_palette,
           sizeof(g_player_obj_palette));
    load_actor_graphics();
    gba_ui_init();

    s_scene_start_x = -1;
    s_scene_start_y = -1;
    s_scene_map_dirty = 1;
    s_pending_frame = 0;
    s_uploaded_frame = 0;
    s_pending_boss_frame = 0;
    s_uploaded_boss_frame = 0;
    s_pending_rock_frame = 0;
    s_uploaded_rock_frame = 0;

    SetMode(MODE_0 | BG0_ON | BG1_ON | OBJ_ON | OBJ_1D_MAP);
}

static void hide_all_objects(OBJATTR *oam)
{
    int i;
    for (i = 0; i < VOLVY_OAM_COUNT; ++i) {
        oam[i].attr0 = ATTR0_DISABLED;
        oam[i].attr1 = 0;
        oam[i].attr2 = 0;
        oam[i].dummy = 0;
    }
}

static void draw_frame_parts(const GbaSpriteFrame *frame, OBJATTR *oam,
                             int first_oam, int base_x, int base_y,
                             int facing_left, int base_tile64)
{
    int tile_index = 0;
    int i;
    int count = frame->part_count;
    if (count > MAX_PLAYER_PARTS) count = MAX_PLAYER_PARTS;
    for (i = 0; i < count; ++i) {
        const GbaSpritePart *part = &frame->parts[i];
        int x = base_x + part->offset_x;
        int y = base_y + part->offset_y;
        if (tile_index & 1) ++tile_index;
        if (facing_left)
            x = base_x + frame->width - part->offset_x - part->hw_w;
        oam[first_oam + i].attr0 = (u16)((y & 0x00FF) | OBJ_8BPP_BIT |
                                         ((u16)part->shape << 14));
        oam[first_oam + i].attr1 = (u16)((x & 0x01FF) |
                                         (facing_left ? OBJ_HFLIP_BIT : 0) |
                                         ((u16)part->size_code << 14));
        /* OBJ attr2 addresses 32-byte units; 8bpp tile payloads are 64B. */
        oam[first_oam + i].attr2 = (u16)((base_tile64 + tile_index) * 2);
        oam[first_oam + i].dummy = 0;
        tile_index += part->tile_bytes / 64;
    }
}

static int draw_boss_room_props(const Level *level, OBJATTR *oam, int next_oam,
                                int cam_x, int cam_y, uint32_t anim_tick)
{
    const GbaSpriteAnim *anim = &g_boss_spr_onion;
    unsigned int frame_index;
    int i;
    if (!anim->frame_count || !s_boss_onion_frame_count) return next_oam;
    frame_index = (anim_tick / (anim->frame_duration ? anim->frame_duration : 1u)) %
                  anim->frame_count;
    if (frame_index >= s_boss_onion_frame_count) frame_index = 0;
    for (i = 0; i < SHREWY_ROOM_PROP_COUNT && next_oam < VOLVY_OAM_COUNT; ++i) {
        const ShrewyProp *prop = &shrewy_props[i];
        const GbaSpriteFrame *frame;
        const GbaSpritePart *part;
        int x, y;
        if (prop->kind != SHREWY_PROP_ONION || level->removed[i]) continue;
        frame = &anim->frames[frame_index];
        if (frame->part_count != 1) continue;
        part = &frame->parts[0];
        x = prop->x - frame->origin_x - cam_x + part->offset_x;
        y = prop->y - frame->origin_y - cam_y + part->offset_y;
        if (x + part->hw_w <= 0 || x >= 240 || y + part->hw_h <= 0 || y >= 160)
            continue;
        oam[next_oam].attr0 = (u16)((y & 255) | OBJ_8BPP_BIT |
                                    ((u16)part->shape << 14));
        oam[next_oam].attr1 = (u16)((x & 511) | ((u16)part->size_code << 14));
        oam[next_oam].attr2 = s_boss_onion_base32[frame_index];
        oam[next_oam].dummy = 0;
        ++next_oam;
    }
    return next_oam;
}

static int bonus_static_animation_for_instance(const Level *level,
                                               const BonusW1RoomInstance *instance,
                                               int index)
{
    switch (instance->kind) {
    case BONUS_W1_ROOM_OBJ_ONION:
        return STATIC_ONION;
    case BONUS_W1_ROOM_OBJ_DESTRUCTIBLE_BLOCK:
    case BONUS_W1_ROOM_OBJ_DESTRUCTIBLE_BLOCK_ONEWAY:
        return STATIC_BREAKABLE;
    case BONUS_W1_ROOM_OBJ_DESTRUCTIBLE_BLOCK_SWOLVY:
        return STATIC_SWOLVY_BLOCK;
    case BONUS_W1_ROOM_OBJ_BUNNY:
    case BONUS_W1_ROOM_OBJ_BUNNY_GROUND:
        return STATIC_BUNNY_HOP;
    case BONUS_W1_ROOM_OBJ_BOUNCE_BLOCK:
        return STATIC_BUMPER_SQ;
    case BONUS_W1_ROOM_OBJ_CORNER_BUMPER:
        return STATIC_BUMPER_TRI;
    case BONUS_W1_ROOM_OBJ_ACTION_BLOCK:
        return level->actor_cooldown[index] ? STATIC_ACTION_SPENT : STATIC_ACTION;
    case BONUS_W1_ROOM_OBJ_SQUIRREL:
        return STATIC_SQUIRREL_WALK;
    case BONUS_W1_ROOM_OBJ_SPRINGBOARD:
        return STATIC_SPRING;
    default:
        return -1;
    }
}

static int draw_bonus_actors(const Level *level, const BonusRace *race, OBJATTR *oam,
                             int next_oam, int cam_x, int cam_y,
                             uint32_t anim_tick)
{
    int i;

    for (i = 0; i < BONUS_W1_ROOM_INSTANCE_COUNT &&
                next_oam < VOLVY_OAM_COUNT; ++i) {
        const BonusW1RoomInstance *instance = &bonus_w1_room_instances[i];
        const StaticAnimGraphic *gfx;
        const GbaSpriteAnim *anim;
        const GbaSpriteFrame *frame;
        const GbaSpritePart *part;
        unsigned int frame_index;
        int gfx_id, x, y;
        int is_collectible_or_breakable =
            instance->kind == BONUS_W1_ROOM_OBJ_ONION ||
            instance->kind == BONUS_W1_ROOM_OBJ_DESTRUCTIBLE_BLOCK ||
            instance->kind == BONUS_W1_ROOM_OBJ_DESTRUCTIBLE_BLOCK_ONEWAY ||
            instance->kind == BONUS_W1_ROOM_OBJ_DESTRUCTIBLE_BLOCK_SWOLVY;

        if (is_collectible_or_breakable && level->removed[i]) continue;
        gfx_id = bonus_static_animation_for_instance(level, instance, i);
        if (gfx_id < 0) continue;

        gfx = &s_static_gfx[gfx_id];
        anim = gfx->animation;
        if (!anim || !anim->frame_count) continue;
        frame_index = (anim_tick / (anim->frame_duration ?
                      anim->frame_duration : 1u)) % anim->frame_count;
        if (instance->kind == BONUS_W1_ROOM_OBJ_SPRINGBOARD) {
            if (level->actor_cooldown[i] && anim->frame_count > 1)
                frame_index = 1u +
                    ((level->actor_cooldown[i] / 4u) % (anim->frame_count - 1u));
            else frame_index = 0;
        }
        if (frame_index >= MAX_STATIC_FRAMES) continue;
        frame = &anim->frames[frame_index];
        if (frame->part_count != 1) continue;
        part = &frame->parts[0];

        if (instance->kind == BONUS_W1_ROOM_OBJ_BUNNY && race) {
            const BonusBunny *bunny = bonus_bunny_for_instance(
                race->bunnies, (unsigned int)i);
            x = (bunny ? fx_to_int(bunny->x) : instance->x / 2) -
                frame->origin_x + part->offset_x - cam_x;
            y = (bunny ? fx_to_int(bunny->y) : instance->y / 2) -
                frame->origin_y + part->offset_y - cam_y;
        } else {
            x = instance->x / 2 - frame->origin_x + part->offset_x - cam_x;
            y = instance->y / 2 - frame->origin_y + part->offset_y - cam_y;
        }
        if (x + part->hw_w <= 0 || x >= 240 ||
            y + part->hw_h <= 0 || y >= 160) continue;

        oam[next_oam].attr0 = (u16)((y & 255) | OBJ_8BPP_BIT |
                                    ((u16)part->shape << 14));
        oam[next_oam].attr1 = (u16)((x & 511) |
                                    ((u16)part->size_code << 14));
        oam[next_oam].attr2 = gfx->frame_tile_base32[frame_index];
        oam[next_oam].dummy = 0;
        ++next_oam;
    }
    return next_oam;
}

void render_draw(const Level *level, const Player *player,
                 const ShrewyBoss *boss, const BonusRace *race, const Camera *camera,
                 GameMode mode, uint32_t anim_tick)
{
    OBJATTR *oam = gba_display_oam_shadow();
    const GbaSpriteAnim *anim;
    const GbaSpriteFrame *frame;
    unsigned int frame_index;
    int base_x, base_y;
    int cam_x = fx_to_int(camera->x);
    int cam_y = fx_to_int(camera->y);
    int part_count, next_oam;
    int facing = player->facing < 0 ? 1 : 0;
    int boss_room = level->boss_room != 0;

    scene_stream_update(camera);
    gba_ui_update(mode, player->onions, level->active_stage,
                  boss_room ? boss->health : 0,
                  level->race_elapsed_frames);
    hide_all_objects(oam);

    if (boss_room && mode != GAME_TITLE) {
        anim = choose_boss_player_anim(player);
    } else if (mode == GAME_PAUSED || mode == GAME_PLAYING ||
               mode == GAME_BONUS || mode == GAME_TITLE || mode == GAME_OVER || mode == GAME_VICTORY) {
        anim = choose_player_anim(player, mode);
    } else {
        return;
    }

    if (!anim || !anim->frame_count) return;
    frame_index = (anim_tick / (anim->frame_duration ? anim->frame_duration : 1u))
                  % anim->frame_count;
    frame = &anim->frames[frame_index];

    if (mode == GAME_TITLE || mode == GAME_OVER || mode == GAME_VICTORY) {
        base_x = 120 - frame->width / 2;
        base_y = 80 - frame->height / 2;
    } else {
        base_x = fx_to_int(player->x) + player->origin_x -
                 frame->origin_x - cam_x;
        base_y = fx_to_int(player->y) + player->origin_y -
                 frame->origin_y - cam_y;
    }

    part_count = frame->part_count;
    if (part_count > MAX_PLAYER_PARTS) part_count = MAX_PLAYER_PARTS;
    draw_frame_parts(frame, oam, 0, base_x, base_y, facing,
                     0);
    s_pending_frame = frame;
    next_oam = part_count;

    if (boss_room && mode != GAME_TITLE) {
        const GbaSpriteAnim *boss_anim = choose_shrewy_anim(boss);
        const GbaSpriteFrame *boss_frame;
        unsigned int boss_frame_index;
        int boss_base_x, boss_base_y;
        int boss_parts;
        if (boss_anim && boss_anim->frame_count) {
            boss_frame_index = (anim_tick / (boss_anim->frame_duration ?
                                boss_anim->frame_duration : 1u)) % boss_anim->frame_count;
            boss_frame = &boss_anim->frames[boss_frame_index];
            boss_base_x = fx_to_int(boss->x) - boss_frame->origin_x - cam_x;
            boss_base_y = fx_to_int(boss->y) - boss_frame->origin_y - cam_y;
            draw_frame_parts(boss_frame, oam, next_oam, boss_base_x, boss_base_y,
                             boss->facing == 0, 128);
            boss_parts = boss_frame->part_count;
            if (boss_parts > MAX_PLAYER_PARTS) boss_parts = MAX_PLAYER_PARTS;
            next_oam += boss_parts;
            s_pending_boss_frame = boss_frame;
        }

        if (boss->rock_active && next_oam < VOLVY_OAM_COUNT) {
            const GbaSpriteAnim *rock_anim = &g_boss_spr_shrewy_rock;
            unsigned int rock_frame_index =
                (anim_tick / (rock_anim->frame_duration ? rock_anim->frame_duration : 1u)) %
                rock_anim->frame_count;
            const GbaSpriteFrame *rock_frame = &rock_anim->frames[rock_frame_index];
            int rock_x = fx_to_int(boss->rock_x) - rock_frame->origin_x - cam_x;
            int rock_y = fx_to_int(boss->rock_y) - rock_frame->origin_y - cam_y;
            draw_frame_parts(rock_frame, oam, next_oam, rock_x, rock_y, 0, 384);
            next_oam += rock_frame->part_count;
            s_pending_rock_frame = rock_frame;
        } else {
            s_pending_rock_frame = 0;
        }
        (void)draw_boss_room_props(level, oam, next_oam,
                                   cam_x, cam_y, anim_tick);
    } else {
        s_pending_boss_frame = 0;
        s_pending_rock_frame = 0;
        draw_world_actors(level, camera, oam, next_oam, anim_tick);
        if (level->bonus_room)
            (void)draw_bonus_actors(level, race, oam, next_oam,
                                    cam_x, cam_y, anim_tick);
    }
}

void render_vblank_commit(Level *level, const Camera *camera)
{
    int cam_x = fx_to_int(camera->x);
    int cam_y = fx_to_int(camera->y);

    (void)level;

    if (s_stage_loaded != s_stage_requested) {
        /* Largest scene: W1 L3 uses 473 8bpp tiles / 30,272 bytes.
         * All three scenes fit in BG char blocks 1+2 (32,768 bytes).
         * BG1 screenblocks 27-28 remain outside this region. */
        if (s_stage_requested == 4) {
            dmaCopy(shrewy_scene_tiles, BG_CHAR_BLOCK_1,
                    SHREWY_SCENE_TILE_COUNT * 64u);
            dmaCopy(shrewy_scene_palette, (void *)BG_COLORS,
                    sizeof(shrewy_scene_palette));
            dmaCopy(g_boss_obj_palette, (void *)OBJ_COLORS,
                    sizeof(g_boss_obj_palette));
            {
                const GbaSpriteAnim *onion = &g_boss_spr_onion;
                u16 base32 = 128;
                unsigned int f;
                s_boss_onion_frame_count = onion->frame_count;
                if (s_boss_onion_frame_count > 16) s_boss_onion_frame_count = 16;
                for (f = 0; f < s_boss_onion_frame_count; ++f) {
                    const GbaSpriteFrame *of = &onion->frames[f];
                    if (of->part_count == 1) {
                        const GbaSpritePart *op = &of->parts[0];
                        if (base32 & 1u) ++base32;
                        s_boss_onion_base32[f] = base32;
                        dmaCopy(op->tiles, OBJ_VRAM + base32 * 32u, op->tile_bytes);
                        base32 = (u16)(base32 + op->tile_bytes / 32u);
                    } else {
                        s_boss_onion_base32[f] = 0;
                    }
                }
            }
        } else if (s_stage_requested == 5) {
            dmaCopy(bonus_w1_composite_tiles, BG_CHAR_BLOCK_1,
                    BONUS_W1_COMPOSITE_TILE_BYTES);
            dmaCopy(bonus_w1_composite_palette, (void *)BG_COLORS,
                    sizeof(bonus_w1_composite_palette));
            dmaCopy(g_player_obj_palette, (void *)OBJ_COLORS,
                    sizeof(g_player_obj_palette));
        } else if (s_stage_requested == 3) {
            dmaCopy(rm_w1_l3_scene_tiles, BG_CHAR_BLOCK_1,
                    RM_W1_L3_SCENE_TILE_COUNT * 64u);
            dmaCopy(rm_w1_l3_scene_palette, (void *)BG_COLORS,
                    sizeof(rm_w1_l3_scene_palette));
            dmaCopy(g_player_obj_palette, (void *)OBJ_COLORS,
                    sizeof(g_player_obj_palette));
        } else if (s_stage_requested == 2) {
            dmaCopy(rm_w1_l2_scene_tiles, BG_CHAR_BLOCK_1,
                    RM_W1_L2_SCENE_TILE_COUNT * 64u);
            dmaCopy(rm_w1_l2_scene_palette, (void *)BG_COLORS,
                    sizeof(rm_w1_l2_scene_palette));
            dmaCopy(g_player_obj_palette, (void *)OBJ_COLORS,
                    sizeof(g_player_obj_palette));
        } else {
            dmaCopy(rm_w1_l1_scene_tiles, BG_CHAR_BLOCK_1,
                    RM_W1_L1_SCENE_TILE_COUNT * 64u);
            dmaCopy(rm_w1_l1_scene_palette, (void *)BG_COLORS,
                    sizeof(rm_w1_l1_scene_palette));
            dmaCopy(g_player_obj_palette, (void *)OBJ_COLORS,
                    sizeof(g_player_obj_palette));
        }
        /* Boss mode overlays onion frames in the world-actor OBJ allocation.
         * Rebuild the normal actor cache when returning to a platform stage. */
        if (s_stage_loaded == 4 && s_stage_requested != 4)
            load_actor_graphics();
        /* Restore the BG0 HUD glyph foreground after changing BG palette. */
        BG_COLORS[15*16+1] = 0x7FFF;
        s_stage_loaded = s_stage_requested;
    }

    if (s_scene_map_dirty) {
        dmaCopy(s_scene_map, BG_MAP_MAIN, sizeof(s_scene_map));
        s_scene_map_dirty = 0;
    }

    if (s_pending_frame && s_pending_frame != s_uploaded_frame) {
        int tile_index = 0;
        int i;
        int count = s_pending_frame->part_count;

        if (count > MAX_PLAYER_PARTS) count = MAX_PLAYER_PARTS;
        for (i = 0; i < count; ++i) {
            const GbaSpritePart *part = &s_pending_frame->parts[i];
            if (tile_index & 1) ++tile_index;
            dmaCopy(part->tiles, OBJ_VRAM + tile_index * 64, part->tile_bytes);
            tile_index += part->tile_bytes / 64;
        }
        s_uploaded_frame = s_pending_frame;
    }

    if (s_pending_boss_frame && s_pending_boss_frame != s_uploaded_boss_frame) {
        int tile_index = 0;
        int i;
        int count = s_pending_boss_frame->part_count;
        if (count > MAX_PLAYER_PARTS) count = MAX_PLAYER_PARTS;
        for (i = 0; i < count; ++i) {
            const GbaSpritePart *part = &s_pending_boss_frame->parts[i];
            if (tile_index & 1) ++tile_index;
            dmaCopy(part->tiles, OBJ_VRAM + (128u + tile_index) * 64u,
                    part->tile_bytes);
            tile_index += part->tile_bytes / 64;
        }
        s_uploaded_boss_frame = s_pending_boss_frame;
    }

    if (s_pending_rock_frame && s_pending_rock_frame != s_uploaded_rock_frame) {
        int tile_index = 0;
        int i;
        int count = s_pending_rock_frame->part_count;
        if (count > MAX_PLAYER_PARTS) count = MAX_PLAYER_PARTS;
        for (i = 0; i < count; ++i) {
            const GbaSpritePart *part = &s_pending_rock_frame->parts[i];
            if (tile_index & 1) ++tile_index;
            dmaCopy(part->tiles, OBJ_VRAM + (384u + tile_index) * 64u,
                    part->tile_bytes);
            tile_index += part->tile_bytes / 64;
        }
        s_uploaded_rock_frame = s_pending_rock_frame;
    }

    /* BG1 scroll is relative to the streamed 64-tile map window. */
    gba_display_set_camera(cam_x - s_scene_start_x * 8,
                           cam_y - s_scene_start_y * 8);
    gba_display_commit_oam();
    gba_ui_vblank_commit();
}
