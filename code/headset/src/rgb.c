#include "rgb.h"
#include "rgb_frame.h"

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
    CHARGER_EN1_PIN = 26,
    CHARGER_EN2_PIN = 13,
    SYMBOL_BITS = 10,
    RESET_BYTES = 320,
    CHANNEL_SUM_LIMIT = 4800,
    EXTERNAL_POWER_CHANNEL_SUM_LIMIT = 960,
};

static const struct device *const gpio = DEVICE_DT_GET(DT_NODELABEL(gpio0));
static const struct device *const gpio1 = DEVICE_DT_GET(DT_NODELABEL(gpio1));
static const struct device *const spi = DEVICE_DT_GET(DT_NODELABEL(spi2));
static const struct device *const hid = DEVICE_DT_GET(DT_NODELABEL(headphone_rgb));
static const struct spi_config spi_config = {
    .frequency = 8000000,
    .operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8),
};
struct pixels {
    uint8_t data[RGB_FRAME_BYTES];
    uint32_t received_at;
    bool valid;
    bool pending;
};

static uint8_t wave[RGB_FRAME_BYTES * SYMBOL_BITS + RESET_BYTES];
static struct rgb_frame staged;
static struct pixels usb_pixels;
static struct pixels wireless_pixels;
static struct k_spinlock frame_lock;
static bool rail_enabled;
static uint32_t last_channel_limit;
static atomic_t usb_lighting_allowed;
static bool usb_host_seen;
static bool usb_configured;
static bool usb_suspended;

USBD_DEVICE_DEFINE(headset_usb, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
                   CONFIG_HEADSET_USB_VID, CONFIG_HEADSET_USB_PID);
USBD_DESC_LANG_DEFINE(lang);
USBD_DESC_PRODUCT_DEFINE(product, "Headphones RGB");
USBD_CONFIGURATION_DEFINE(usb_config, USB_SCD_SELF_POWERED, 250, NULL);

static const uint8_t report_descriptor[] = {RGB_HID_DESCRIPTOR_BYTES};

static int receive_report(const struct device *dev, uint8_t type, uint8_t id,
                          uint16_t len, const uint8_t *data)
{
    (void)dev;
    if (type != HID_REPORT_TYPE_FEATURE || id != RGB_REPORT_ID) {
        return -EINVAL;
    }

    const uint32_t now = k_uptime_get_32();
    k_spinlock_key_t key = k_spin_lock(&frame_lock);
    const int status = rgb_frame_accept(&staged, usb_pixels.data, data, len, now);
    if (status > 0) {
        usb_pixels.received_at = now;
        usb_pixels.valid = true;
        usb_pixels.pending = true;
    }
    k_spin_unlock(&frame_lock, key);
    return status < 0 ? status : 0;
}

static void usb_ready(const struct device *dev, bool ready)
{
    (void)dev;
    if (!ready) {
        k_spinlock_key_t key = k_spin_lock(&frame_lock);
        staged.have_left = false;
        usb_pixels.valid = false;
        k_spin_unlock(&frame_lock, key);
    }
}

void rgb_receive_wireless(const uint8_t *pixels)
{
    const uint32_t now = k_uptime_get_32();
    k_spinlock_key_t key = k_spin_lock(&frame_lock);
    memcpy(wireless_pixels.data, pixels, RGB_FRAME_BYTES);
    wireless_pixels.received_at = now;
    wireless_pixels.valid = true;
    wireless_pixels.pending = true;
    k_spin_unlock(&frame_lock, key);
}

void rgb_clear_wireless(void)
{
    k_spinlock_key_t key = k_spin_lock(&frame_lock);
    wireless_pixels.valid = false;
    k_spin_unlock(&frame_lock, key);
}

static const struct hid_device_ops hid_ops = {
    .iface_ready = usb_ready,
    .set_report = receive_report,
};

