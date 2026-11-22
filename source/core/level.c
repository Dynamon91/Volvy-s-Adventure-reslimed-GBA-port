#include "core/level.h"

#include <string.h>
#include "w1_l1_objects.h"
#include "w1_l2_objects.h"
#include "w1_l3_objects.h"
#include "shrewy_room.h"
#include "bonus_w1_room.h"
#include "bonus_w1_collision.h"

void level_init_test_room(Level *level)
{
    int x;

    memset(level, 0, sizeof(*level));

    /* Temporary physics test room. Geometry will be replaced by extracted maps. */
    for (x = 0; x < 142; ++x) {
        level->tiles[18][x] = TILE_GROUND;
    }

    /* Short platforms to exercise jump height and camera framing. */
    for (x = 8; x <= 12; ++x) level->tiles[15][x] = TILE_GROUND;
    for (x = 17; x <= 21; ++x) level->tiles[13][x] = TILE_GROUND;
    for (x = 27; x <= 31; ++x) level->tiles[15][x] = TILE_GROUND;
    for (x = 42; x <= 47; ++x) level->tiles[13][x] = TILE_GROUND;

    /* Big Hand interaction targets. */
    level->tiles[15][24] = TILE_BREAKABLE;
    level->tiles[15][25] = TILE_BREAKABLE;
    level->tiles[17][34] = TILE_BREAKABLE;
    level->tiles[18][35] = TILE_BREAKABLE;
    level->tiles[18][36] = TILE_BREAKABLE;

    /* A low rail line: hold R while touching it to attach. */
    for (x = 14; x <= 20; ++x) level->tiles[17][x] = TILE_RAIL;

    /* Onions placed along the route; exact power-up threshold is provisional. */
    level->tiles[17][6] = TILE_ONION;
    level->tiles[14][10] = TILE_ONION;
    level->tiles[12][19] = TILE_ONION;
    level->tiles[14][29] = TILE_ONION;
    level->tiles[12][44] = TILE_ONION;
    level->tiles[16][49] = TILE_ONION;

    /* A simple hazard and a test-room exit marker. */
    level->tiles[17][39] = TILE_HAZARD;
    level->tiles[17][61] = TILE_EXIT;

    level->dirty = 1;
    level->world1_active = 0;
    level->active_stage = 0;
    level->boss_room = 0;
    level->width_tiles = 142;
    level->height_tiles = 32;
}

int level_world_object_count(const Level *level)
{
    if (!level->world1_active || level->boss_room) return 0;
    if (level->active_stage == 3) return W1_L3_OBJECT_COUNT;
    if (level->active_stage == 2) return W1_L2_OBJECT_COUNT;
    return W1_L1_OBJECT_COUNT;
}

const WorldObject *level_world_object(const Level *level, int index)
{
    if (!level->world1_active || level->boss_room || index < 0 ||
        index >= level_world_object_count(level)) return 0;
    if (level->active_stage == 3) return &w1_l3_objects[index];
    if (level->active_stage == 2) return &w1_l2_objects[index];
    return &w1_l1_objects[index];
}

void level_init_world1_stage(Level *level, uint8_t stage)
{
    unsigned int i;
    memset(level, 0, sizeof(*level));
    /* Copy the full 142x32 map as contiguous bytes.  Indexing through
     * tiles[0][i] for i >= 142 would be undefined C array access. */
    const uint8_t *source_collision;
    if (stage < 1 || stage > 3) stage = 1;
    if (stage == 3) source_collision = w1_l3_collision;
    else if (stage == 2) source_collision = w1_l2_collision;
    else source_collision = w1_l1_collision;
    {
        unsigned int row;
        for (row = 0; row < 32; ++row) {
            memcpy(level->tiles[row], source_collision + row * 142u, 142u);
        }
    }
    level->active_stage = stage;
    level->world1_active = 1;
    level->boss_room = 0;
    level->bonus_room = 0;
    level->width_tiles = 142;
    level->height_tiles = 32;
    for (i = 0; i < (unsigned int)level_world_object_count(level); ++i) {
        if (level_world_object(level, (int)i)->kind == WORLD_SQUIRREL)
            level->actor_dir[i] = (i & 1u) ? -1 : 1;
    }
    level->dirty = 1;
}

void level_init_world1(Level *level)
{
    level_init_world1_stage(level, 1);
}

