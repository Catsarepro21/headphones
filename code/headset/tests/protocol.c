#include "bm83_protocol.h"
#include "headset_logic.h"

#include <assert.h>
#include <string.h>

static void test_frames(void)
{
    const uint8_t example[] = {0xaa, 0x00, 0x02, 0x14, 0x33, 0xb7};
    const uint8_t event = 0x33;
    uint8_t packet[BM83_MAX_BODY + 4U];
    assert(bm83_encode(packet, sizeof(packet), BM83_CMD_EVENT_ACK, &event, 1) ==
           sizeof(example));
    assert(memcmp(packet, example, sizeof(example)) == 0);
    assert(bm83_encode(packet, 5, BM83_CMD_EVENT_ACK, &event, 1) == 0);

    struct bm83_parser parser = {0};
    assert(!bm83_parse_byte(&parser, 0x12));
    assert(!bm83_parse_byte(&parser, 0xaa));
    assert(!bm83_parse_byte(&parser, 0x03));
    assert(!bm83_parse_byte(&parser, 0x00));
    for (size_t i = 0; i < sizeof(example) - 1U; ++i) {
        assert(!bm83_parse_byte(&parser, example[i]));
    }
    assert(!bm83_parse_byte(&parser, BM83_START));
    for (size_t i = 1; i < sizeof(example) - 1U; ++i) {
        assert(!bm83_parse_byte(&parser, example[i]));
    }
    assert(bm83_parse_byte(&parser, example[sizeof(example) - 1U]));
    assert(parser.length == 2U);
    assert(parser.body[0] == BM83_CMD_EVENT_ACK && parser.body[1] == event);

    uint8_t payload[BM83_MAX_BODY - 1U];
    memset(payload, BM83_START, sizeof(payload));
    const size_t size = bm83_encode(packet, sizeof(packet), 0x18, payload, sizeof(payload));
    assert(size == sizeof(packet));
    for (size_t i = 0; i < size - 1U; ++i) {
        assert(!bm83_parse_byte(&parser, packet[i]));
    }
    assert(bm83_parse_byte(&parser, packet[size - 1U]));
    assert(parser.length == BM83_MAX_BODY);
    assert(memcmp(parser.body + 1, payload, sizeof(payload)) == 0);
}

static void test_buttons(void)
{
    struct headset_inputs inputs = {0};
    assert(headset_inputs_update(&inputs, true, true, 0) == HEADSET_MODE_CHANGED);
    assert(headset_inputs_update(&inputs, true, false, 5) == 0);
    assert(headset_inputs_update(&inputs, true, false, 30) == 0);
    assert(headset_inputs_update(&inputs, false, true, 35) == 0);
    assert(headset_inputs_update(&inputs, true, false, 40) == 0);
    assert(headset_inputs_update(&inputs, false, true, 45) == 0);
    assert(headset_inputs_update(&inputs, false, true, 69) == 0);
    assert(headset_inputs_update(&inputs, false, true, 70) ==
           (HEADSET_MODE_CHANGED | HEADSET_PAIR_PRESSED));
    assert(headset_inputs_update(&inputs, false, true, 100) == 0);

    struct headset_input button = {0};
    assert(!headset_button_pressed(&button, false, UINT32_MAX - 10));
    assert(!headset_button_pressed(&button, true, UINT32_MAX - 5));
    assert(!headset_button_pressed(&button, true, 18));
    assert(headset_button_pressed(&button, true, 19));
}

int main(void)
{
    test_frames();
    test_buttons();
    return 0;
}
