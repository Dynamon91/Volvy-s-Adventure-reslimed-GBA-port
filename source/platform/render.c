#include <string.h>
#include <gba.h>

#include "platform/render.h"
#include "platform/gba_display.h"
#include "player_sprites.h"
#include "boss_sprites.h"
#include "core/boss_shrewy.h"
#include "core/native_scene.h"
#include "core/static_frame_scheduler.h"
#include "core/sprite_visibility.h"
#include "shrewy_room.h"
#include "w1_l1_objects.h"
#include "bonus_w1_room.h"
#include "w1_l1_native.h"
#include "w1_l2_native.h"
#include "w1_l3_native.h"
#include "bonus_native.h"
#include "shrewy_native.h"
#include "platform/ui.h"

#define BG_MAP_MAIN ((u16 *)0x0600D800)
#define BG_CHAR_BLOCK_1 ((u8 *)0x06004000)
#define OBJ_VRAM ((u8 *)0x06010000)
#define MAX_PLAYER_PARTS 8
#define STATIC_ANIM_COUNT 25
#define MAX_STATIC_FRAMES 16
#define STATIC_OBJ_BASE_32 160 /* reserve 5120B for native animated player frames */
#define OBJ_8BPP_BIT (1u << 13)
#define OBJ_HFLIP_BIT (1u << 12)
#define OBJ_PRIORITY_SHIFT 10u
#define OBJ_GAMEPLAY_PRIORITY (1u << OBJ_PRIORITY_SHIFT)
#define NATIVE_BG_UPLOADS_PER_VBLANK 128u
#define BOSS_ONION_OBJ_BASE32 192u /* 6144..7168, after player (0..5120), before boss (8192+) */
#define STATIC_OBJ_UPLOAD_BUDGET 6144u

static u16 s_scene_map[NATIVE_SCENE_WINDOW_CELLS] ALIGN(4);
static NativeSceneCache s_scene_cache;
static NativeSceneUpload s_scene_uploads[NATIVE_SCENE_CACHE_TILES];
static u16 s_scene_upload_count;
static u16 s_scene_upload_pos;
static int s_scene_loading;
static int s_bg_hard_jump;
static NativeScenePresentation s_scene_presentation;
static int s_render_cam_x;
static int s_render_cam_y;
static int s_render_cam_initialized;
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
static int s_actor_cache_ready = 1;
static int s_actor_reload_anim;
static int s_actor_reload_part;

static const NativeSceneAsset *scene_asset_source(void)
{
    if (s_stage_requested == 5) return &g_bonus_native_scene;
    if (s_stage_requested == 4) return &g_shrewy_native_scene;
    if (s_stage_requested == 3) return &g_w1_l3_native_scene;
    if (s_stage_requested == 2) return &g_w1_l2_native_scene;
    return &g_w1_l1_native_scene;
}

static void reset_native_scene_cache(void)
{
    native_scene_cache_reset(&s_scene_cache);
    s_scene_upload_count = 0;
    s_scene_upload_pos = 0;
    s_scene_start_x = -1;
    s_scene_start_y = -1;
    s_scene_map_dirty = 1;
    s_scene_loading = 1;
    s_bg_hard_jump = 0;
    native_scene_presentation_reset(&s_scene_presentation);
    s_render_cam_initialized = 0;
}

void render_select_stage(uint8_t stage)
{
    if (stage < 1 || stage > 3) stage = 1;
    s_stage_requested = stage;
    reset_native_scene_cache();
    s_pending_frame = 0;
    s_uploaded_frame = 0;
    s_pending_boss_frame = 0;
    s_pending_rock_frame = 0;
}

void render_select_boss(void)
{
    s_stage_requested = 4;
    reset_native_scene_cache();
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
    reset_native_scene_cache();
    s_pending_frame = 0;
    s_uploaded_frame = 0;
    s_pending_boss_frame = 0;
    s_uploaded_boss_frame = 0;
    s_pending_rock_frame = 0;
    s_uploaded_rock_frame = 0;
}

