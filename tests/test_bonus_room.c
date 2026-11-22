#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bonus_w1_room.h"

static const BonusW1RoomInstance *find_instance(uint32_t source_id)
{
    unsigned int i;
    for (i = 0; i < BONUS_W1_ROOM_INSTANCE_COUNT; ++i) {
        if (bonus_w1_room_instances[i].source_id == source_id)
            return &bonus_w1_room_instances[i];
    }
    return 0;
}

int main(void)
{
    unsigned int i;
    unsigned int count_sum = 0;
    uint16_t max_tile = 0;
    const BonusW1RoomInstance *start_upper;
    const BonusW1RoomInstance *start_lower;
    const BonusW1RoomInstance *finish;
    const BonusW1RoomInstance *podium;
    const BonusW1RoomInstance *segment_1;
    const BonusW1RoomInstance *negative_scale;

    assert(BONUS_W1_ROOM_SOURCE_WIDTH == 5312);
    assert(BONUS_W1_ROOM_SOURCE_HEIGHT == 1120);
    assert(BONUS_W1_ROOM_MAP_WIDTH == 332);
    assert(BONUS_W1_ROOM_MAP_HEIGHT == 70);
    assert(BONUS_W1_ROOM_TILE_COUNT == 139);
    assert(BONUS_W1_ROOM_TILE_BYTES == 8896);
    assert(BONUS_W1_ROOM_PALETTE_COLORS == 23);
    assert(BONUS_W1_ROOM_INSTANCE_COUNT == 1024);
    assert(BONUS_W1_ROOM_OBJECT_KIND_COUNT == 46);

    for (i = 0; i < BONUS_W1_ROOM_MAP_WIDTH * BONUS_W1_ROOM_MAP_HEIGHT; ++i) {
        assert(bonus_w1_room_map[i] < BONUS_W1_ROOM_TILE_COUNT);
        if (bonus_w1_room_map[i] > max_tile) max_tile = bonus_w1_room_map[i];
    }
    assert(max_tile == BONUS_W1_ROOM_TILE_COUNT - 1);

    for (i = 0; i < BONUS_W1_ROOM_OBJECT_KIND_COUNT; ++i)
        count_sum += bonus_w1_room_object_counts[i];
    assert(count_sum == BONUS_W1_ROOM_INSTANCE_COUNT);
    assert(bonus_w1_room_object_counts[BONUS_W1_ROOM_OBJ_ONION] == 397);
    assert(bonus_w1_room_object_counts[BONUS_W1_ROOM_OBJ_DESTRUCTIBLE_BLOCK] == 193);
    assert(bonus_w1_room_object_counts[BONUS_W1_ROOM_OBJ_SOLID] == 166);
    assert(bonus_w1_room_object_counts[BONUS_W1_ROOM_OBJ_RACE_START] == 2);
    assert(bonus_w1_room_object_counts[BONUS_W1_ROOM_OBJ_RACE_FINISH] == 1);
    assert(bonus_w1_room_object_counts[BONUS_W1_ROOM_OBJ_RACE_PODIUM] == 1);
    assert(bonus_w1_room_object_counts[BONUS_W1_ROOM_OBJ_RACE_CROWD] == 7);
    assert(bonus_w1_room_object_counts[BONUS_W1_ROOM_OBJ_BUNNY] == 2);
    assert(bonus_w1_room_object_counts[BONUS_W1_ROOM_OBJ_BUNNY_GROUND] == 9);
    assert(bonus_w1_room_object_counts[BONUS_W1_ROOM_OBJ_RAIL_DAVE] == 9);
    assert(bonus_w1_room_object_counts[BONUS_W1_ROOM_OBJ_SEG] == 4);

    start_upper = find_instance(101996);
    start_lower = find_instance(101997);
    finish = find_instance(102001);
    podium = find_instance(102002);
    segment_1 = find_instance(101392);
    negative_scale = find_instance(101744);
    assert(start_upper && start_lower && finish && podium && segment_1 && negative_scale);
    assert(start_upper->kind == BONUS_W1_ROOM_OBJ_RACE_START);
    assert(start_upper->x == 440 && start_upper->y == 632);
    assert(start_lower->kind == BONUS_W1_ROOM_OBJ_RACE_START);
    assert(start_lower->x == 440 && start_lower->y == 536);
    assert(finish->kind == BONUS_W1_ROOM_OBJ_RACE_FINISH);
    assert(finish->x == 4984 && finish->y == 840);
    assert(podium->kind == BONUS_W1_ROOM_OBJ_RACE_PODIUM);
    assert(podium->x == 5120 && podium->y == 832);
    assert(segment_1->kind == BONUS_W1_ROOM_OBJ_SEG);
    assert(segment_1->x == 256 && segment_1->y == 496);
    assert(segment_1->scale_y_q8 == 768);
    assert(negative_scale->kind == BONUS_W1_ROOM_OBJ_CROWLY);
    assert(negative_scale->scale_x_q8 == 256);
    assert((negative_scale->flip_flags & 1u) != 0);

    puts("World 1 bonus/race room asset and source-placement tests: PASS");
    return 0;
}
