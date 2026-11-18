#include "bm83_protocol.h"

#include <string.h>

bool bm83_parse_byte(struct bm83_parser *parser, uint8_t byte)
{
    switch (parser->count) {
    case 0:
        if (byte == BM83_START) {
            parser->count = 1;
        }
        return false;
    case 1:
        parser->length = (uint16_t)byte << 8;
        parser->checksum = byte;
        break;
    case 2:
        parser->length |= byte;
        if (parser->length == 0U || parser->length > sizeof(parser->body)) {
            parser->count = byte == BM83_START ? 1U : 0U;
            return false;
        }
        break;
    default:
        if (parser->count == parser->length + 3U) {
            const bool valid = (uint8_t)(parser->checksum + byte) == 0U;
            parser->count = !valid && byte == BM83_START ? 1U : 0U;
            return valid;
        }
        parser->body[parser->count - 3U] = byte;
        break;
    }
    if (parser->count > 1U) {
        parser->checksum = (uint8_t)(parser->checksum + byte);
    }
    ++parser->count;
    return false;
}

size_t bm83_encode(uint8_t *out, size_t capacity, uint8_t command,
                   const uint8_t *params, size_t count)
{
    if (count >= BM83_MAX_BODY || capacity < count + 5U ||
        (count != 0U && params == NULL)) {
        return 0;
    }

    const uint16_t length = (uint16_t)(count + 1U);
    out[0] = BM83_START;
    out[1] = (uint8_t)(length >> 8);
    out[2] = (uint8_t)length;
    out[3] = command;
    if (count != 0U) {
        memcpy(&out[4], params, count);
    }

    uint8_t checksum = 0;
    for (size_t i = 1; i < count + 4U; ++i) {
        checksum = (uint8_t)(checksum + out[i]);
    }
    out[4U + count] = (uint8_t)(0U - checksum);
    return count + 5U;
}