/* Static actors share per-animation 8bpp OBJ slots; animation frames are
 * streamed into those slots during VBlank. This is not a full-frame cache. */
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
    STATIC_BUMPER_TRI,
    STATIC_MOVING_PLATFORM,
    STATIC_MOVING_PLATFORM_WIDE,
    STATIC_CLOUD_PLATFORM,
    STATIC_HIVEFLY,
    STATIC_CROWLY,
    STATIC_VINECHOMP,
    STATIC_SPIKES,
    STATIC_CANNON,
    STATIC_BUTTON,
    STATIC_RAIL,
    STATIC_BKG_SPACE_ONION,
    STATIC_WORM,
    STATIC_PLANTS,
    STATIC_LEVEL_EXIT_HOLE
} StaticAnimId;

typedef struct StaticAnimGraphic {
    const GbaSpriteAnim *animation;
    u16 part_tile_base32[MAX_PLAYER_PARTS];
    u16 part_capacity32[MAX_PLAYER_PARTS];
    u8 max_parts;
    u8 loaded_frame;
    u8 pending_frame;
    u8 pending;
} StaticAnimGraphic;

static StaticAnimGraphic s_static_gfx[STATIC_ANIM_COUNT];
static StaticFrameScheduler s_static_frame_scheduler;

static void load_actor_graphics(void)
{
    const GbaSpriteAnim *const animations[STATIC_ANIM_COUNT] = {
        &g_spr_onion, &g_spr_destructible_block, &g_spr_action_block,
        &g_spr_action_block_spent, &g_spr_squirrel_walk,
        &g_spr_squirrel_idle, &g_spr_springboard,
        &g_spr_destructible_block_swolvy, &g_spr_bunny_hop,
        &g_spr_bumper_sq, &g_spr_bumper_tri,
        &g_spr_moving_platform, &g_spr_moving_platform_wide,
        &g_spr_cloud_platform, &g_spr_hivefly, &g_spr_crowly_fly,
        &g_spr_vinechomp_base, &g_spr_spikes_dave, &g_spr_cannon,
        &g_spr_button_off, &g_spr_rail_slice,
        &g_spr_bkg_space_onion, &g_spr_worm, &g_spr_plants,
        &g_spr_level_exit_hole
    };
    u16 base = STATIC_OBJ_BASE_32;
    int i, f, p;

    for (i = 0; i < STATIC_ANIM_COUNT; ++i) {
        const GbaSpriteAnim *anim = animations[i];
        unsigned int max_parts = 0;
        s_static_gfx[i].animation = anim;
        s_static_gfx[i].pending = 0;
        s_static_gfx[i].loaded_frame = 0;
        s_static_gfx[i].pending_frame = 0;

        for (f = 0; f < anim->frame_count && f < MAX_STATIC_FRAMES; ++f) {
            if (anim->frames[f].part_count > max_parts)
                max_parts = anim->frames[f].part_count;
        }
        if (max_parts > MAX_PLAYER_PARTS) max_parts = MAX_PLAYER_PARTS;
        s_static_gfx[i].max_parts = (u8)max_parts;

        /* Reserve one native-size frame slot per animation part. Animation
           frames are streamed into these slots instead of caching every frame. */
        for (p = 0; p < (int)max_parts; ++p) {
            u16 max_bytes = 0;
            for (f = 0; f < anim->frame_count && f < MAX_STATIC_FRAMES; ++f) {
                const GbaSpriteFrame *frame = &anim->frames[f];
                if ((unsigned int)p < frame->part_count &&
                    frame->parts[p].tile_bytes > max_bytes)
                    max_bytes = frame->parts[p].tile_bytes;
            }
            if (base & 1u) ++base;
            s_static_gfx[i].part_tile_base32[p] = base;
            s_static_gfx[i].part_capacity32[p] = (u16)(max_bytes / 32u);
            base = (u16)(base + max_bytes / 32u);
        }

        if (anim->frame_count && anim->frames[0].part_count <= MAX_PLAYER_PARTS) {
            const GbaSpriteFrame *first = &anim->frames[0];
            for (p = 0; p < first->part_count; ++p) {
                const GbaSpritePart *part = &first->parts[p];
                dmaCopy(part->tiles,
                        OBJ_VRAM + (unsigned int)s_static_gfx[i].part_tile_base32[p] * 32u,
                        part->tile_bytes);
            }
        }
    }
    s_actor_cache_ready = 1;
    s_actor_reload_anim = 0;
    s_actor_reload_part = 0;
}

static void start_actor_graphics_reload(void)
{
    s_actor_cache_ready = 0;
    s_actor_reload_anim = 0;
    s_actor_reload_part = 0;
}

