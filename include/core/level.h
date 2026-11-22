#ifndef VOLVY_CORE_LEVEL_H
#define VOLVY_CORE_LEVEL_H

#include <stdint.h>
#include "world_objects.h"

#define LEVEL_WIDTH_TILES  332
#define LEVEL_HEIGHT_TILES 70
#define LEVEL_TILE_SIZE    8
#define LEVEL_TILE_COUNT   (LEVEL_WIDTH_TILES * LEVEL_HEIGHT_TILES)
#define LEVEL_MAX_OBJECTS 1024

typedef enum TileId {
    TILE_EMPTY = 0,
    TILE_GROUND = 1,
    TILE_BREAKABLE = 2,
    TILE_RAIL = 3,
    TILE_ONION = 4,
    TILE_HAZARD = 5,
    TILE_EXIT = 6,
    TILE_ONEWAY = 7,
    TILE_BREAKABLE_ONEWAY = 8,
    TILE_SWOLVY_BLOCK = 9
} TileId;

typedef struct Level {
    uint8_t tiles[LEVEL_HEIGHT_TILES][LEVEL_WIDTH_TILES];
    uint8_t dirty;
    uint8_t world1_active;
    uint8_t active_stage;
    uint8_t boss_room;
    uint8_t bonus_room;
    uint16_t width_tiles;
    uint16_t height_tiles;
    uint8_t removed[LEVEL_MAX_OBJECTS];
    int8_t actor_dx[LEVEL_MAX_OBJECTS];
    int8_t actor_dir[LEVEL_MAX_OBJECTS];
    uint8_t actor_cooldown[LEVEL_MAX_OBJECTS];
    uint16_t actor_frame;
    uint32_t race_elapsed_frames;
} Level;

void level_init_test_room(Level *level);
void level_init_world1(Level *level);
void level_init_world1_stage(Level *level, uint8_t stage);
void level_init_shrewy(Level *level);
void level_init_bonus_race(Level *level);
int level_width_pixels(const Level *level);
int level_height_pixels(const Level *level);
int level_world_object_count(const Level *level);
const WorldObject *level_world_object(const Level *level, int index);
void level_tick_world_actors(Level *level);
int level_world_actor_offset(const Level *level, int index);
int level_world_actor_rect(const Level *level, int index,
                           int *left, int *top, int *width, int *height);
int level_collect_world_onions(Level *level, int x, int y, int width, int height);
int level_reached_world_exit(const Level *level, int x, int y, int width, int height);
int level_object_active(const Level *level, int index);
uint8_t level_get_tile(const Level *level, int tx, int ty);
uint8_t level_get_tile_px(const Level *level, int px, int py);
void level_set_tile(Level *level, int tx, int ty, uint8_t tile);
int level_is_solid(uint8_t tile);
int level_break_tile(Level *level, int tx, int ty);

#endif
