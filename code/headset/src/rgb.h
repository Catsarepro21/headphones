#ifndef HEADSET_RGB_H
#define HEADSET_RGB_H

#include <stdint.h>

int rgb_start(void);
void rgb_poll(void);
void rgb_receive_wireless(const uint8_t *pixels);
void rgb_clear_wireless(void);

#endif