static void upload_actor_graphics_batch(void)
{
    unsigned int budget = 4096u;
    while (s_actor_reload_anim < STATIC_ANIM_COUNT && budget) {
        StaticAnimGraphic *gfx = &s_static_gfx[s_actor_reload_anim];
        const GbaSpriteAnim *anim = gfx->animation;
        const GbaSpriteFrame *first;
        if (!anim || !anim->frame_count) {
            ++s_actor_reload_anim;
            s_actor_reload_part = 0;
            continue;
        }
        first = &anim->frames[0];
        if (s_actor_reload_part >= first->part_count ||
            s_actor_reload_part >= gfx->max_parts) {
            ++s_actor_reload_anim;
            s_actor_reload_part = 0;
            continue;
        }
        {
            unsigned int part_index = (unsigned int)s_actor_reload_part;
            const GbaSpritePart *part = &first->parts[part_index];
            if (part->tile_bytes > budget && budget < 4096u) break;
            dmaCopy(part->tiles,
                    OBJ_VRAM + (unsigned int)gfx->part_tile_base32[part_index] * 32u,
                    part->tile_bytes);
            budget -= part->tile_bytes;
            ++s_actor_reload_part;
        }
    }
    if (s_actor_reload_anim >= STATIC_ANIM_COUNT) {
        int i;
        for (i = 0; i < STATIC_ANIM_COUNT; ++i) {
            s_static_gfx[i].loaded_frame = 0;
            s_static_gfx[i].pending = 0;
        }
        s_actor_cache_ready = 1;
    }
}

static unsigned int request_static_frame(int gfx_id, unsigned int frame_index)
{
    StaticAnimGraphic *gfx;
    const GbaSpriteFrame *wanted;
    unsigned int bytes = 0;
    unsigned int p;
    unsigned int chosen;
    if (gfx_id < 0 || gfx_id >= STATIC_ANIM_COUNT) return 0;
    gfx = &s_static_gfx[gfx_id];
    if (!gfx->animation || !gfx->animation->frame_count) return 0;
    if (frame_index >= gfx->animation->frame_count) frame_index = 0;
    wanted = &gfx->animation->frames[frame_index];
    for (p = 0; p < wanted->part_count && p < gfx->max_parts; ++p)
        bytes += wanted->parts[p].tile_bytes;
    chosen = static_frame_scheduler_choose(&s_static_frame_scheduler,
                                           (unsigned int)gfx_id, frame_index,
                                           gfx->loaded_frame, bytes);
    if (chosen != gfx->loaded_frame) {
        gfx->pending_frame = (u8)chosen;
        gfx->pending = 1;
    }
    return chosen;
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
    case WORLD_HIVEFLY: return STATIC_HIVEFLY;
    case WORLD_PLANT: return STATIC_PLANTS;
    case WORLD_WORM: return STATIC_WORM;
    case WORLD_LEVEL_EXIT_HOLE: return STATIC_LEVEL_EXIT_HOLE;
    default: return -1;
    }
}

/* Culling before frame scheduling is important: off-screen instances used to
 * claim the same VBlank upload allowance as visible actors. Test the desired
 * and the resident frame, because their part positions may differ. */
static int static_frame_visible(const GbaSpriteFrame *frame,
                                int base_x, int base_y, int facing_left)
{
    unsigned int i;
    for (i = 0; i < frame->part_count && i < MAX_PLAYER_PARTS; ++i) {
        const GbaSpritePart *part = &frame->parts[i];
        int x = facing_left
            ? base_x + frame->width - part->offset_x - part->hw_w
            : base_x + part->offset_x;
        int y = base_y + part->offset_y;
        if (sprite_part_intersects_viewport(x, y, part->hw_w, part->hw_h))
            return 1;
    }
    return 0;
}

static int static_actor_visible(const StaticAnimGraphic *gfx,
                                const GbaSpriteFrame *requested,
                                int world_x, int world_y, int cam_x, int cam_y,
                                int facing_left)
{
    const GbaSpriteFrame *loaded;
    if (static_frame_visible(requested, world_x - requested->origin_x - cam_x,
                             world_y - requested->origin_y - cam_y, facing_left))
        return 1;
    if (gfx->loaded_frame >= gfx->animation->frame_count) return 0;
    loaded = &gfx->animation->frames[gfx->loaded_frame];
    return static_frame_visible(loaded, world_x - loaded->origin_x - cam_x,
                                world_y - loaded->origin_y - cam_y, facing_left);
}

