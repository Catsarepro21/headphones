#ifndef HEADSET_RGB_H
#define HEADSET_RGB_H

#include <stdbool.h>
#include <stdint.h>

int rgb_start(void);
void rgb_poll(bool external_power);
void rgb_receive_wireless(const uint8_t *pixels);
void rgb_clear_wireless(void);

#endif
