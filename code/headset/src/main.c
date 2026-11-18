#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "bm83_link.h"
#include "codec.h"
#include "headset_logic.h"
#include "rgb.h"

enum {
    PIN_MODE_N = 29,
    PIN_PAIR_N = 4,
    PIN_PLAY_N = 24,
    PIN_DOWN_N = 0,
    PIN_UP_N = 2,
    PIN_USB_EN1 = 26,
    PIN_USB_EN2 = 13,
    PIN_RGB_ENABLE = 22,
    PIN_BT_AUDIO_SEL = 31,
    PIN_CHG_N = 4,
    PIN_PGOOD_N = 5,
};

static const struct device *const gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
static const struct device *const gpio1 = DEVICE_DT_GET(DT_NODELABEL(gpio1));

int main(void)
{
    struct headset_inputs inputs = {0};
    bool bluetooth_routed = false;
    int last_charger_status = -1;

    if (!device_is_ready(gpio0) || !device_is_ready(gpio1)) {
        printk("GPIO unavailable\n");
        return 1;
    }

    if (gpio_pin_configure(gpio0, PIN_RGB_ENABLE, GPIO_OUTPUT_INACTIVE) < 0 ||
        gpio_pin_configure(gpio0, PIN_BT_AUDIO_SEL, GPIO_OUTPUT_INACTIVE) < 0 ||
        gpio_pin_configure(gpio0, PIN_USB_EN1, GPIO_OUTPUT_INACTIVE) < 0 ||
        gpio_pin_configure(gpio1, PIN_USB_EN2, GPIO_OUTPUT_INACTIVE) < 0 ||
        gpio_pin_configure(gpio0, PIN_CHG_N, GPIO_INPUT) < 0 ||
        gpio_pin_configure(gpio0, PIN_PGOOD_N, GPIO_INPUT) < 0 ||
        gpio_pin_configure(gpio0, PIN_MODE_N, GPIO_INPUT) < 0 ||
        gpio_pin_configure(gpio1, PIN_PAIR_N, GPIO_INPUT | GPIO_PULL_UP) < 0 ||
        gpio_pin_configure(gpio0, PIN_PLAY_N, GPIO_INPUT | GPIO_PULL_UP) < 0 ||
        gpio_pin_configure(gpio1, PIN_DOWN_N, GPIO_INPUT | GPIO_PULL_UP) < 0 ||
        gpio_pin_configure(gpio1, PIN_UP_N, GPIO_INPUT | GPIO_PULL_UP) < 0) {
        printk("GPIO setup failed\n");
        return 1;
    }

    printk("Headset bring-up\n");
    const int codec_status = codec_prepare();
    printk("Codec clock register setup: %d\n", codec_status);
    const int bt_status = bm83_start();
    if (bt_status < 0) {
        printk("BM83 UART setup failed: %d\n", bt_status);
    }
    const int rgb_status = rgb_start();
    if (rgb_status < 0) {
        printk("RGB over USB unavailable: %d\n", rgb_status);
    }

    for (;;) {
        const int mode = gpio_pin_get_raw(gpio0, PIN_MODE_N);
        const int pair = gpio_pin_get_raw(gpio1, PIN_PAIR_N);
        const int play = gpio_pin_get_raw(gpio0, PIN_PLAY_N);
        const int down = gpio_pin_get_raw(gpio1, PIN_DOWN_N);
        const int up = gpio_pin_get_raw(gpio1, PIN_UP_N);
        const int pgood = gpio_pin_get_raw(gpio0, PIN_PGOOD_N);
        const int charging = gpio_pin_get_raw(gpio0, PIN_CHG_N);

        if (mode < 0 || pair < 0 || play < 0 || down < 0 || up < 0) {
            printk("Input read failed\n");
            return 1;
        }

        const uint32_t now = k_uptime_get_32();
        const uint8_t events = headset_inputs_update(&inputs, mode == 0, pair == 0, now);
        const bool play_pressed = headset_button_pressed(&inputs.play, play == 0, now);
        const bool down_pressed = headset_button_pressed(&inputs.volume_down, down == 0, now);
        const bool up_pressed = headset_button_pressed(&inputs.volume_up, up == 0, now);
        if (pgood >= 0 && charging >= 0) {
            const int charger_status = pgood != 0 ? 0 : (charging == 0 ? 2 : 1);
            if (charger_status != last_charger_status) {
                const char *const statuses[] = {"No valid input", "Input valid", "Charging"};
                printk("Charger: %s\n", statuses[charger_status]);
                last_charger_status = charger_status;
            }
        }

        if ((events & HEADSET_MODE_CHANGED) != 0U) {
            printk("Requested source: %s\n", inputs.mode_bluetooth.stable ?
                   "Bluetooth" : "USB dongle");
            if (bt_status == 0) {
                bm83_set_selected(inputs.mode_bluetooth.stable);
            }
        }
        if (bt_status == 0) {
            if ((events & HEADSET_PAIR_PRESSED) != 0U) {
                bm83_request_pairing();
            }
            if (play_pressed) {
                bm83_request_media(BM83_MEDIA_PLAY_PAUSE);
            }
            if (down_pressed) {
                bm83_request_media(BM83_MEDIA_VOLUME_DOWN);
            }
            if (up_pressed) {
                bm83_request_media(BM83_MEDIA_VOLUME_UP);
            }
            bm83_poll();
            const bool route_bluetooth = bm83_audio_available();
            if (route_bluetooth != bluetooth_routed) {
                if (gpio_pin_set(gpio0, PIN_BT_AUDIO_SEL, route_bluetooth) < 0) {
                    printk("Audio switch update failed\n");
                    return 1;
                }
                bluetooth_routed = route_bluetooth;
            }
        }

        if (rgb_status == 0) {
            rgb_poll();
        }
        k_msleep(5);
    }
}