static int draw_static_parts(const StaticAnimGraphic *gfx,
                             const GbaSpriteFrame *frame,
                             OBJATTR *oam, int next_oam,
                             int base_x, int base_y, int facing_left)
{
    unsigned int part_index;
    unsigned int count = frame->part_count;
    if (count > gfx->max_parts) count = gfx->max_parts;
    if (count > MAX_PLAYER_PARTS) count = MAX_PLAYER_PARTS;
    for (part_index = 0; part_index < count && next_oam < VOLVY_OAM_COUNT;
         ++part_index) {
        const GbaSpritePart *part = &frame->parts[part_index];
        int x = facing_left
            ? base_x + frame->width - part->offset_x - part->hw_w
            : base_x + part->offset_x;
        int y = base_y + part->offset_y;
        if (!sprite_part_intersects_viewport(x, y, part->hw_w, part->hw_h))
            continue;
        oam[next_oam].attr0 = (u16)((y & 255) | OBJ_8BPP_BIT |
                                    ((u16)part->shape << 14));
        oam[next_oam].attr1 = (u16)((x & 511) |
                                    (facing_left ? OBJ_HFLIP_BIT : 0) |
                                    ((u16)part->size_code << 14));
        oam[next_oam].attr2 = (u16)(gfx->part_tile_base32[part_index] | OBJ_GAMEPLAY_PRIORITY);
        oam[next_oam].dummy = 0;
        ++next_oam;
    }
    return next_oam;
}

