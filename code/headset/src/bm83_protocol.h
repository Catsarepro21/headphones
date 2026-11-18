#ifndef BM83_PROTOCOL_H
#define BM83_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    BM83_START = 0xaa,
    BM83_MAX_BODY = 512,
    BM83_EVENT_COMMAND_ACK = 0x00,
    BM83_EVENT_STATUS = 0x01,
    BM83_EVENT_VERSION = 0x18,
    BM83_CMD_MMI = 0x02,
    BM83_CMD_MUSIC_CONTROL = 0x04,
    BM83_CMD_VERSION = 0x08,
    BM83_CMD_EVENT_ACK = 0x14,
    BM83_CMD_SET_GAIN = 0x23,
    BM83_ACTION_POWER_PRESS = 0x51,
    BM83_ACTION_POWER_RELEASE = 0x52,
    BM83_ACTION_PAIR = 0x5d,
    BM83_ACTION_EXIT_PAIR = 0x6b,
};

struct bm83_parser {
    uint16_t count;
    uint16_t length;
    uint8_t checksum;
    uint8_t body[BM83_MAX_BODY];
};

bool bm83_parse_byte(struct bm83_parser *parser, uint8_t byte);
size_t bm83_encode(uint8_t *out, size_t capacity, uint8_t command,
                   const uint8_t *params, size_t count);

#endif
