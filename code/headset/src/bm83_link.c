#include "bm83_link.h"
#include "bm83_protocol.h"

#include <errno.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/ring_buffer.h>

enum {
    PIN_MFB = 6,
    ACK_MS = 200,
    RESPONSE_MS = 800,
    POWER_ON_MS = 2000,
    PAIR_MS = 30000,
    RX_GAP_MS = 30,
    MAX_ATTEMPTS = 3,
};

static const struct device *const serial = DEVICE_DT_GET(DT_NODELABEL(uart0));
static const struct device *const mfb_gpio = DEVICE_DT_GET(DT_NODELABEL(gpio1));
RING_BUF_DECLARE(received, 1024);
K_MSGQ_DEFINE(media_queue, sizeof(enum bm83_media_control), 8, 1);
static atomic_t rx_overflow;
static struct bm83_parser parser;
static uint32_t last_rx;
static uint32_t sent_at;
static uint32_t pair_started;
static uint32_t power_started;
static uint8_t awaiting;
static uint8_t action;
static uint8_t attempts;
static uint8_t handsfree_links;
static uint8_t hf_device[2];
static uint8_t media_links;
static uint8_t call_link = UINT8_MAX;
static bool command_pending;
static bool command_acknowledged;
static bool selected;
static bool probe_needed;
static bool responsive;
static bool pair_requested;
static bool pairing;
static bool exit_pairing;
static bool power_release_needed;
static bool power_requested;
static bool power_starting;
static bool powered;

static void command_failed(void)
{
    command_pending = false;
    if (awaiting == BM83_CMD_VERSION ||
        (awaiting == BM83_CMD_MMI &&
         (action == BM83_ACTION_POWER_PRESS || action == BM83_ACTION_POWER_RELEASE))) {
        pair_requested = false;
        power_requested = false;
    }
}

static void receive_irq(const struct device *dev, void *context)
{
    uint8_t bytes[32];
    (void)context;

    if (!uart_irq_update(dev) || !uart_irq_rx_ready(dev)) {
        return;
    }
    int count;
    while ((count = uart_fifo_read(dev, bytes, sizeof(bytes))) > 0) {
        if (ring_buf_put(&received, bytes, (uint32_t)count) != (uint32_t)count) {
            atomic_set(&rx_overflow, 1);
        }
    }
}

static int send_frame(uint8_t command, const uint8_t *params, size_t count)
{
    uint8_t frame[12];
    const size_t length = bm83_encode(frame, sizeof(frame), command, params, count);
    if (length == 0U) {
        return -EINVAL;
    }
    int status = gpio_pin_set(mfb_gpio, PIN_MFB, 1);
    if (status < 0) {
        return status;
    }
    k_busy_wait(2500);
    for (size_t i = 0; i < length; ++i) {
        uart_poll_out(serial, frame[i]);
    }
    k_busy_wait(100);
    return gpio_pin_set(mfb_gpio, PIN_MFB, 0);
}

static void send_pending(void)
{
    uint8_t params[6] = {0, action};
    size_t count = awaiting == BM83_CMD_VERSION ? 1U : 2U;
    if (awaiting == BM83_CMD_SET_GAIN) {
        params[0] = call_link < 2U ? call_link : (media_links & 1U ? 0U : 1U);
        params[1] = call_link < 2U ? 0x02 : 0x01;
        params[2] = action;
        count = sizeof(params);
    }
    const int status = send_frame(awaiting, params, count);
    sent_at = k_uptime_get_32();
    ++attempts;
    command_pending = status == 0;
    command_acknowledged = false;
    if (status < 0) {
        command_failed();
        printk("BM83 transmit GPIO error: %d\n", status);
    }
}

static void queue_command(uint8_t command, uint8_t mmi_action)
{
    awaiting = command;
    action = mmi_action;
    attempts = 0;
    send_pending();
}

static void handle_event(uint32_t now)
{
    const uint8_t event = parser.body[0];

    if (event == BM83_EVENT_COMMAND_ACK) {
        if (parser.length < 3U || !command_pending || parser.body[1] != awaiting) {
            return;
        }
        if (parser.body[2] != 0U) {
            printk("BM83 rejected 0x%02x: 0x%02x\n", awaiting, parser.body[2]);
            command_failed();
        } else if (awaiting == BM83_CMD_MMI) {
            command_pending = false;
            switch (action) {
            case BM83_ACTION_POWER_PRESS:
                power_release_needed = true;
                break;
            case BM83_ACTION_POWER_RELEASE:
                power_starting = !powered;
                power_started = now;
                printk("BM83 accepted power-on sequence\n");
                break;
            default:
                pairing = action == BM83_ACTION_PAIR;
                pair_started = now;
                exit_pairing = pairing && !selected;
                printk("BM83 accepted pairing %s\n", pairing ? "start" : "exit");
                break;
            }
        } else if (awaiting == BM83_CMD_VERSION) {
            command_acknowledged = true;
            sent_at = now;
        } else {
            command_pending = false;
        }
        return;
    }

    if (send_frame(BM83_CMD_EVENT_ACK, &event, 1) < 0) {
        printk("BM83 event acknowledgement failed\n");
    }
    if (event == BM83_EVENT_VERSION && parser.length >= 4U && parser.body[1] == 0U) {
        responsive = true;
        if (command_pending && awaiting == BM83_CMD_VERSION) {
            command_pending = false;
        }
        printk("BM83 UART version reply received\n");
    } else if (event == BM83_EVENT_STATUS && parser.length >= 2U) {
        const uint8_t link = parser.length >= 3U ? parser.body[2] & 0x0fU : UINT8_MAX;
        const uint8_t mask = link < 2U ? 1U << link : 0U;
        printk("BM83 status 0x%02x\n", parser.body[1]);
        switch (parser.body[1]) {
        case 0x00:
            powered = false;
            pairing = false;
            handsfree_links = 0U;
            media_links = 0U;
            call_link = UINT8_MAX;
            break;
        case 0x01:
            powered = true;
            if (!pairing) {
                pair_started = now;
            }
            pairing = true;
            exit_pairing |= !selected;
            break;
        case 0x02:
            powered = true;
            break;
        case 0x03:
        case 0x04:
            pairing = false;
            exit_pairing = false;
            break;
        case 0x05:
            handsfree_links |= mask;
            if (mask != 0U) {
                hf_device[link] = parser.body[2] >> 4;
            }
            powered = true;
            break;
        case 0x06:
            media_links |= mask;
            powered = true;
            break;
        case 0x07:
            handsfree_links &= ~mask;
            if (call_link == link) {
                call_link = UINT8_MAX;
            }
            break;
        case 0x08:
            media_links &= ~mask;
            break;
        case 0x09:
            for (uint8_t index = 0; index < 2U; ++index) {
                if ((handsfree_links & (1U << index)) != 0U && hf_device[index] == link) {
                    call_link = index;
                }
            }
            break;
        case 0x0a:
            call_link = UINT8_MAX;
            break;
        default:
            break;
        }
    }
}

