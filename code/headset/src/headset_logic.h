#ifndef HEADSET_LOGIC_H
#define HEADSET_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

enum headset_event {
    HEADSET_MODE_CHANGED = 1U << 0,
    HEADSET_PAIR_PRESSED = 1U << 1,
};

struct headset_input {
    uint32_t changed_ms;
    bool initialized;
    bool candidate;
    bool stable;
};

struct headset_inputs {
    struct headset_input mode_bluetooth;
    struct headset_input pair;
    struct headset_input play;
    struct headset_input volume_down;
    struct headset_input volume_up;
};

bool headset_button_pressed(struct headset_input *input, bool down, uint32_t now_ms);
uint8_t headset_inputs_update(struct headset_inputs *inputs, bool bluetooth,
                              bool pair_down, uint32_t now_ms);

#endif
