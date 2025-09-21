#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

class NdefHelper
{
    public:
    struct Tlv
    {
        uint8_t tag = 0;
        const uint8_t *value = nullptr;
        size_t length = 0;
        const uint8_t *next = nullptr;
    };

    struct NdefRecord
    {
        uint8_t tnf = 0;
        bool mb = false;
        bool me = false;
        bool sr = false;
        bool il = false;
        uint8_t typeLen = 0;
        uint32_t payloadLen = 0;
        uint8_t idLen = 0;
        const uint8_t *type = nullptr;
        const uint8_t *id = nullptr;
        const uint8_t *payload = nullptr;
    };

    bool findFirstNdefTlv(const uint8_t *data, size_t length, Tlv &out) const;
    bool parseFirstRecord(const uint8_t *msg, size_t msgLen, NdefRecord &rec) const;
    bool isTextRecord(const NdefRecord &rec) const;
    void decodeAndPrintTextRecord(const NdefRecord &rec) const;
    String getString(const uint8_t *data, size_t len) const;

    private:
    bool parseNextTlv(const uint8_t *start, const uint8_t *end, Tlv &out) const;
};
