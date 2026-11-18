#include "headset_logic.h"

enum { DEBOUNCE_MS = 25 };

static bool input_changed(struct headset_input *input, bool sample, uint32_t now_ms)
{
    if (!input->initialized) {
        input->candidate = sample;
        input->stable = sample;
        input->changed_ms = now_ms;
        input->initialized = true;
        return true;
    }

    if (input->candidate != sample) {
        input->candidate = sample;
        input->changed_ms = now_ms;
    }

    if (input->stable != sample && now_ms - input->changed_ms >= DEBOUNCE_MS) {
        input->stable = sample;
        return true;
    }
    return false;
}

bool headset_button_pressed(struct headset_input *input, bool down, uint32_t now_ms)
{
    const bool initialized = input->initialized;
    return input_changed(input, down, now_ms) && initialized && input->stable;
}

uint8_t headset_inputs_update(struct headset_inputs *inputs, bool bluetooth,
                              bool pair_down, uint32_t now_ms)
{
    uint8_t events = 0;

    if (input_changed(&inputs->mode_bluetooth, bluetooth, now_ms)) {
        events |= HEADSET_MODE_CHANGED;
    }
    if (headset_button_pressed(&inputs->pair, pair_down, now_ms)) {
        events |= HEADSET_PAIR_PRESSED;
    }
    return events;
}
