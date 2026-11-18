#include "usb_rgb.h"
#include "radio.h"
#include "rgb_frame.h"

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/usb/class/usbd_hid.h>

enum { MIN_FRAME_INTERVAL_MS = 33 };

static struct k_spinlock lock;
static struct rgb_frame staged;
static uint8_t pixels[RGB_FRAME_BYTES];
static bool pending;
static uint32_t revision;
static uint32_t last_frame;
static uint32_t last_send;

static void interface_ready(const struct device *dev, bool ready)
{
    ARG_UNUSED(dev);
    if (!ready) {
        k_spinlock_key_t key = k_spin_lock(&lock);
        pending = false;
        staged.have_left = false;
        k_spin_unlock(&lock, key);
    }
}

static int set_report(const struct device *dev, uint8_t type, uint8_t id,
                      uint16_t length, const uint8_t *data)
{
    ARG_UNUSED(dev);
    if (type != HID_REPORT_TYPE_FEATURE || id != RGB_REPORT_ID) {
        return -EINVAL;
    }
    const uint32_t now = k_uptime_get_32();
    k_spinlock_key_t key = k_spin_lock(&lock);
    const int status = rgb_frame_accept(&staged, pixels, data, length, now);
    if (status > 0) {
        ++revision;
        last_frame = now;
        pending = true;
    }
    k_spin_unlock(&lock, key);
    return status < 0 ? status : 0;
}

int usb_rgb_register(void)
{
    const struct device *const hid = DEVICE_DT_GET(DT_NODELABEL(rgb_hid));
    static const struct hid_device_ops ops = {
        .iface_ready = interface_ready,
        .set_report = set_report,
    };
    static const uint8_t descriptor[] = {RGB_HID_DESCRIPTOR_BYTES};
    if (!device_is_ready(hid)) {
        return -ENODEV;
    }
    return hid_device_register(hid, descriptor, sizeof(descriptor), &ops);
}

void usb_rgb_poll(bool linked)
{
    const uint32_t now = k_uptime_get_32();
    if (!linked || now - last_send < MIN_FRAME_INTERVAL_MS) {
        return;
    }
    uint8_t packet[RGB_RADIO_BYTES] = {RGB_COMMAND_FRAME};
    k_spinlock_key_t key = k_spin_lock(&lock);
    pending &= now - last_frame < RGB_TIMEOUT_MS;
    const bool ready = pending;
    const uint32_t sending = revision;
    if (ready) {
        memcpy(packet + 1, pixels, sizeof(pixels));
    }
    k_spin_unlock(&lock, key);

    if (ready && radio_send(packet, sizeof(packet)) == 0) {
        last_send = now;
        key = k_spin_lock(&lock);
        if (revision == sending) {
            pending = false;
        }
        k_spin_unlock(&lock, key);
    }
}
