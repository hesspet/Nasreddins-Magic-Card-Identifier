#include "Utf8Decoder.h"

Utf8Decoder gUtf;

bool Utf8Decoder::feed(uint8_t byte, uint32_t& out) {
    if (needed == 0) {
        if (byte < 0x80) {
            out = byte;
            return true;
        }
        if ((byte & 0xE0) == 0xC0) {
            codepoint = byte & 0x1F;
            needed = 1;
            return false;
        }
        if ((byte & 0xF0) == 0xE0) {
            codepoint = byte & 0x0F;
            needed = 2;
            return false;
        }
        if ((byte & 0xF8) == 0xF0) {
            codepoint = byte & 0x07;
            needed = 3;
            return false;
        }
        return false;
    }

    if ((byte & 0xC0) != 0x80) {
        needed = 0;
        return false;
    }
    codepoint = (codepoint << 6) | (byte & 0x3F);
    if (--needed == 0) {
        out = codepoint;
        return true;
    }
    return false;
}