static int draw_world_actors(const Level *level, OBJATTR *oam, int next_oam,
                             uint32_t anim_tick, int cam_x, int cam_y)
{
    int i;
    if (!s_actor_cache_ready || !level->world1_active) return next_oam;
    for (i = 0; i < level_world_object_count(level) && next_oam < VOLVY_OAM_COUNT; ++i) {
        const WorldObject *object = level_world_object(level, i);
        const StaticAnimGraphic *gfx;
        const GbaSpriteAnim *anim;
        const GbaSpriteFrame *frame;
        unsigned int frame_index;
        int gfx_id, base_x, base_y, facing_left, world_x, world_y;
        if (!level_object_active(level, i)) continue;
        gfx_id = static_animation_for_actor(level, i);
        if (gfx_id < 0) continue;
        gfx = &s_static_gfx[gfx_id];
        anim = gfx->animation;
        if (!anim->frame_count) continue;
        frame_index = (anim_tick / (anim->frame_duration ?
                      anim->frame_duration : 1u)) % anim->frame_count;
        if (frame_index >= MAX_STATIC_FRAMES) continue;
        world_x = (object->x + level_world_actor_offset(level, i)) * 2;
        world_y = object->y * 2;
        facing_left = object->kind == WORLD_SQUIRREL &&
                      level->actor_dir[i] < 0;
        frame = &anim->frames[frame_index];
        if (!static_actor_visible(gfx, frame, world_x, world_y,
                                  cam_x, cam_y, facing_left))
            continue;
        /* Reserve OBJ bandwidth only for an animation with visible parts. */
        frame_index = request_static_frame(gfx_id, frame_index);
        frame = &anim->frames[frame_index];
        base_x = world_x - frame->origin_x - cam_x;
        base_y = world_y - frame->origin_y - cam_y;
        next_oam = draw_static_parts(gfx, frame, oam, next_oam,
                                     base_x, base_y, facing_left);
    }
    return next_oam;
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

static void scene_stream_update(int cam_x, int cam_y)
{
    const NativeSceneAsset *scene = scene_asset_source();
    NativeSceneUpload uploads[NATIVE_SCENE_CACHE_TILES];
    uint16_t upload_count = 0;
    int origin_x, origin_y;

    /* Never replace a staged map before its tile data and screen block
       have actually been committed during VBlank. */
    if ((s_scene_start_x >= 0 && s_scene_map_dirty) ||
        s_scene_upload_pos < s_scene_upload_count) return;

    native_scene_window_origin(cam_x, cam_y,
                               scene->map_width, scene->map_height,
                               &origin_x, &origin_y);
    if (origin_x == s_scene_start_x && origin_y == s_scene_start_y &&
        !s_scene_map_dirty) return;

    if (!native_scene_cache_build(scene, origin_x, origin_y, &s_scene_cache,
                                  s_scene_map, uploads, &upload_count))
        return;

    if (upload_count)
        memcpy(s_scene_uploads, uploads,
               (unsigned int)upload_count * sizeof(NativeSceneUpload));
    s_scene_upload_count = upload_count;
    s_scene_upload_pos = 0;
    s_scene_start_x = origin_x;
    s_scene_start_y = origin_y;
    s_scene_map_dirty = 1;
}

static int clamp_camera(int value, int world_size, int view_size)
{
    int maximum = world_size - view_size;
    if (maximum < 0) maximum = 0;
    if (value < 0) return 0;
    if (value > maximum) return maximum;
    return value;
}

static void render_compute_camera(const Level *level, const Player *player,
                                  const Camera *camera, GameMode mode,
                                  int *out_x, int *out_y)
{
    int target_x, target_y;
    const NativeSceneAsset *scene = scene_asset_source();

    if (level->boss_room) {
        *out_x = fx_to_int(camera->x);
        *out_y = fx_to_int(camera->y);
        return;
    }
    if (mode == GAME_TITLE) {
        *out_x = 0;
        *out_y = 0;
        s_render_cam_x = 0;
        s_render_cam_y = 0;
        s_render_cam_initialized = 1;
        return;
    }

    /* Physics and collision remain in the 2:1 gameplay coordinate system.
       The renderer maps those world positions back to original source pixels. */
    target_x = (fx_to_int(player->x) + player->origin_x) * 2 - 120;
    target_y = (fx_to_int(player->y) + player->origin_y) * 2 - 80;
    target_x = clamp_camera(target_x, scene->map_width * 8, 240);
    target_y = clamp_camera(target_y, scene->map_height * 8, 160);

    if (!s_render_cam_initialized) {
        s_render_cam_x = target_x;
        s_render_cam_y = target_y;
        s_render_cam_initialized = 1;
    } else {
        int dx = target_x - s_render_cam_x;
        int dy = target_y - s_render_cam_y;
        if (dx > -2 && dx < 2) s_render_cam_x = target_x;
        else s_render_cam_x += dx / 2;
        if (dy > -2 && dy < 2) s_render_cam_y = target_y;
        else s_render_cam_y += dy / 2;
    }
    *out_x = s_render_cam_x;
    *out_y = s_render_cam_y;
}

void render_init(Level *level)
{
    const NativeSceneAsset *scene = &g_w1_l1_native_scene;
    (void)level;

    /* Native 4bpp BG scene; the first window is streamed into BG VRAM. */
    SetMode(MODE_0);
    BGCTRL[1] = BG_SIZE_0 | CHAR_BASE(1) | SCREEN_BASE(27) | 1;
    dmaCopy(scene->palette, (void *)BG_COLORS, sizeof(u16) * 256u);
    dmaCopy(g_player_obj_palette, (void *)OBJ_COLORS,
            sizeof(g_player_obj_palette));
    load_actor_graphics();
    gba_ui_init();

    s_stage_requested = 1;
    s_stage_loaded = 1;
    reset_native_scene_cache();
    s_pending_frame = 0;
    s_uploaded_frame = 0;
    s_pending_boss_frame = 0;
    s_uploaded_boss_frame = 0;
    s_pending_rock_frame = 0;
    s_uploaded_rock_frame = 0;

    /* BG1 remains disabled until the first 32x32 native tile window is ready. */
    SetMode(MODE_0 | BG0_ON | OBJ_ON | OBJ_1D_MAP);
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
                             int facing_left, int base_tile64, unsigned int priority)
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
        oam[first_oam + i].attr2 = (u16)(((base_tile64 + tile_index) * 2) |
                                                ((priority & 3u) << OBJ_PRIORITY_SHIFT));
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
        oam[next_oam].attr2 = (u16)(s_boss_onion_base32[frame_index] |
                                             OBJ_GAMEPLAY_PRIORITY);
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
    case BONUS_W1_ROOM_OBJ_MOVING_PLATFORM:
        return instance->scale_x_q8 > 300u ?
            STATIC_MOVING_PLATFORM_WIDE : STATIC_MOVING_PLATFORM;
    case BONUS_W1_ROOM_OBJ_CLOUD_PLATFORM:
        return STATIC_CLOUD_PLATFORM;
    case BONUS_W1_ROOM_OBJ_ACTION_BLOCK:
        return level->actor_cooldown[index] ? STATIC_ACTION_SPENT : STATIC_ACTION;
    case BONUS_W1_ROOM_OBJ_SQUIRREL:
        return STATIC_SQUIRREL_WALK;
    case BONUS_W1_ROOM_OBJ_SPRINGBOARD:
        return STATIC_SPRING;
    case BONUS_W1_ROOM_OBJ_HIVEFLY:
        return STATIC_HIVEFLY;
    case BONUS_W1_ROOM_OBJ_CROWLY:
        return STATIC_CROWLY;
    case BONUS_W1_ROOM_OBJ_VINECHOMP:
        return STATIC_VINECHOMP;
    case BONUS_W1_ROOM_OBJ_SPIKES_DAVE:
        return STATIC_SPIKES;
    case BONUS_W1_ROOM_OBJ_CANNON:
        return STATIC_CANNON;
    case BONUS_W1_ROOM_OBJ_BUTTON:
        return STATIC_BUTTON;
    /* Source spr_hole is a uniform opaque green 16x16 marker, not visible
       level art. Keep its room placement for logic/audits, but never allocate
       or submit it as an OBJ. */
    case BONUS_W1_ROOM_OBJ_RAIL_DAVE:
        return STATIC_RAIL;
    case BONUS_W1_ROOM_OBJ_BKG_SPACE_ONION:
        return STATIC_BKG_SPACE_ONION;
    default:
        return -1;
    }
}

