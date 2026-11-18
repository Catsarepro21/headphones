#include "usb_audio.h"
#include "usb_rgb.h"
#include "audio_control.h"
#include "audio_fifo.h"

#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/usb/class/usbd_hid.h>
#include <zephyr/usb/class/usbd_uac2.h>
#include <zephyr/usb/usbd.h>

enum {
    PLAY_BYTES = AUDIO_USB_FRAMES * AUDIO_STEREO * sizeof(int16_t),
    MIC_BYTES = AUDIO_USB_FRAMES * AUDIO_MONO * sizeof(int16_t),
};

AUDIO_FIFO_DEFINE(playback, AUDIO_STEREO);
AUDIO_FIFO_DEFINE(microphone, AUDIO_MONO);
K_MEM_SLAB_DEFINE_STATIC(out_buffers, ROUND_UP(PLAY_BYTES, UDC_BUF_GRANULARITY),
                         5, UDC_BUF_ALIGN);
K_MEM_SLAB_DEFINE_STATIC(in_buffers, ROUND_UP(MIC_BYTES, UDC_BUF_GRANULARITY),
                         5, UDC_BUF_ALIGN);
K_MSGQ_DEFINE(buttons, sizeof(uint8_t), 8, 1);
static atomic_t mic_enabled;
static atomic_t media_enabled;
static atomic_t media_pressed;
static atomic_t media_in_flight;
static uint8_t media_report[2] __aligned(UDC_BUF_ALIGN);
static atomic_t suspended;
static const struct device *const audio = DEVICE_DT_GET(DT_NODELABEL(usb_audio));
static const struct device *const hid = DEVICE_DT_GET(DT_NODELABEL(media_hid));

USBD_DEVICE_DEFINE(dongle_usb, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
                   CONFIG_DONGLE_USB_VID, CONFIG_DONGLE_USB_PID);
USBD_DESC_LANG_DEFINE(language);
USBD_DESC_PRODUCT_DEFINE(product, "Headphones Dongle");
USBD_CONFIGURATION_DEFINE(configuration, 0, 50, NULL);

static const uint8_t media_descriptor[] = {
    0x05, 0x0c, 0x09, 0x01, 0xa1, 0x01,
    0x15, 0x00, 0x26, 0xff, 0x03,
    0x19, 0x00, 0x2a, 0xff, 0x03,
    0x75, 0x10, 0x95, 0x01, 0x81, 0x00, 0xc0,
};

static void media_ready(const struct device *dev, bool enabled)
{
    ARG_UNUSED(dev);
    atomic_set(&media_enabled, enabled);
    if (!enabled) {
        atomic_clear(&media_pressed);
    }
}

static void media_sent(const struct device *dev, const uint8_t *report)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(report);
    atomic_clear(&media_in_flight);
}

static const struct hid_device_ops media_ops = {
    .iface_ready = media_ready,
    .input_report_done = media_sent,
};

static void terminal_update(const struct device *dev, uint8_t terminal, bool enabled,
                            bool microframes, void *user)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(microframes);
    ARG_UNUSED(user);
    if (terminal == UAC2_ENTITY_ID(DT_NODELABEL(usb_mic))) {
        atomic_set(&mic_enabled, enabled);
        audio_fifo_clear(&microphone);
    } else if (terminal == UAC2_ENTITY_ID(DT_NODELABEL(usb_play))) {
        audio_fifo_clear(&playback);
    }
}

static void *out_buffer(const struct device *dev, uint8_t terminal, uint16_t size, void *user)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(terminal);
    ARG_UNUSED(user);
    void *buffer;
    if (size > PLAY_BYTES || k_mem_slab_alloc(&out_buffers, &buffer, K_NO_WAIT) != 0) {
        return NULL;
    }
    return buffer;
}

static void received(const struct device *dev, uint8_t terminal, void *buffer, uint16_t size,
                     void *user)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(terminal);
    ARG_UNUSED(user);
    if (size != 0U && size <= PLAY_BYTES && size % (AUDIO_STEREO * sizeof(int16_t)) == 0U) {
        audio_fifo_write(&playback, buffer, size / (AUDIO_STEREO * sizeof(int16_t)));
    }
    k_mem_slab_free(&out_buffers, buffer);
}

static void capture_released(const struct device *dev, uint8_t terminal, void *buffer, void *user)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(terminal);
    ARG_UNUSED(user);
    k_mem_slab_free(&in_buffers, buffer);
}

static void sof(const struct device *dev, void *user)
{
    ARG_UNUSED(user);
    void *buffer;
    if (!usb_mic_enabled() || k_mem_slab_alloc(&in_buffers, &buffer, K_NO_WAIT) != 0) {
        return;
    }
    audio_fifo_play(&microphone, buffer, AUDIO_USB_FRAMES);
    if (usbd_uac2_send(dev, UAC2_ENTITY_ID(DT_NODELABEL(usb_mic)), buffer, MIC_BYTES) < 0) {
        k_mem_slab_free(&in_buffers, buffer);
    }
}

