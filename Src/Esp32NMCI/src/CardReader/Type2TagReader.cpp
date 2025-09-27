/***************************************************************************
 * Project: Esp32NMCI
 * File: Type2TagReader.cpp
 * Repository: https://github.com/hesspet/NasrredinsMagicCardIdentifier
 * Author: Peter Heß, Büdingen DE
 ***************************************************************************/

#include "Type2TagReader.h"

#include <string.h>

Type2TagReader::Type2TagReader(PN532 &nfc)
    : nfc_(nfc)
{
}

bool Type2TagReader::readCapabilityContainer(TagInfo &out) const
{
    uint8_t page3[4] = {0};
    if (!nfc_.mifareultralight_ReadPage(3, page3))
    {
        return false;
    }

    out.ccMagic = page3[0];
    out.verMaj = (page3[1] >> 4) & 0x0F;
    out.verMin = page3[1] & 0x0F;
    out.size8 = page3[2];
    out.access = page3[3];

    out.ccValid = (out.ccMagic == 0xE1);

    out.userBytes = (uint16_t)out.size8 * 8u;
    out.userPages = out.userBytes / kBytesPerPage;

    switch (out.size8)
    {
    case 0x06:
        out.probableType = "MIFARE Ultralight (48 B user)";
        break;
    case 0x0C:
        out.probableType = "MIFARE Ultralight C (96 B user)";
        break;
    case 0x12:
        out.probableType = "NTAG213 (144 B user)";
        break;
    case 0x3F:
        out.probableType = "NTAG215 (504 B user)";
        break;
    case 0x6F:
        out.probableType = "NTAG216 (888 B user)";
        break;
    default:
        out.probableType = "Type 2 (unknown capacity)";
        break;
    }

    return true;
}

bool Type2TagReader::readUserMemory(uint8_t *buffer, size_t bufferCap, const TagInfo &info) const
{
    if (!buffer || info.userBytes == 0)
        return false;
    if (info.userBytes > bufferCap)
        return false;

    for (uint16_t i = 0; i < info.userPages; ++i)
    {
        if (!nfc_.mifareultralight_ReadPage(kFirstUserPage + i,
                                            buffer + i * kBytesPerPage))
        {
            return false;
        }
    }

    return true;
}

bool Type2TagReader::isSameUid(const uint8_t *lhs, const uint8_t *rhs, uint8_t len)
{
    return lhs && rhs && (memcmp(lhs, rhs, len) == 0);
}
