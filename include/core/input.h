#ifndef VOLVY_CORE_INPUT_H
#define VOLVY_CORE_INPUT_H

#include <gba.h>

typedef struct InputState {
    u16 held;
    u16 pressed;
    u16 released;
} InputState;

void input_init(void);
void input_update(void);
const InputState *input_get_state(void);

static inline int input_is_held(u16 key)
{
    return (input_get_state()->held & key) != 0;
}

static inline int input_was_pressed(u16 key)
{
    return (input_get_state()->pressed & key) != 0;
}

static inline int input_was_released(u16 key)
{
    return (input_get_state()->released & key) != 0;
}

#endif