static const struct uac2_ops audio_ops = {
    .sof_cb = sof,
    .terminal_update_cb = terminal_update,
    .get_recv_buf = out_buffer,
    .data_recv_cb = received,
    .buf_release_cb = capture_released,
};

static void usb_event(struct usbd_context *context, const struct usbd_msg *message)
{
    if (message->type == USBD_MSG_VBUS_READY) {
        atomic_clear(&suspended);
        (void)usbd_enable(context);
    } else if (message->type == USBD_MSG_VBUS_REMOVED) {
        atomic_clear(&mic_enabled);
        atomic_clear(&media_enabled);
        atomic_clear(&media_pressed);
        atomic_clear(&suspended);
        usb_clear_audio();
        (void)usbd_disable(context);
    } else if (message->type == USBD_MSG_SUSPEND) {
        atomic_set(&suspended, 1);
        usb_clear_audio();
    } else if (message->type == USBD_MSG_RESUME) {
        atomic_clear(&suspended);
    } else if (message->type == USBD_MSG_RESET) {
        atomic_clear(&mic_enabled);
        atomic_clear(&media_enabled);
        atomic_clear(&media_pressed);
        atomic_clear(&suspended);
        usb_clear_audio();
    }
}

int usb_audio_start(void)
{
    if (CONFIG_DONGLE_USB_VID == 0 || CONFIG_DONGLE_USB_PID == 0) {
        return -ENOTSUP;
    }
    if (!device_is_ready(audio) || !device_is_ready(hid)) {
        return -ENODEV;
    }
    usbd_uac2_set_ops(audio, &audio_ops, NULL);
    int status = hid_device_register(hid, media_descriptor, sizeof(media_descriptor), &media_ops);
    if (status == 0) {
        status = usb_rgb_register();
    }
    if (status == 0) {
        status = usbd_add_descriptor(&dongle_usb, &language);
    }
    if (status == 0) {
        status = usbd_add_descriptor(&dongle_usb, &product);
    }
    if (status == 0) {
        status = usbd_add_configuration(&dongle_usb, USBD_SPEED_FS, &configuration);
    }
    if (status == 0) {
        status = usbd_register_all_classes(&dongle_usb, USBD_SPEED_FS, 1, NULL);
    }
    if (status == 0) {
        status = usbd_device_set_code_triple(&dongle_usb, USBD_SPEED_FS,
                                             USB_BCC_MISCELLANEOUS, 0x02, 0x01);
    }
    if (status == 0) {
        status = usbd_msg_register_cb(&dongle_usb, usb_event);
    }
    if (status == 0) {
        status = usbd_init(&dongle_usb);
    }
    if (status == 0 && !usbd_can_detect_vbus(&dongle_usb)) {
        status = usbd_enable(&dongle_usb);
    }
    return status;
}

bool usb_read_playback(int16_t *pcm)
{
    return audio_fifo_read(&playback, pcm, AUDIO_FRAMES);
}

bool usb_mic_enabled(void)
{
    return atomic_get(&mic_enabled) != 0 && !usb_suspended();
}

bool usb_suspended(void)
{
    return atomic_get(&suspended) != 0;
}

void usb_receive_mic(const int16_t *pcm)
{
    if (usb_mic_enabled()) {
        audio_fifo_write(&microphone, pcm, AUDIO_FRAMES);
    }
}

void usb_clear_audio(void)
{
    audio_fifo_clear(&playback);
    audio_fifo_clear(&microphone);
    k_msgq_purge(&buttons);
}

void usb_media_enqueue(uint8_t usage)
{
    if (atomic_get(&media_enabled) && !usb_suspended() &&
        (usage == AUDIO_PLAY_PAUSE || usage == AUDIO_VOLUME_UP || usage == AUDIO_VOLUME_DOWN)) {
        (void)k_msgq_put(&buttons, &usage, K_NO_WAIT);
    }
}

void usb_media_poll(void)
{
    if (!atomic_get(&media_enabled) || usb_suspended() || atomic_get(&media_in_flight)) {
        return;
    }
    const bool release = atomic_get(&media_pressed) != 0;
    uint8_t usage = 0;
    if (!release && k_msgq_get(&buttons, &usage, K_NO_WAIT) != 0) {
        return;
    }
    media_report[0] = usage;
    atomic_set(&media_in_flight, 1);
    if (hid_device_submit_report(hid, sizeof(media_report), media_report) == 0) {
        atomic_set(&media_pressed, !release);
    } else {
        atomic_clear(&media_in_flight);
    }
}
