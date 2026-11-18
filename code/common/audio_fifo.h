#ifndef HEADPHONES_AUDIO_FIFO_H
#define HEADPHONES_AUDIO_FIFO_H

#include "audio.h"

#include <zephyr/kernel.h>

enum { AUDIO_FIFO_FRAMES = 8 * AUDIO_FRAMES };

struct audio_fifo {
    struct k_spinlock lock;
    int16_t *samples;
    size_t head;
    size_t used;
    unsigned int channels;
    bool primed;
};

#define AUDIO_FIFO_DEFINE(name, count) \
    static int16_t name##_samples[AUDIO_FIFO_FRAMES * (count)]; \
    static struct audio_fifo name = {.samples = name##_samples, .channels = (count)}

void audio_fifo_clear(struct audio_fifo *fifo);
void audio_fifo_write(struct audio_fifo *fifo, const int16_t *samples, size_t frames);
bool audio_fifo_read(struct audio_fifo *fifo, int16_t *samples, size_t frames);
void audio_fifo_play(struct audio_fifo *fifo, int16_t *samples, size_t frames);

#endif
