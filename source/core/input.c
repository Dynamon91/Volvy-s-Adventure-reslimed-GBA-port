#include "core/input.h"

#define VOLVY_KEY_MASK 0x03FFu

static InputState s_input;

void input_init(void)
{
    const u16 current = (u16)(~REG_KEYINPUT) & VOLVY_KEY_MASK;

    s_input.held = current;
    s_input.pressed = 0;
    s_input.released = 0;
}

void input_update(void)
{
    const u16 current = (u16)(~REG_KEYINPUT) & VOLVY_KEY_MASK;
    const u16 previous = s_input.held;

    /* GBA KEYINPUT is active-low: a zero bit means the key is down. */
    s_input.pressed = (u16)(current & (u16)~previous);
    s_input.released = (u16)(previous & (u16)~current);
    s_input.held = current;
}

const InputState *input_get_state(void)
{
    return &s_input;
}
