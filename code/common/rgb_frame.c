#include "rgb_frame.h"

#include <errno.h>
#include <string.h>

int rgb_frame_accept(struct rgb_frame *frame, uint8_t *pixels, const uint8_t *data,
                     size_t length, uint32_t now)
{
    if (length != RGB_REPORT_BYTES || data[0] != RGB_REPORT_ID ||
        data[1] != RGB_COMMAND_FRAME || data[3] > 1U) {
        return -EINVAL;
    }
    if (data[3] == 0U) {
        memcpy(frame->left, data + RGB_REPORT_HEADER, RGB_CUP_BYTES);
        frame->sequence = data[2];
        frame->left_at = now;
        frame->have_left = true;
        return 0;
    }
    if (!frame->have_left || frame->sequence != data[2] ||
        now - frame->left_at >= RGB_TIMEOUT_MS) {
        return 0;
    }
    memcpy(pixels, frame->left, RGB_CUP_BYTES);
    memcpy(pixels + RGB_CUP_BYTES, data + RGB_REPORT_HEADER, RGB_CUP_BYTES);
    frame->have_left = false;
    return 1;
}