static int draw_bonus_actors(const Level *level, const BonusRace *race,
                             OBJATTR *oam, int next_oam,
                             int cam_x, int cam_y, uint32_t anim_tick)
{
    int i;
    if (!s_actor_cache_ready) return next_oam;
    for (i = 0; i < BONUS_W1_ROOM_INSTANCE_COUNT &&
                next_oam < VOLVY_OAM_COUNT; ++i) {
        const BonusW1RoomInstance *instance = &bonus_w1_room_instances[i];
        const StaticAnimGraphic *gfx;
        const GbaSpriteAnim *anim;
        const GbaSpriteFrame *frame;
        unsigned int frame_index;
        int gfx_id, base_x, base_y, world_x, world_y, facing_left = 0;

        if (level->removed[i] &&
            (instance->kind == BONUS_W1_ROOM_OBJ_ONION ||
             instance->kind == BONUS_W1_ROOM_OBJ_DESTRUCTIBLE_BLOCK ||
             instance->kind == BONUS_W1_ROOM_OBJ_DESTRUCTIBLE_BLOCK_ONEWAY ||
             instance->kind == BONUS_W1_ROOM_OBJ_DESTRUCTIBLE_BLOCK_SWOLVY))
            continue;

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
        if (instance->kind == BONUS_W1_ROOM_OBJ_MOVING_PLATFORM && race) {
            const BonusMovingPlatform *platform = bonus_platform_for_instance(
                race->platforms, (unsigned int)i);
            world_x = platform ? fx_to_int(platform->x) * 2 : (int)instance->x;
            world_y = platform ? fx_to_int(platform->y) * 2 : (int)instance->y;
        } else if (instance->kind == BONUS_W1_ROOM_OBJ_BUNNY && race) {
            const BonusBunny *bunny = bonus_bunny_for_instance(
                race->bunnies, (unsigned int)i);
            world_x = bunny ? fx_to_int(bunny->x) * 2 : (int)instance->x;
            world_y = bunny ? fx_to_int(bunny->y) * 2 : (int)instance->y;
            facing_left = bunny && bunny->facing < 0;
        } else {
            world_x = (int)instance->x;
            world_y = (int)instance->y;
        }
        frame = &anim->frames[frame_index];
        if (!static_actor_visible(gfx, frame, world_x, world_y,
                                  cam_x, cam_y, facing_left))
            continue;
        /* Both the bonus actors and World 1 actors use the same VBlank budget. */
        frame_index = request_static_frame(gfx_id, frame_index);
        frame = &anim->frames[frame_index];
        base_x = world_x - frame->origin_x - cam_x;
        base_y = world_y - frame->origin_y - cam_y;
        next_oam = draw_static_parts(gfx, frame, oam, next_oam,
                                     base_x, base_y, facing_left);
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
    int cam_x, cam_y;
    int part_count, next_oam;
    int facing = player->facing < 0 ? 1 : 0;
    int boss_room = level->boss_room != 0;
    int player_scale = boss_room ? 1 : 2;

    render_compute_camera(level, player, camera, mode, &cam_x, &cam_y);
    s_render_cam_x = cam_x;
    s_render_cam_y = cam_y;
    scene_stream_update(cam_x, cam_y);
    {
        int desired_x = cam_x;
        int desired_y = cam_y;
        int requires_blackout = native_scene_presentation_camera(
            &s_scene_presentation, desired_x, desired_y,
            s_scene_start_x >= 0 ? s_scene_start_x : 0,
            s_scene_start_y >= 0 ? s_scene_start_y : 0,
            &cam_x, &cam_y);
        /* A distant warp must not expose a new BG window at the old
           scroll origin. The regular camera follow remains clamped to
           the last committed 256x256 tile window while uploads progress. */
        if (requires_blackout && s_scene_map_dirty) s_bg_hard_jump = 1;
        if (s_bg_hard_jump && !s_scene_map_dirty) s_bg_hard_jump = 0;
        s_render_cam_x = cam_x;
        s_render_cam_y = cam_y;
    }

    gba_ui_update(mode, player->onions, level->active_stage,
                  boss_room ? boss->health : 0,
                  level->race_elapsed_frames);
    /* A single DMA budget is shared by all visible animation sets. Reserve
       uploads before filling OAM, rather than drawing previous-frame geometry
       above tiles that will be replaced in this same VBlank. */
    static_frame_scheduler_begin(&s_static_frame_scheduler,
                                 STATIC_OBJ_UPLOAD_BUDGET);
    {
        unsigned int a;
        for (a = 0; a < STATIC_ANIM_COUNT; ++a)
            s_static_gfx[a].pending = 0;
    }
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
        base_x = (fx_to_int(player->x) + player->origin_x) * player_scale -
                 frame->origin_x - cam_x;
        base_y = (fx_to_int(player->y) + player->origin_y) * player_scale -
                 frame->origin_y - cam_y;
    }

    part_count = frame->part_count;
    if (part_count > MAX_PLAYER_PARTS) part_count = MAX_PLAYER_PARTS;
    draw_frame_parts(frame, oam, 0, base_x, base_y, facing, 0,
                     mode == GAME_TITLE ? 0u : 1u);
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
                             boss->facing == 0, 128, 1u);
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
            draw_frame_parts(rock_frame, oam, next_oam, rock_x, rock_y, 0, 384, 1u);
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
        next_oam = draw_world_actors(level, oam, next_oam, anim_tick,
                                     cam_x, cam_y);
        if (level->bonus_room)
            (void)draw_bonus_actors(level, race, oam, next_oam,
                                    cam_x, cam_y, anim_tick);
    }
}

