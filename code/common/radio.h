#ifndef HEADPHONES_RADIO_H
#define HEADPHONES_RADIO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*radio_receive_fn)(const uint8_t *packet, size_t length);

int radio_start(radio_receive_fn receive);
void radio_poll(void);
bool radio_active(void);
int radio_send(const uint8_t *packet, size_t length);

#ifndef HEADPHONES_DONGLE
void radio_set_available(bool enabled);
void radio_allow_pairing(void);
#endif

#endif
