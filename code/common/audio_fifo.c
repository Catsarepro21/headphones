#include "audio_fifo.h"

#include <string.h>

void audio_fifo_clear(struct audio_fifo *fifo)
{
    k_spinlock_key_t key = k_spin_lock(&fifo->lock);
    fifo->head = 0;
    fifo->used = 0;
    fifo->primed = false;
    k_spin_unlock(&fifo->lock, key);
}

void audio_fifo_write(struct audio_fifo *fifo, const int16_t *samples, size_t frames)
{
    k_spinlock_key_t key = k_spin_lock(&fifo->lock);
    const size_t overflow = frames + fifo->used > AUDIO_FIFO_FRAMES ?
        frames + fifo->used - AUDIO_FIFO_FRAMES : 0U;
    const size_t tail = (fifo->head + fifo->used) % AUDIO_FIFO_FRAMES;
    const size_t contiguous = MIN(frames, AUDIO_FIFO_FRAMES - tail);
    const size_t width = fifo->channels * sizeof(*samples);
    memcpy(fifo->samples + tail * fifo->channels, samples, contiguous * width);
    memcpy(fifo->samples, samples + contiguous * fifo->channels,
           (frames - contiguous) * width);
    fifo->head = (fifo->head + overflow) % AUDIO_FIFO_FRAMES;
    fifo->used += frames - overflow;
    k_spin_unlock(&fifo->lock, key);
}

static void pop(struct audio_fifo *fifo, int16_t *samples, size_t frames)
{
    const size_t contiguous = MIN(frames, AUDIO_FIFO_FRAMES - fifo->head);
    const size_t width = fifo->channels * sizeof(*samples);
    memcpy(samples, fifo->samples + fifo->head * fifo->channels, contiguous * width);
    memcpy(samples + contiguous * fifo->channels, fifo->samples, (frames - contiguous) * width);
    fifo->head = (fifo->head + frames) % AUDIO_FIFO_FRAMES;
    fifo->used -= frames;
}

bool audio_fifo_read(struct audio_fifo *fifo, int16_t *samples, size_t frames)
{
    k_spinlock_key_t key = k_spin_lock(&fifo->lock);
    const bool available = fifo->used >= frames;
    if (available) {
        pop(fifo, samples, frames);
    }
    k_spin_unlock(&fifo->lock, key);
    return available;
}

void audio_fifo_play(struct audio_fifo *fifo, int16_t *samples, size_t frames)
{
    int16_t buffer[(AUDIO_FRAMES + 1) * AUDIO_STEREO];
    size_t consume = frames;
    k_spinlock_key_t key = k_spin_lock(&fifo->lock);
    if (fifo->used < frames) {
        fifo->primed = false;
    } else if (fifo->used >= 3U * AUDIO_FRAMES) {
        fifo->primed = true;
    }
    if (!fifo->primed) {
        k_spin_unlock(&fifo->lock, key);
        memset(samples, 0, frames * fifo->channels * sizeof(*samples));
        return;
    }
    if (fifo->used > 5U * AUDIO_FRAMES) {
        ++consume;
    } else if (fifo->used < 2U * AUDIO_FRAMES) {
        --consume;
    }
    pop(fifo, consume == frames ? samples : buffer, consume);
    k_spin_unlock(&fifo->lock, key);
    if (consume == frames) {
        return;
    }
    for (size_t i = 0; i < frames; ++i) {
        const size_t position = i * (consume - 1U);
        const size_t index = position / (frames - 1U);
        const int32_t mix = (int32_t)(position % (frames - 1U));
        for (size_t channel = 0; channel < fifo->channels; ++channel) {
            const int32_t a = buffer[index * fifo->channels + channel];
            const size_t next = MIN(index + 1U, consume - 1U);
            const int32_t b = buffer[next * fifo->channels + channel];
            samples[i * fifo->channels + channel] =
                (int16_t)(a + ((b - a) * mix) / (int32_t)(frames - 1U));
        }
    }
}
