#pragma once

#include <PN532.h>
#include <stdint.h>

#include "../../../../config.h"
#include "NdefHelper.h"
#include "Type2TagReader.h"

class CardReaderManager
{
    public:
    using PayloadCallback = void (*)(const NdefHelper::NdefRecord &record);

    CardReaderManager(PN532 &nfc, Type2TagReader &tagReader, NdefHelper &ndefHelper);

    void begin();
    void process();

    void setPayloadCallback(PayloadCallback callback);

    private:
    void handleTagDetected(const uint8_t *uid, uint8_t uidLength);
    void handleTagRemoved();
    void notifyPayload(const NdefHelper::NdefRecord &record) const;

    PN532 &nfc_;
    Type2TagReader &tagReader_;
    NdefHelper &ndefHelper_;
    PayloadCallback payloadCallback_ = nullptr;

    bool tagPresent_ = false;
    uint8_t lastUid_[kUidBufferMax] = {0};
    uint8_t lastUidLength_ = 0;
    uint8_t userMemory_[kUserMaxBytes] = {0};
};
