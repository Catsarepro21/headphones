#include "wireless_audio.h"
#include "audio.h"
#include "audio_control.h"
#include "audio_fifo.h"
#include "codec.h"
#include "radio.h"
#include "rgb.h"
#include "rgb_frame.h"

#include <errno.h>
#include <string.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>

enum {
    PCM_BYTES = AUDIO_FRAMES * AUDIO_STEREO * sizeof(int16_t),
    UNMUTE_POLL_MS = 100,
    UNMUTE_TIMEOUT_MS = 10000,
};

static const struct device *const i2s = DEVICE_DT_GET(DT_NODELABEL(i2s0));
static atomic_t selected;
static atomic_t mic_requested;
static atomic_t mic_at;
static atomic_t mic_muted = ATOMIC_INIT(1);
static bool initialized;
AUDIO_FIFO_DEFINE(speaker, AUDIO_STEREO);
K_MEM_SLAB_DEFINE_STATIC(pcm_tx, PCM_BYTES, 5, 4);
K_MEM_SLAB_DEFINE_STATIC(pcm_rx, PCM_BYTES, 5, 4);
K_THREAD_STACK_DEFINE(audio_stack, 2560);
static struct k_thread audio_thread;

static void receive(const uint8_t *packet, size_t length)
{
    if (!atomic_get(&selected)) {
        return;
    }
    if (length == RGB_RADIO_BYTES && packet[0] == RGB_COMMAND_FRAME) {
        rgb_receive_wireless(packet + 1);
        return;
    }
    if (length == 3U && packet[0] == AUDIO_CONTROL && packet[1] == AUDIO_CAPTURE_ENABLE) {
        atomic_set(&mic_at, (atomic_val_t)k_uptime_get_32());
        atomic_set(&mic_requested, packet[2] == 1U);
        return;
    }
    int16_t pcm[AUDIO_FRAMES * AUDIO_STEREO];
    if (audio_decode(pcm, packet, length, AUDIO_STEREO)) {
        audio_fifo_write(&speaker, pcm, AUDIO_FRAMES);
    }
}

static int write_playback(bool silence)
{
    void *block;
    int status = k_mem_slab_alloc(&pcm_tx, &block, K_NO_WAIT);
    if (status < 0) {
        return status;
    }
    if (silence) {
        memset(block, 0, PCM_BYTES);
    } else {
        audio_fifo_play(&speaker, block, AUDIO_FRAMES);
    }
    status = i2s_write(i2s, block, PCM_BYTES);
    if (status < 0) {
        k_mem_slab_free(&pcm_tx, block);
    }
    return status;
}

static bool active(void)
{
    return atomic_get(&selected) != 0 && radio_active();
}

static int pump_audio(void)
{
    struct audio_encoder encoder = {0};
    bool waiting_to_unmute = true;
    const uint32_t started = k_uptime_get_32();
    uint32_t last_unmute = started;
    while (active()) {
        void *block;
        size_t size;
        int status = i2s_read(i2s, &block, &size);
        if (status < 0) {
            return status;
        }
        if (active() && atomic_get(&mic_requested) && size == PCM_BYTES &&
            k_uptime_get_32() - (uint32_t)atomic_get(&mic_at) < 1500U) {
            int16_t mono[AUDIO_FRAMES] = {0};
            uint8_t packet[AUDIO_PACKET_MAX];
            const int16_t *const stereo = block;
            if (!atomic_get(&mic_muted)) {
                for (size_t i = 0; i < AUDIO_FRAMES; ++i) {
                    mono[i] = stereo[i * AUDIO_STEREO];
                }
            }
            const size_t length = audio_encode(&encoder, packet, mono, AUDIO_MONO);
            (void)radio_send(packet, length);
        }
        k_mem_slab_free(&pcm_rx, block);
        status = write_playback(false);
        if (status < 0) {
            return status;
        }
        const uint32_t now = k_uptime_get_32();
        if (waiting_to_unmute && now - last_unmute >= UNMUTE_POLL_MS) {
            last_unmute = now;
            status = codec_audio_unmute();
            if (status == 0) {
                waiting_to_unmute = false;
            } else if (status != -EAGAIN) {
                return status;
            } else if (now - started >= UNMUTE_TIMEOUT_MS) {
                waiting_to_unmute = false;
                printk("Headphones stayed muted during codec startup\n");
            }
        }
    }
    return 0;
}

static int start_stream(void)
{
    int status = codec_audio_start();
    if (status == 0) {
        status = write_playback(true);
    }
    if (status == 0) {
        status = write_playback(true);
    }
    if (status == 0) {
        status = write_playback(true);
    }
    if (status == 0) {
        status = i2s_trigger(i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
    }
    return status;
}

static void audio_run(void *a, void *b, void *c)
{
    ARG_UNUSED(a);
    ARG_UNUSED(b);
    ARG_UNUSED(c);
    bool needs_reset = false;
    for (;;) {
        if (!active()) {
            k_msleep(20);
            continue;
        }
        int status = needs_reset ? codec_prepare() : 0;
        if (status == 0) {
            status = start_stream();
        }
        if (status == 0) {
            status = pump_audio();
        }
        const int stopped = codec_audio_stop();
        needs_reset = status < 0 || stopped < 0;
        (void)i2s_trigger(i2s, I2S_DIR_BOTH, I2S_TRIGGER_DROP);
        audio_fifo_clear(&speaker);
        atomic_clear(&mic_requested);
        if (status < 0) {
            printk("Wireless audio restart: %d\n", status);
            k_msleep(250);
        }
    }
}

int wireless_audio_start(void)
{
    if (!device_is_ready(i2s)) {
        return -ENODEV;
    }
    struct i2s_config config = {
        .word_size = 16,
        .channels = AUDIO_STEREO,
        .format = I2S_FMT_DATA_FORMAT_I2S,
        .options = I2S_OPT_BIT_CLK_SLAVE | I2S_OPT_FRAME_CLK_SLAVE,
        .frame_clk_freq = AUDIO_RATE,
        .mem_slab = &pcm_tx,
        .block_size = PCM_BYTES,
        .timeout = 30,
    };
    int status = i2s_configure(i2s, I2S_DIR_TX, &config);
    if (status == 0) {
        config.mem_slab = &pcm_rx;
        status = i2s_configure(i2s, I2S_DIR_RX, &config);
    }
    if (status == 0) {
        status = radio_start(receive);
    }
    if (status == 0) {
        initialized = true;
        k_thread_create(&audio_thread, audio_stack, K_THREAD_STACK_SIZEOF(audio_stack),
                        audio_run, NULL, NULL, NULL, 5, 0, K_NO_WAIT);
    }
    return status;
}

void wireless_audio_select(bool enabled)
{
    if (initialized) {
        atomic_set(&selected, enabled);
        radio_set_available(enabled);
    }
}

void wireless_audio_poll(void)
{
    if (initialized) {
        radio_poll();
        if (!radio_active()) {
            rgb_clear_wireless();
        }
    }
}

void wireless_audio_pair(void)
{
    if (initialized) {
        radio_allow_pairing();
    }
}

void wireless_audio_media(uint8_t usage)
{
    if (active()) {
        const uint8_t packet[] = {AUDIO_CONTROL, usage, 0};
        (void)radio_send(packet, sizeof(packet));
    }
}

void wireless_audio_set_mic_muted(bool muted)
{
    atomic_set(&mic_muted, muted);
}
