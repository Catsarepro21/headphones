#include "rgb.h"

#include <errno.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/class/usbd_hid.h>
#include <zephyr/usb/usbd.h>

enum {
    RGB_ENABLE_PIN = 22,
    REPORT_ID = 1,
    RGB_CHANNELS = 3,
    CUP_LEDS = 16,
    CUP_BYTES = CUP_LEDS * RGB_CHANNELS,
    FRAME_BYTES = 2 * CUP_BYTES,
    REPORT_HEADER = 4,
    REPORT_BYTES = REPORT_HEADER + CUP_BYTES,
    COMMAND_FRAME = 0x52,
    SYMBOL_BITS = 10,
    RESET_BYTES = 320,
    RGB_TIMEOUT_MS = 1000,
    CHANNEL_SUM_LIMIT = 4800,
};

static const struct device *const gpio = DEVICE_DT_GET(DT_NODELABEL(gpio0));
static const struct device *const spi = DEVICE_DT_GET(DT_NODELABEL(spi2));
static const struct device *const hid = DEVICE_DT_GET(DT_NODELABEL(headphone_rgb));
static const struct spi_config spi_config = {
    .frequency = 8000000,
    .operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8),
};
static uint8_t wave[FRAME_BYTES * SYMBOL_BITS + RESET_BYTES];
static uint8_t staged[FRAME_BYTES];
static uint8_t pending[FRAME_BYTES];
static uint8_t sequence;
static bool have_left;
static bool frame_ready;
static struct k_spinlock frame_lock;
static atomic_t usb_active;
static bool rail_enabled;
static uint32_t last_frame;

USBD_DEVICE_DEFINE(headset_usb, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
                   CONFIG_HEADSET_USB_VID, CONFIG_HEADSET_USB_PID);
USBD_DESC_LANG_DEFINE(lang);
USBD_DESC_PRODUCT_DEFINE(product, "Headphones RGB");
USBD_CONFIGURATION_DEFINE(usb_config, USB_SCD_SELF_POWERED, 50, NULL);

static const uint8_t report_descriptor[] = {
    0x06, 0x31, 0xff,
    0x09, 0x01, 0xa1, 0x01,
    0x85, REPORT_ID,
    0x15, 0x00, 0x26, 0xff, 0x00,
    0x75, 0x08, 0x95, REPORT_BYTES - 1,
    0x09, 0x02, 0xb1, 0x02,
    0xc0,
};

static int receive_report(const struct device *dev, uint8_t type, uint8_t id,
                          uint16_t len, const uint8_t *data)
{
    (void)dev;
    if (type != HID_REPORT_TYPE_FEATURE || id != REPORT_ID ||
        len != REPORT_BYTES || data[0] != REPORT_ID ||
        data[1] != COMMAND_FRAME || data[3] > 1U) {
        return -EINVAL;
    }

    k_spinlock_key_t key = k_spin_lock(&frame_lock);
    if (data[3] == 0U) {
        memcpy(staged, data + REPORT_HEADER, CUP_BYTES);
        sequence = data[2];
        have_left = true;
    } else if (have_left && data[2] == sequence) {
        memcpy(staged + CUP_BYTES, data + REPORT_HEADER, CUP_BYTES);
        memcpy(pending, staged, sizeof(pending));
        have_left = false;
        frame_ready = true;
    }
    k_spin_unlock(&frame_lock, key);
    return 0;
}

static void usb_ready(const struct device *dev, bool ready)
{
    (void)dev;
    atomic_set(&usb_active, ready);
    if (!ready) {
        k_spinlock_key_t key = k_spin_lock(&frame_lock);
        have_left = false;
        frame_ready = false;
        k_spin_unlock(&frame_lock, key);
    }
}

static const struct hid_device_ops hid_ops = {
    .iface_ready = usb_ready,
    .set_report = receive_report,
};

static void usb_message(struct usbd_context *ctx, const struct usbd_msg *msg)
{
    int status = 0;
    switch (msg->type) {
    case USBD_MSG_VBUS_READY:
        status = usbd_enable(ctx);
        break;
    case USBD_MSG_VBUS_REMOVED:
        usb_ready(hid, false);
        status = usbd_disable(ctx);
        break;
    default:
        break;
    }
    if (status < 0) {
        printk("RGB USB VBUS event failed: %d\n", status);
    }
}

