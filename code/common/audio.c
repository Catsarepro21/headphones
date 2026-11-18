#include "audio.h"

static const int steps[] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544,
    598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707,
    1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871,
    5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
};

static const int index_change[] = {-1, -1, -1, -1, 2, 4, 6, 8};

struct adpcm_state {
    int predictor;
    int index;
};

static int clamp(int value, int min, int max)
{
    return value < min ? min : (value > max ? max : value);
}

static void apply_code(struct adpcm_state *state, uint8_t code)
{
    const int step = steps[state->index];
    int delta = step >> 3;
    if ((code & 4U) != 0U) {
        delta += step;
    }
    if ((code & 2U) != 0U) {
        delta += step >> 1;
    }
    if ((code & 1U) != 0U) {
        delta += step >> 2;
    }
    state->predictor = clamp(state->predictor + ((code & 8U) != 0U ? -delta : delta),
                             INT16_MIN, INT16_MAX);
    state->index = clamp(state->index + index_change[code & 7U], 0, 88);
}

static uint8_t encode_sample(struct adpcm_state *state, int16_t sample)
{
    int difference = sample - state->predictor;
    uint8_t code = difference < 0 ? 8U : 0U;
    if (difference < 0) {
        difference = -difference;
    }
    int step = steps[state->index];
    for (uint8_t bit = 4; bit != 0U; bit >>= 1, step >>= 1) {
        if (difference >= step) {
            code |= bit;
            difference -= step;
        }
    }
    apply_code(state, code);
    return code;
}

static size_t packet_size(unsigned int channels)
{
    return 2U + 3U * channels + ((AUDIO_FRAMES - 1U) * channels + 1U) / 2U;
}

size_t audio_encode(struct audio_encoder *encoder, uint8_t *packet, const int16_t *pcm,
                    unsigned int channels)
{
    if (channels != AUDIO_MONO && channels != AUDIO_STEREO) {
        return 0;
    }
    struct adpcm_state state[AUDIO_STEREO];
    packet[0] = 1;
    packet[1] = (uint8_t)channels;
    for (unsigned int channel = 0; channel < channels; ++channel) {
        const uint16_t first = (uint16_t)pcm[channel];
        state[channel] = (struct adpcm_state){pcm[channel], encoder->index[channel]};
        packet[2U + channel * 3U] = (uint8_t)first;
        packet[3U + channel * 3U] = (uint8_t)(first >> 8);
        packet[4U + channel * 3U] = encoder->index[channel];
    }
    uint8_t *codes = packet + 2U + 3U * channels;
    const size_t count = (AUDIO_FRAMES - 1U) * channels;
    for (size_t i = 0; i < count; ++i) {
        const uint8_t code = encode_sample(&state[i % channels], pcm[channels + i]);
        if (i % 2U == 0U) {
            codes[i / 2U] = code;
        } else {
            codes[i / 2U] |= (uint8_t)(code << 4);
        }
    }
    for (unsigned int channel = 0; channel < channels; ++channel) {
        encoder->index[channel] = (uint8_t)state[channel].index;
    }
    return packet_size(channels);
}

bool audio_decode(int16_t *pcm, const uint8_t *packet, size_t length, unsigned int channels)
{
    if ((channels != AUDIO_MONO && channels != AUDIO_STEREO) ||
        length != packet_size(channels) || packet[0] != 1U || packet[1] != channels) {
        return false;
    }
    struct adpcm_state state[AUDIO_STEREO];
    for (unsigned int channel = 0; channel < channels; ++channel) {
        const uint8_t *header = packet + 2U + channel * 3U;
        if (header[2] > 88U) {
            return false;
        }
        pcm[channel] = (int16_t)((uint16_t)header[0] | ((uint16_t)header[1] << 8));
        state[channel] = (struct adpcm_state){pcm[channel], header[2]};
    }
    const uint8_t *codes = packet + 2U + 3U * channels;
    for (size_t i = 0; i < (AUDIO_FRAMES - 1U) * channels; ++i) {
        struct adpcm_state *const sample = &state[i % channels];
        apply_code(sample, (codes[i / 2U] >> (4U * (i % 2U))) & 15U);
        pcm[channels + i] = (int16_t)sample->predictor;
    }
    return true;
}
