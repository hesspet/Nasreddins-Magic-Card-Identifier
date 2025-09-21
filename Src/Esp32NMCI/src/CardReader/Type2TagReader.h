#pragma once

#include <PN532.h>
#include <stddef.h>
#include <stdint.h>

class Type2TagReader
{
    public:
    struct TagInfo
    {
        bool ccValid = false;
        uint8_t ccMagic = 0x00;
        uint8_t verMaj = 0;
        uint8_t verMin = 0;
        uint8_t size8 = 0;
        uint8_t access = 0;
        uint16_t userBytes = 0;
        uint16_t userPages = 0;
        const char *probableType = "Unknown Type 2";
    };

    explicit Type2TagReader(PN532 &nfc);

    bool readCapabilityContainer(TagInfo &out) const;
    bool readUserMemory(uint8_t *buffer, size_t bufferCap, const TagInfo &info) const;
    static bool isSameUid(const uint8_t *lhs, const uint8_t *rhs, uint8_t len);

    private:
    static constexpr uint8_t kBytesPerPage = 4;
    static constexpr uint8_t kFirstUserPage = 4;

    PN532 &nfc_;
};
