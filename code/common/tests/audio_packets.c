#include "audio.h"

#include <assert.h>
#include <stdlib.h>

static void check_audio(unsigned int channels)
{
    int16_t pcm[AUDIO_FRAMES * AUDIO_STEREO] = {0};
    int16_t decoded[AUDIO_FRAMES * AUDIO_STEREO] = {0};
    uint8_t packet[AUDIO_PACKET_MAX];
    struct audio_encoder encoder = {0};
    for (unsigned int i = 0; i < AUDIO_FRAMES; ++i) {
        pcm[i * channels] = (int16_t)(-15000 + (int)i * 150);
        if (channels == AUDIO_STEREO) {
            pcm[i * channels + 1U] = (int16_t)(10000 - (int)i * 100);
        }
    }
    size_t length = audio_encode(&encoder, packet, pcm, channels);
    assert(length == (channels == AUDIO_STEREO ? 199U : 101U));
    assert(audio_decode(decoded, packet, length, channels));
    for (unsigned int i = 0; i < channels; ++i) {
        assert(decoded[i] == pcm[i]);
    }
    for (unsigned int i = 24U * channels; i < AUDIO_FRAMES * channels; ++i) {
        assert(abs(decoded[i] - pcm[i]) < 220);
    }
    assert(!audio_decode(decoded, packet, length - 1U, channels));
    packet[4] = 89;
    assert(!audio_decode(decoded, packet, length, channels));
    length = audio_encode(&encoder, packet, pcm, channels);
    assert(audio_decode(decoded, packet, length, channels));
}

int main(void)
{
    check_audio(AUDIO_MONO);
    check_audio(AUDIO_STEREO);
    return 0;
}
