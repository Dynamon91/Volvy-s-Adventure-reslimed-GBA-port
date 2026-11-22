#include <assert.h>
#include <stdio.h>
#include "core/level.h"
#include "core/player.h"
#include "core/world_interactions.h"
#include "w1_l1_objects.h"
#include "w1_l2_objects.h"
#include "rm_w1_l2_scene.h"

static int find_actor(const Level *level, WorldObjectType kind)
{
    int i;
    for (i=0;i<level_world_object_count(level);++i) {
        const WorldObject *o=level_world_object(level,i);
        if(o->kind==kind) return i;
    }
    return -1;
}

int main(void)
{
    Level level;
    Player player;
    int i, onion_count=0, break_count=0, spring_count=0, squirrel_count=0;
    int first_onion, first_block, first_spring, first_exit;
    int x, y, w, h;
    int found_breakable = 0;
    unsigned int col;

    level_init_world1_stage(&level,2);
    assert(level.world1_active && level.active_stage==2);
    assert(level_world_object_count(&level)==W1_L2_OBJECT_COUNT);
    assert(W1_L2_OBJECT_COUNT==367);
    assert(W1_L2_OBJECT_COUNT < LEVEL_MAX_OBJECTS);
    assert(W1_L2_ONION_COUNT==180);

    for(i=0;i<level_world_object_count(&level);++i) {
        const WorldObject *o=level_world_object(&level,i);
        assert(o != 0);
        if(o->kind==WORLD_ONION) ++onion_count;
        if(o->kind==WORLD_BREAKABLE) ++break_count;
        if(o->kind==WORLD_SPRINGBOARD) ++spring_count;
        if(o->kind==WORLD_SQUIRREL) ++squirrel_count;
    }
    assert(onion_count==180 && break_count==107);
    assert(spring_count==4 && squirrel_count==3);
    assert(level_world_object(&level,-1)==0);
    assert(level_world_object(&level,W1_L2_OBJECT_COUNT)==0);

    first_onion=find_actor(&level,WORLD_ONION);
    first_block=find_actor(&level,WORLD_BREAKABLE);
    first_spring=find_actor(&level,WORLD_SPRINGBOARD);
    first_exit=find_actor(&level,WORLD_EXIT_RIGHT);
    assert(first_onion>=0 && first_block>=0 && first_spring>=0 && first_exit>=0);

    assert(level_world_actor_rect(&level,first_onion,&x,&y,&w,&h));
    col=level_collect_world_onions(&level,x,y,w,h);
    assert(col>=1 && level.removed[first_onion]);
    assert(!level_object_active(&level,first_onion));
    assert(level_collect_world_onions(&level,x,y,w,h)==0);

    assert(level_world_actor_rect(&level,first_block,&x,&y,&w,&h));
    for(i=y/LEVEL_TILE_SIZE;i<(y+h+LEVEL_TILE_SIZE-1)/LEVEL_TILE_SIZE;++i) {
        int tx;
        for(tx=x/LEVEL_TILE_SIZE;tx<(x+w+LEVEL_TILE_SIZE-1)/LEVEL_TILE_SIZE;++tx) {
            if(level_get_tile(&level,tx,i)==TILE_BREAKABLE) {
                assert(level_break_tile(&level,tx,i));
                found_breakable=1;
                break;
            }
        }
        if(found_breakable) break;
    }
    assert(found_breakable);
    assert(!level_object_active(&level,first_block));

    assert(level_world_actor_rect(&level,first_spring,&x,&y,&w,&h));
    player_init(&player,x,y-10);
    player.grounded=0;
    player.vy=fx_from_int(1);
    assert(world_interact_player(&level,&player,y-2,1)&WORLD_INTERACT_SPRING);
    assert(player.vy<0);

    assert(level_world_actor_rect(&level,first_exit,&x,&y,&w,&h));
    assert(level_reached_world_exit(&level,x,y,w,h));

    assert(RM_W1_L2_SCENE_MAP_WIDTH==142);
    assert(RM_W1_L2_SCENE_MAP_HEIGHT==23);
    assert(RM_W1_L2_SCENE_TILE_COUNT==453);

    level_init_world1_stage(&level,1);
    assert(level.active_stage==1);
    assert(level_world_object_count(&level)==W1_L1_OBJECT_COUNT);
    assert(!level.removed[first_onion]);
    level_init_world1_stage(&level,2);
    assert(level_world_object_count(&level)==W1_L2_OBJECT_COUNT);
    assert(level_object_active(&level,first_onion));
    puts("World 1 Level 2 source room, interactions and stage switch tests: PASS");
    return 0;
}
