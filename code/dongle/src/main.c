#include "audio.h"
#include "audio_control.h"
#include "radio.h"
#include "usb_audio.h"
#include "usb_rgb.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

static void received(const uint8_t *packet, size_t length)
{
    if (length == 3U && packet[0] == AUDIO_CONTROL) {
        usb_media_enqueue(packet[1]);
        return;
    }
    int16_t pcm[AUDIO_FRAMES];
    if (audio_decode(pcm, packet, length, AUDIO_MONO)) {
        usb_receive_mic(pcm);
    }
}

int main(void)
{
    const struct device *const gpio = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    if (!device_is_ready(gpio) || gpio_pin_configure(gpio, 5, GPIO_OUTPUT_INACTIVE) < 0) {
        return 1;
    }
    int status = usb_audio_start();
    if (status < 0) {
        printk("Dongle USB setup: %d\n", status);
        return 1;
    }
    status = radio_start(received);
    if (status < 0) {
        printk("Dongle radio setup: %d\n", status);
        return 1;
    }

    struct audio_encoder encoder = {0};
    bool linked = false;
    bool last_capture = false;
    uint32_t last_capture_at = 0;
    for (;;) {
        radio_poll();
        const bool active = radio_active();
        if (active != linked) {
            linked = active;
            usb_clear_audio();
            (void)gpio_pin_set(gpio, 5, linked);
            last_capture = !usb_mic_enabled();
        }
        if (linked) {
            const uint32_t now = k_uptime_get_32();
            const bool capture = usb_mic_enabled();
            if (capture != last_capture || now - last_capture_at >= 500U) {
                const uint8_t request[] = {AUDIO_CONTROL, AUDIO_CAPTURE_ENABLE, capture};
                if (radio_send(request, sizeof(request)) == 0) {
                    last_capture = capture;
                    last_capture_at = now;
                }
            }
            int16_t pcm[AUDIO_FRAMES * AUDIO_STEREO];
            if (usb_read_playback(pcm)) {
                uint8_t packet[AUDIO_PACKET_MAX];
                const size_t length = audio_encode(&encoder, packet, pcm, AUDIO_STEREO);
                (void)radio_send(packet, length);
            }
        }
        usb_rgb_poll(linked && !usb_suspended());
        usb_media_poll();
        k_msleep(1);
    }
}