void render_vblank_commit(Level *level, const Camera *camera)
{
    const NativeSceneAsset *scene = scene_asset_source();
    (void)level;
    (void)camera;

    if (s_stage_loaded != s_stage_requested) {
        /* All scenes use native-resolution 4bpp tiles and per-tile palette
           banks. The 32x32 tile cache is rebuilt incrementally below. */
        SetMode(MODE_0 | BG0_ON | OBJ_ON | OBJ_1D_MAP);
        dmaCopy(scene->palette, (void *)BG_COLORS, sizeof(u16) * 256u);
        if (s_stage_requested == 4) {
            dmaCopy(g_boss_obj_palette, (void *)OBJ_COLORS,
                    sizeof(g_boss_obj_palette));
            {
                const GbaSpriteAnim *onion = &g_boss_spr_onion;
                u16 base32 = BOSS_ONION_OBJ_BASE32;
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
        } else {
            dmaCopy(g_player_obj_palette, (void *)OBJ_COLORS,
                    sizeof(g_player_obj_palette));
            /* The boss package overwrites the normal actor cache. Rebuild it
               when returning to a platform stage. */
            if (s_stage_loaded == 4)
                start_actor_graphics_reload();
        }
        BG_COLORS[15 * 16 + 1] = 0x7FFF;
        s_stage_loaded = s_stage_requested;
        s_scene_loading = 1;
    }

    /* A distant camera warp may reuse tiles that appear under the previous
       screen map. Blank BG1 before writing any of them, even when the new
       window needs zero additional tile uploads (all tiles are cached). */
    if (s_bg_hard_jump) {
        SetMode(MODE_0 | BG0_ON | OBJ_ON | OBJ_1D_MAP);
        s_scene_loading = 1;
    }

    if (s_scene_upload_pos < s_scene_upload_count) {
        unsigned int budget = NATIVE_BG_UPLOADS_PER_VBLANK;
        unsigned int remaining = s_scene_upload_count - s_scene_upload_pos;
        unsigned int count = remaining < budget ? remaining : budget;
        unsigned int i;

        if (s_scene_upload_count > NATIVE_BG_UPLOADS_PER_VBLANK ||
            s_bg_hard_jump)
            s_scene_loading = 1;
        if (s_scene_loading)
            SetMode(MODE_0 | BG0_ON | OBJ_ON | OBJ_1D_MAP);

        for (i = 0; i < count; ++i) {
            const NativeSceneUpload *upload =
                &s_scene_uploads[s_scene_upload_pos + i];
            dmaCopy(scene->tiles + (unsigned int)upload->source_tile * 32u,
                    BG_CHAR_BLOCK_1 + (unsigned int)upload->cache_slot * 32u,
                    32u);
        }
        s_scene_upload_pos = (u16)(s_scene_upload_pos + count);
    }

    if (s_scene_upload_pos >= s_scene_upload_count && s_scene_map_dirty) {
        dmaCopy(s_scene_map, BG_MAP_MAIN, sizeof(s_scene_map));
        native_scene_presentation_commit(&s_scene_presentation,
                                         s_scene_start_x, s_scene_start_y);
        s_scene_map_dirty = 0;
        s_scene_upload_count = 0;
        s_scene_upload_pos = 0;
        if (s_scene_loading && !s_bg_hard_jump) {
            SetMode(MODE_0 | BG0_ON | BG1_ON | OBJ_ON | OBJ_1D_MAP);
            s_scene_loading = 0;
        }
    }

    if (!s_actor_cache_ready)
        upload_actor_graphics_batch();

    if (s_actor_cache_ready) {
    {
        unsigned int budget = STATIC_OBJ_UPLOAD_BUDGET;
        int gfx_id;
        for (gfx_id = 0; gfx_id < STATIC_ANIM_COUNT; ++gfx_id) {
            StaticAnimGraphic *gfx = &s_static_gfx[gfx_id];
            const GbaSpriteFrame *frame;
            unsigned int bytes = 0;
            unsigned int part_index;
            if (!gfx->pending || !gfx->animation ||
                gfx->pending_frame >= gfx->animation->frame_count)
                continue;
            frame = &gfx->animation->frames[gfx->pending_frame];
            for (part_index = 0; part_index < frame->part_count &&
                                 part_index < gfx->max_parts; ++part_index)
                bytes += frame->parts[part_index].tile_bytes;
            if (bytes > budget) continue;
            for (part_index = 0; part_index < frame->part_count &&
                                 part_index < gfx->max_parts; ++part_index) {
                const GbaSpritePart *part = &frame->parts[part_index];
                dmaCopy(part->tiles,
                        OBJ_VRAM + (unsigned int)gfx->part_tile_base32[part_index] * 32u,
                        part->tile_bytes);
            }
            gfx->loaded_frame = gfx->pending_frame;
            gfx->pending = 0;
            budget -= bytes;
        }
    }
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

    /* Scroll is derived from the map window actually visible in BG VRAM,
       not from the newly staged origin. This prevents the 8px camera jump
       previously seen when a map window changed before VBlank commit. */
    if (s_scene_presentation.has_visible_map) {
        int relative_x = s_render_cam_x -
                         s_scene_presentation.visible_start_x * 8;
        int relative_y = s_render_cam_y -
                         s_scene_presentation.visible_start_y * 8;
        if (relative_x >= 0 && relative_x <= 16 &&
            relative_y >= 0 && relative_y <= 96 &&
            !s_scene_map_dirty && !s_bg_hard_jump && s_scene_loading) {
            SetMode(MODE_0 | BG0_ON | BG1_ON | OBJ_ON | OBJ_1D_MAP);
            s_scene_loading = 0;
        }
        gba_display_set_camera(relative_x, relative_y);
    }
    gba_display_commit_oam();
    gba_ui_vblank_commit();
}
