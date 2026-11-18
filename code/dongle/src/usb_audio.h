#ifndef HEADPHONES_DONGLE_USB_AUDIO_H
#define HEADPHONES_DONGLE_USB_AUDIO_H

#include <stdbool.h>
#include <stdint.h>

int usb_audio_start(void);
bool usb_read_playback(int16_t *pcm);
bool usb_mic_enabled(void);
bool usb_suspended(void);
void usb_receive_mic(const int16_t *pcm);
void usb_clear_audio(void);
void usb_media_enqueue(uint8_t usage);
void usb_media_poll(void);

#endif
