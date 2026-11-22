#ifndef VOLVY_TEST_WORLD_OBJECTS_H
#define VOLVY_TEST_WORLD_OBJECTS_H

#include <stdint.h>

/* Minimal public-test shape for the generated private world-object header. */
typedef enum WorldObjectKind {
    WORLD_SQUIRREL = 1,
    WORLD_SPRINGBOARD,
    WORLD_ACTION_BLOCK
} WorldObjectKind;

typedef struct WorldObject {
    int kind;
    int left;
    int top;
    int width;
    int height;
} WorldObject;

#endif
