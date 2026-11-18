#ifndef HEADPHONES_AUDIO_H
#define HEADPHONES_AUDIO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    AUDIO_RATE = 48000,
    AUDIO_USB_FRAMES = 48,
    AUDIO_FRAMES = 192,
    AUDIO_STEREO = 2,
    AUDIO_MONO = 1,
    AUDIO_PACKET_MAX = 2 + 3 * AUDIO_STEREO + (AUDIO_FRAMES - 1),
    AUDIO_PACKET_MIN_MTU = AUDIO_PACKET_MAX + 3,
};

struct audio_encoder {
    uint8_t index[AUDIO_STEREO];
};

size_t audio_encode(struct audio_encoder *encoder, uint8_t *packet, const int16_t *pcm,
                    unsigned int channels);
bool audio_decode(int16_t *pcm, const uint8_t *packet, size_t length, unsigned int channels);

#endif