void level_init_shrewy(Level *level)
{
    int x, y;
    memset(level, 0, sizeof(*level));
    for (y = 0; y < SHREWY_ROOM_GRID_HEIGHT; ++y) {
        for (x = 0; x < SHREWY_ROOM_GRID_WIDTH; ++x) {
            level->tiles[y][x] = shrewy_collision[
                y * SHREWY_ROOM_GRID_WIDTH + x
            ];
        }
    }
    level->active_stage = 4;
    level->world1_active = 0;
    level->boss_room = 1;
    level->bonus_room = 0;
    level->race_elapsed_frames = 0;
    level->width_tiles = SHREWY_ROOM_GRID_WIDTH;
    level->height_tiles = SHREWY_ROOM_GRID_HEIGHT;
    level->dirty = 1;
}

void level_init_bonus_race(Level *level)
{
    memset(level, 0, sizeof(*level));
    memcpy(level->tiles, bonus_w1_collision, BONUS_W1_COLLISION_CELLS);
    level->active_stage = 5;
    level->world1_active = 0;
    level->boss_room = 0;
    level->bonus_room = 1;
    level->width_tiles = BONUS_W1_COLLISION_WIDTH;
    level->height_tiles = BONUS_W1_COLLISION_HEIGHT;
    level->race_elapsed_frames = 0;
    level->dirty = 1;
}

int level_width_pixels(const Level *level)
{
    return (int)(level->width_tiles ? level->width_tiles : LEVEL_WIDTH_TILES) * LEVEL_TILE_SIZE;
}

int level_height_pixels(const Level *level)
{
    return (int)(level->height_tiles ? level->height_tiles : LEVEL_HEIGHT_TILES) * LEVEL_TILE_SIZE;
}

