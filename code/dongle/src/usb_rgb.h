#ifndef HEADPHONES_DONGLE_RGB_H
#define HEADPHONES_DONGLE_RGB_H

#include <stdbool.h>

int usb_rgb_register(void);
void usb_rgb_poll(bool linked);

#endif