int rgb_start(void)
{
    if (CONFIG_HEADSET_USB_VID == 0 || CONFIG_HEADSET_USB_PID == 0) {
        return -ENOTSUP;
    }
    if (!device_is_ready(hid) || !device_is_ready(spi)) {
        return -ENODEV;
    }
    int status = hid_device_register(hid, report_descriptor, sizeof(report_descriptor), &hid_ops);
    if (status == 0) {
        status = usbd_add_descriptor(&headset_usb, &lang);
    }
    if (status == 0) {
        status = usbd_add_descriptor(&headset_usb, &product);
    }
    if (status == 0) {
        status = usbd_add_configuration(&headset_usb, USBD_SPEED_FS, &usb_config);
    }
    if (status == 0) {
        status = usbd_register_class(&headset_usb, "hid_0", USBD_SPEED_FS, 1);
    }
    if (status == 0) {
        status = usbd_msg_register_cb(&headset_usb, usb_message);
    }
    if (status == 0) {
        usbd_self_powered(&headset_usb, true);
        status = usbd_init(&headset_usb);
    }
    if (status == 0 && !usbd_can_detect_vbus(&headset_usb)) {
        status = usbd_enable(&headset_usb);
    }
    return status;
}

static void encode_channel(uint8_t *out, uint8_t value)
{
    uint32_t bits = 0;
    unsigned int held = 0;
    for (uint8_t bit = 0x80; bit != 0; bit >>= 1) {
        bits = (bits << SYMBOL_BITS) | ((value & bit) != 0 ? 0x3e0U : 0x380U);
        held += SYMBOL_BITS;
        while (held >= 8U) {
            held -= 8U;
            *out++ = (uint8_t)(bits >> held);
        }
    }
}

static int send_pixels(const uint8_t *rgb, uint32_t sum)
{
    const uint8_t channel_order[] = {1, 0, 2};
    uint8_t *out = wave;
    for (size_t pixel = 0; pixel < FRAME_BYTES; pixel += RGB_CHANNELS) {
        for (size_t channel = 0; channel < RGB_CHANNELS; ++channel) {
            uint8_t value = rgb[pixel + channel_order[channel]];
            if (sum > CHANNEL_SUM_LIMIT) {
                value = (uint32_t)value * CHANNEL_SUM_LIMIT / sum;
            }
            encode_channel(out, value);
            out += SYMBOL_BITS;
        }
    }
    const struct spi_buf buffer = {.buf = wave, .len = sizeof(wave)};
    const struct spi_buf_set tx = {.buffers = &buffer, .count = 1};
    return spi_write(spi, &spi_config, &tx);
}

static void turn_off(void)
{
    if (rail_enabled && gpio_pin_set(gpio, RGB_ENABLE_PIN, 0) == 0) {
        rail_enabled = false;
    }
}

void rgb_poll(void)
{
    const uint32_t now = k_uptime_get_32();
    uint8_t pixels[FRAME_BYTES];
    k_spinlock_key_t key = k_spin_lock(&frame_lock);
    const bool ready = frame_ready;
    if (ready) {
        memcpy(pixels, pending, sizeof(pixels));
        frame_ready = false;
    }
    k_spin_unlock(&frame_lock, key);
    if (!atomic_get(&usb_active) || (!ready && now - last_frame >= RGB_TIMEOUT_MS)) {
        turn_off();
        return;
    }
    if (!ready) {
        return;
    }

    uint32_t sum = 0;
    for (size_t i = 0; i < sizeof(pixels); ++i) {
        sum += pixels[i];
    }
    if (sum == 0U) {
        turn_off();
        return;
    }
    if (!rail_enabled) {
        if (gpio_pin_set(gpio, RGB_ENABLE_PIN, 1) < 0) {
            return;
        }
        rail_enabled = true;
        k_msleep(10);
    }
    if (send_pixels(pixels, sum) < 0) {
        turn_off();
        printk("RGB SPI transfer failed\n");
        return;
    }
    last_frame = now;
}