static int overlap_rect(int ax, int ay, int aw, int ah,
                        int bx, int by, int bw, int bh)
{
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

int level_object_active(const Level *level, int index)
{
    if (!level->world1_active || index < 0 || index >= level_world_object_count(level)) return 0;
    return !level->removed[index];
}

static int floor_divide_two(int value)
{
    return value >= 0 ? value / 2 : -((1 - value) / 2);
}

int level_collect_world_onions(Level *level, int x, int y, int w, int h)
{
    int count = 0;
    int i;
    if (level->bonus_room) {
        for (i = 0; i < BONUS_W1_ROOM_INSTANCE_COUNT; ++i) {
            const BonusW1RoomInstance *instance = &bonus_w1_room_instances[i];
            int left, top;
            if (instance->kind != BONUS_W1_ROOM_OBJ_ONION || level->removed[i])
                continue;
            /* spr_onion source frame: 13x14, origin (8,8), room scale 1:1.
             * Room pixels are converted to GBA pixels at 2:1. */
            left = floor_divide_two((int)instance->x - 8);
            top = floor_divide_two((int)instance->y - 8);
            if (overlap_rect(x, y, w, h, left, top, 7, 7)) {
                level->removed[i] = 1;
                ++count;
            }
        }
        return count;
    }
    if (level->boss_room) {
        for (i = 0; i < SHREWY_ROOM_PROP_COUNT; ++i) {
            const ShrewyProp *prop = &shrewy_props[i];
            if (prop->kind == SHREWY_PROP_ONION && !level->removed[i] &&
                overlap_rect(x, y, w, h, prop->left, prop->top,
                             prop->width, prop->height)) {
                level->removed[i] = 1;
                ++count;
            }
        }
        return count;
    }
    if (!level->world1_active) return 0;
    for (i = 0; i < level_world_object_count(level); ++i) {
        const WorldObject *obj = level_world_object(level, i);
        if (obj->kind == WORLD_ONION && !level->removed[i] &&
            overlap_rect(x, y, w, h,
                         obj->left, obj->top, obj->width, obj->height)) {
            level->removed[i] = 1;
            ++count;
        }
    }
    return count;
}

int level_reached_world_exit(const Level *level, int x, int y, int w, int h)
{
    int i;
    if (!level->world1_active) return 0;
    for (i = 0; i < level_world_object_count(level); ++i) {
        const WorldObject *obj = level_world_object(level, i);
        if (obj->kind == WORLD_EXIT_RIGHT &&
            overlap_rect(x, y, w, h, obj->left, obj->top,
                         obj->width, obj->height)) return 1;
    }
    return 0;
}


int level_world_actor_offset(const Level *level, int index)
{
    if (!level->world1_active || index < 0 ||
        index >= level_world_object_count(level)) return 0;
    return level->actor_dx[index];
}

/* Source sprite AABBs with runtime x-offset for moving enemy prototypes. */
int level_world_actor_rect(const Level *level, int index,
                           int *left, int *top, int *width, int *height)
{
    const WorldObject *obj;
    if (!level_object_active(level, index)) return 0;
    obj = level_world_object(level, index);
    if (left) *left = obj->left + level->actor_dx[index];
    if (top) *top = obj->top;
    if (width) *width = obj->width;
    if (height) *height = obj->height;
    return 1;
}

/* 1 GBA pixel per 4 frames, turning at edges and mapped walls.
 * This is a deterministic placeholder patrol, not extracted original AI. */
void level_tick_world_actors(Level *level)
{
    int i;
    if (!level->world1_active) return;
    ++level->actor_frame;
    for (i = 0; i < level_world_object_count(level); ++i) {
        const WorldObject *obj = level_world_object(level, i);
        int dir, dx, next_left, foot_y, ahead_x;
        if (level->actor_cooldown[i]) --level->actor_cooldown[i];
        if (obj->kind != WORLD_SQUIRREL || level->removed[i]) continue;
        if ((level->actor_frame & 3u) != 0) continue;
        dir = level->actor_dir[i] ? level->actor_dir[i] : 1;
        dx = level->actor_dx[i];
        if ((dir > 0 && dx >= 12) || (dir < 0 && dx <= -12)) {
            level->actor_dir[i] = (int8_t)(-dir);
            continue;
        }
        next_left = obj->left + dx + dir;
        foot_y = obj->top + obj->height;
        ahead_x = dir > 0 ? next_left + obj->width - 1 : next_left;
        if (level_is_solid(level_get_tile_px(level, ahead_x, obj->top + obj->height / 2)) ||
            !level_is_solid(level_get_tile_px(level, ahead_x, foot_y + 1))) {
            level->actor_dir[i] = (int8_t)(-dir);
        } else {
            level->actor_dx[i] = (int8_t)(dx + dir);
        }
    }
}

uint8_t level_get_tile(const Level *level, int tx, int ty)
{
    int width = level->width_tiles ? level->width_tiles : LEVEL_WIDTH_TILES;
    int height = level->height_tiles ? level->height_tiles : LEVEL_HEIGHT_TILES;
    if (ty < 0) return TILE_EMPTY;
    if (tx < 0 || tx >= width || ty >= height) {
        return TILE_GROUND;
    }
    return level->tiles[ty][tx];
}

uint8_t level_get_tile_px(const Level *level, int px, int py)
{
    int tx = px / LEVEL_TILE_SIZE;
    int ty = py / LEVEL_TILE_SIZE;
    if (px < 0 && px % LEVEL_TILE_SIZE) --tx;
    if (py < 0 && py % LEVEL_TILE_SIZE) --ty;
    return level_get_tile(level, tx, ty);
}

void level_set_tile(Level *level, int tx, int ty, uint8_t tile)
{
    if (tx < 0 || tx >= (level->width_tiles ? level->width_tiles : LEVEL_WIDTH_TILES) ||
        ty < 0 || ty >= (level->height_tiles ? level->height_tiles : LEVEL_HEIGHT_TILES)) {
        return;
    }

    if (level->tiles[ty][tx] != tile) {
        level->tiles[ty][tx] = tile;
        level->dirty = 1;
    }
}

int level_is_solid(uint8_t tile)
{
    return tile == TILE_GROUND ||
           tile == TILE_BREAKABLE ||
           tile == TILE_SWOLVY_BLOCK ||
           tile == TILE_HAZARD;
}

/* Clear a broken source-object footprint, preserving any still-active
 * neighbouring breakable whose rectangular mask shares a coarse 8x8 tile. */
static void clear_breakable_footprint(Level *level, const WorldObject *obj)
{
    int ty, tx, i;
    int left = obj->left / LEVEL_TILE_SIZE;
    int top = obj->top / LEVEL_TILE_SIZE;
    int right = (obj->left + obj->width - 1) / LEVEL_TILE_SIZE;
    int bottom = (obj->top + obj->height - 1) / LEVEL_TILE_SIZE;
    for (ty = top; ty <= bottom; ++ty) {
        for (tx = left; tx <= right; ++tx) {
            int still_occupied = 0;
            if (level_get_tile(level, tx, ty) != TILE_BREAKABLE) continue;
            for (i = 0; i < level_world_object_count(level); ++i) {
                const WorldObject *other = level_world_object(level, i);
                if (level->removed[i] || other->kind != WORLD_BREAKABLE) continue;
                if (overlap_rect(tx * LEVEL_TILE_SIZE, ty * LEVEL_TILE_SIZE,
                                 LEVEL_TILE_SIZE, LEVEL_TILE_SIZE,
                                 other->left, other->top,
                                 other->width, other->height)) {
                    still_occupied = 1;
                    break;
                }
            }
            if (!still_occupied) level_set_tile(level, tx, ty, TILE_EMPTY);
        }
    }
}

static int floor_divide_eight(int value)
{
    return value >= 0 ? value / 8 : -((7 - value) / 8);
}

static int ceil_divide_eight(int value)
{
    return -floor_divide_eight(-value);
}

static int bonus_breakable_bounds(const BonusW1RoomInstance *instance,
                                  int *tx0, int *ty0, int *tx1, int *ty1)
{
    int width, height, origin_x, origin_y;
    int left_source, top_source, right_source, bottom_source;
    int left, top, right, bottom;

    switch (instance->kind) {
    case BONUS_W1_ROOM_OBJ_DESTRUCTIBLE_BLOCK:
    case BONUS_W1_ROOM_OBJ_DESTRUCTIBLE_BLOCK_ONEWAY:
        width = 16; height = 16; origin_x = 8; origin_y = 8;
        break;
    case BONUS_W1_ROOM_OBJ_DESTRUCTIBLE_BLOCK_SWOLVY:
        width = 32; height = 32; origin_x = 16; origin_y = 16;
        break;
    default:
        return 0;
    }

    left_source = (int)instance->x -
        (origin_x * (int)instance->scale_x_q8) / 256;
    top_source = (int)instance->y -
        (origin_y * (int)instance->scale_y_q8) / 256;
    right_source = left_source +
        (width * (int)instance->scale_x_q8) / 256;
    bottom_source = top_source +
        (height * (int)instance->scale_y_q8) / 256;

    left = floor_divide_two(left_source);
    top = floor_divide_two(top_source);
    right = -floor_divide_two(-right_source);
    bottom = -floor_divide_two(-bottom_source);

    *tx0 = floor_divide_eight(left);
    *ty0 = floor_divide_eight(top);
    *tx1 = ceil_divide_eight(right);
    *ty1 = ceil_divide_eight(bottom);
    return 1;
}

int level_break_tile(Level *level, int tx, int ty)
{
    int i, broken = 0;
    if (level_get_tile(level, tx, ty) != TILE_BREAKABLE &&
        level_get_tile(level, tx, ty) != TILE_BREAKABLE_ONEWAY &&
        level_get_tile(level, tx, ty) != TILE_SWOLVY_BLOCK) return 0;
    if (level->bonus_room) {
        unsigned int object_index;
        for (object_index = 0; object_index < BONUS_W1_ROOM_INSTANCE_COUNT;
             ++object_index) {
            const BonusW1RoomInstance *instance =
                &bonus_w1_room_instances[object_index];
            int tx0, ty0, tx1, ty1, x, y;
            if (level->removed[object_index] ||
                !bonus_breakable_bounds(instance, &tx0, &ty0, &tx1, &ty1))
                continue;
            if (tx < tx0 || tx >= tx1 || ty < ty0 || ty >= ty1) continue;
            level->removed[object_index] = 1;
            for (y = ty0; y < ty1; ++y) {
                for (x = tx0; x < tx1; ++x) {
                    uint8_t cell = level_get_tile(level, x, y);
                    if (cell == TILE_BREAKABLE ||
                        cell == TILE_BREAKABLE_ONEWAY ||
                        cell == TILE_SWOLVY_BLOCK)
                        level_set_tile(level, x, y, TILE_EMPTY);
                }
            }
            return 1;
        }
    }
    if (!level->world1_active) {
        level_set_tile(level, tx, ty, TILE_EMPTY);
        return 1;
    }

    /* A source block may cover several coarse tiles when not 8px aligned.
     * Mark every contacted instance first, then clear all exposed cells. */
    for (i = 0; i < level_world_object_count(level); ++i) {
        const WorldObject *obj = level_world_object(level, i);
        if (obj->kind != WORLD_BREAKABLE || level->removed[i]) continue;
        if (overlap_rect(tx * LEVEL_TILE_SIZE, ty * LEVEL_TILE_SIZE,
                         LEVEL_TILE_SIZE, LEVEL_TILE_SIZE,
                         obj->left, obj->top, obj->width, obj->height)) {
            level->removed[i] = 2; /* just destroyed this frame */
            broken = 1;
        }
    }
    if (broken) {
        for (i = 0; i < level_world_object_count(level); ++i) {
            if (level->removed[i] == 2 &&
                level_world_object(level, i)->kind == WORLD_BREAKABLE) {
                clear_breakable_footprint(level, level_world_object(level, i));
                level->removed[i] = 1;
            }
        }
    } else {
        /* Defensive fallback for malformed/incomplete source maps. */
        level_set_tile(level, tx, ty, TILE_EMPTY);
    }
    return 1;
}
