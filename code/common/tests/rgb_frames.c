#include "rgb_frame.h"

#include <assert.h>
#include <errno.h>
#include <string.h>

int main(void)
{
    struct rgb_frame frame = {0};
    uint8_t rgb[RGB_FRAME_BYTES] = {0};
    uint8_t left[RGB_REPORT_BYTES] = {RGB_REPORT_ID, RGB_COMMAND_FRAME, 255, 0};
    uint8_t right[RGB_REPORT_BYTES] = {RGB_REPORT_ID, RGB_COMMAND_FRAME, 255, 1};
    memset(left + RGB_REPORT_HEADER, 16, RGB_CUP_BYTES);
    memset(right + RGB_REPORT_HEADER, 32, RGB_CUP_BYTES);

    assert(rgb_frame_accept(&frame, rgb, right, sizeof(right), 0) == 0);
    assert(rgb_frame_accept(&frame, rgb, left, 1, 0) == -EINVAL);
    assert(rgb_frame_accept(&frame, rgb, left, sizeof(left), UINT32_MAX - 1U) == 0);
    right[2] = 0;
    assert(rgb_frame_accept(&frame, rgb, right, sizeof(right), 1) == 0);
    right[2] = 255;
    assert(rgb_frame_accept(&frame, rgb, right, sizeof(right), 2) == 1);
    assert(memcmp(rgb, left + RGB_REPORT_HEADER, RGB_CUP_BYTES) == 0);
    assert(memcmp(rgb + RGB_CUP_BYTES, right + RGB_REPORT_HEADER, RGB_CUP_BYTES) == 0);
    assert(rgb_frame_accept(&frame, rgb, right, sizeof(right), 3) == 0);
    assert(rgb_frame_accept(&frame, rgb, left, sizeof(left), 50) == 0);
    assert(rgb_frame_accept(&frame, rgb, right, sizeof(right), 50 + RGB_TIMEOUT_MS) == 0);
    return 0;
}
