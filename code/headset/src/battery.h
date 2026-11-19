#ifndef HEADPHONES_BATTERY_H
#define HEADPHONES_BATTERY_H

#include <stdint.h>

int battery_start(void);
void battery_poll(uint32_t now);

#endif