int bm83_start(void)
{
    if (!device_is_ready(serial) || !device_is_ready(mfb_gpio)) {
        return -ENODEV;
    }
    int status = gpio_pin_configure(mfb_gpio, PIN_MFB, GPIO_OUTPUT_INACTIVE);
    if (status < 0) {
        return status;
    }
    status = uart_irq_callback_user_data_set(serial, receive_irq, NULL);
    if (status < 0) {
        return status;
    }
    uart_irq_rx_enable(serial);
    return 0;
}

void bm83_set_selected(bool enabled)
{
    selected = enabled;
    if (enabled && !responsive && !command_pending) {
        probe_needed = true;
    }
    power_requested = enabled && !powered;
    if (!enabled) {
        pair_requested = false;
        exit_pairing |= pairing;
        k_msgq_purge(&media_queue);
    }
}

void bm83_request_media(enum bm83_media_control control)
{
    if (selected && powered) {
        (void)k_msgq_put(&media_queue, &control, K_NO_WAIT);
    }
}

void bm83_request_pairing(void)
{
    if (selected) {
        pair_requested = true;
        power_requested |= !powered;
        probe_needed |= !responsive && !command_pending;
    }
}

bool bm83_audio_available(void)
{
    return selected && powered;
}

void bm83_poll(void)
{
    uint32_t now = k_uptime_get_32();
    uint8_t byte;

    if (atomic_cas(&rx_overflow, 1, 0) || now - last_rx > RX_GAP_MS) {
        parser.count = 0;
    }
    while (ring_buf_get(&received, &byte, 1) == 1U) {
        last_rx = now;
        if (bm83_parse_byte(&parser, byte)) {
            handle_event(now);
        }
    }

    now = k_uptime_get_32();
    if (power_starting && (powered || now - power_started >= POWER_ON_MS)) {
        power_starting = false;
        if (!powered) {
            pair_requested = false;
            printk("BM83 did not report power on\n");
        }
    }
    if (command_pending) {
        const uint32_t wait = command_acknowledged ? RESPONSE_MS : ACK_MS;
        if (now - sent_at < wait) {
            return;
        }
        if (attempts < MAX_ATTEMPTS &&
            (awaiting == BM83_CMD_VERSION || awaiting == BM83_CMD_MMI)) {
            send_pending();
            return;
        }
        printk("BM83 command 0x%02x timed out\n", awaiting);
        command_failed();
    }
    if (power_release_needed) {
        power_release_needed = false;
        queue_command(BM83_CMD_MMI, BM83_ACTION_POWER_RELEASE);
    } else if (probe_needed && selected) {
        probe_needed = false;
        queue_command(BM83_CMD_VERSION, 0);
    } else if (exit_pairing || (pairing && now - pair_started >= PAIR_MS)) {
        exit_pairing = false;
        pairing = false;
        queue_command(BM83_CMD_MMI, BM83_ACTION_EXIT_PAIR);
    } else if (selected && responsive && power_requested && !power_starting) {
        power_requested = false;
        if (!powered) {
            queue_command(BM83_CMD_MMI, BM83_ACTION_POWER_PRESS);
        }
    } else if (selected && responsive && powered && pair_requested) {
        pair_requested = false;
        queue_command(BM83_CMD_MMI, BM83_ACTION_PAIR);
    } else if (selected && powered) {
        enum bm83_media_control control;
        if (k_msgq_get(&media_queue, &control, K_NO_WAIT) != 0) {
            return;
        }
        if (control == BM83_MEDIA_PLAY_PAUSE && media_links != 0U && call_link == UINT8_MAX) {
            queue_command(BM83_CMD_MUSIC_CONTROL, 0x07);
        } else if (control != BM83_MEDIA_PLAY_PAUSE &&
                   (media_links != 0U || call_link < 2U)) {
            queue_command(BM83_CMD_SET_GAIN, control == BM83_MEDIA_VOLUME_UP ? 0x01 : 0x02);
        }
    }
}
