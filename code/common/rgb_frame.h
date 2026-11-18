#ifndef HEADPHONES_RGB_FRAME_H
#define HEADPHONES_RGB_FRAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    RGB_REPORT_ID = 1,
    RGB_CHANNELS = 3,
    RGB_PIXELS_PER_CUP = 16,
    RGB_CUP_BYTES = RGB_PIXELS_PER_CUP * RGB_CHANNELS,
    RGB_FRAME_BYTES = 2 * RGB_CUP_BYTES,
    RGB_REPORT_HEADER = 4,
    RGB_REPORT_BYTES = RGB_REPORT_HEADER + RGB_CUP_BYTES,
    RGB_COMMAND_FRAME = 0x52,
    RGB_RADIO_BYTES = 1 + RGB_FRAME_BYTES,
    RGB_TIMEOUT_MS = 1000,
};

#define RGB_HID_DESCRIPTOR_BYTES \
    0x06, 0x31, 0xff, 0x09, 0x01, 0xa1, 0x01, 0x85, RGB_REPORT_ID, \
    0x15, 0x00, 0x26, 0xff, 0x00, 0x75, 0x08, 0x95, RGB_REPORT_BYTES - 1, \
    0x09, 0x02, 0xb1, 0x02, 0xc0

struct rgb_frame {
    uint8_t left[RGB_CUP_BYTES];
    uint8_t sequence;
    uint32_t left_at;
    bool have_left;
};

int rgb_frame_accept(struct rgb_frame *frame, uint8_t *pixels, const uint8_t *data,
                     size_t length, uint32_t now);

#endif
