#ifndef HEADPHONES_WIRELESS_AUDIO_H
#define HEADPHONES_WIRELESS_AUDIO_H

#include <stdbool.h>
#include <stdint.h>

int wireless_audio_start(void);
void wireless_audio_select(bool selected);
void wireless_audio_poll(void);
void wireless_audio_pair(void);
void wireless_audio_media(uint8_t usage);

#endif