static int charger_update(void)
{
    const bool standby = usb_host_seen && usb_suspended;
    const bool enable_ilim = usb_configured || standby;
    int status;

    if (enable_ilim) {
        status = gpio_pin_set_raw(gpio1, CHARGER_EN2_PIN, 1);
        if (status == 0) {
            status = gpio_pin_set_raw(gpio, CHARGER_EN1_PIN, standby);
        }
    } else {
        status = gpio_pin_set_raw(gpio, CHARGER_EN1_PIN, 0);
        if (status == 0) {
            status = gpio_pin_set_raw(gpio1, CHARGER_EN2_PIN, 0);
        }
    }
    return status;
}

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
        usb_host_seen = false;
        usb_configured = false;
        usb_suspended = false;
        break;
    case USBD_MSG_RESET:
        usb_host_seen = true;
        usb_configured = false;
        usb_suspended = false;
        break;
    case USBD_MSG_CONFIGURATION:
        usb_host_seen = true;
        usb_configured = msg->status != 0;
        break;
    case USBD_MSG_SUSPEND:
        usb_suspended = true;
        break;
    case USBD_MSG_RESUME:
        usb_suspended = false;
        break;
    default:
        return;
    }
    if (status < 0) {
        printk("RGB USB VBUS event failed: %d\n", status);
    }
    status = charger_update();
    atomic_set(&usb_lighting_allowed, status == 0 && usb_configured && !usb_suspended);
    if (status < 0) {
        printk("Charger mode update failed: %d\n", status);
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
    int status = gpio_pin_configure(gpio, CHARGER_EN1_PIN, GPIO_OUTPUT_INACTIVE);
    if (status == 0) {
        status = gpio_pin_configure(gpio1, CHARGER_EN2_PIN, GPIO_OUTPUT_INACTIVE);
    }
    if (status == 0) {
        status = hid_device_register(hid, report_descriptor, sizeof(report_descriptor), &hid_ops);
    }
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

static int send_pixels(const uint8_t *rgb, uint32_t sum, uint32_t limit)
{
    const uint8_t channel_order[] = {1, 0, 2};
    uint8_t *out = wave;
    for (size_t pixel = 0; pixel < RGB_FRAME_BYTES; pixel += RGB_CHANNELS) {
        for (size_t channel = 0; channel < RGB_CHANNELS; ++channel) {
            uint8_t value = rgb[pixel + channel_order[channel]];
            if (sum > limit) {
                value = (uint32_t)value * limit / sum;
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

void rgb_poll(bool external_power)
{
    if (!device_is_ready(spi)) {
        return;
    }
    const uint32_t limit = !external_power ? CHANNEL_SUM_LIMIT :
        (atomic_get(&usb_lighting_allowed) ? EXTERNAL_POWER_CHANNEL_SUM_LIMIT : 0U);
    if (limit == 0U) {
        turn_off();
        last_channel_limit = 0U;
        return;
    }
    uint8_t pixels[RGB_FRAME_BYTES];
    k_spinlock_key_t key = k_spin_lock(&frame_lock);
    const uint32_t now = k_uptime_get_32();
    usb_pixels.valid &= now - usb_pixels.received_at < RGB_TIMEOUT_MS;
    wireless_pixels.valid &= now - wireless_pixels.received_at < RGB_TIMEOUT_MS;
    struct pixels *const source = usb_pixels.valid ? &usb_pixels : &wireless_pixels;
    const bool active = source->valid;
    const bool ready = active && (source->pending || limit != last_channel_limit);
    if (ready) {
        memcpy(pixels, source->data, sizeof(pixels));
        source->pending = false;
        (source == &usb_pixels ? &wireless_pixels : &usb_pixels)->pending = true;
    }
    k_spin_unlock(&frame_lock, key);
    if (!active) {
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
    if (send_pixels(pixels, sum, limit) < 0) {
        turn_off();
        printk("RGB SPI transfer failed\n");
        return;
    }
    last_channel_limit = limit;
}
