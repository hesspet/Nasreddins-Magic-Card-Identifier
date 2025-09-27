/***************************************************************************
 * Project: Esp32NMCI
 * File: CardReaderManager.h
 * Repository: https://github.com/hesspet/NasrredinsMagicCardIdentifier
 * Author: Peter Heß, Büdingen DE
 * Description: Declares the interface of the manager that drives the PN532,
 *              processes NFC tags, and forwards payload data via callbacks.
 ***************************************************************************/

#pragma once

#include <Arduino.h>
#include <PN532.h>
#include <stdint.h>

#include "../config.h"
#include "NdefHelper.h"
#include "Type2TagReader.h"

class CardReaderManager
{
    public:
    using NewDataCallback = void (*)(const String &resolvedPayload);

    CardReaderManager(PN532 &nfc, Type2TagReader &tagReader, NdefHelper &ndefHelper);

    void begin();
    void process();

    void setNewDataCallback(NewDataCallback callback);

    private:
    void handleTagDetected(const uint8_t *uid, uint8_t uidLength);
    void handleTagRemoved();
    void processRecord(const NdefHelper::NdefRecord &record);
    bool resolvePayloadText(const NdefHelper::NdefRecord &record, String &resolvedText) const;
    void notifyNewData(const String &payloadText) const;

    PN532 &nfc_;
    Type2TagReader &tagReader_;
    NdefHelper &ndefHelper_;
    NewDataCallback newDataCallback_ = nullptr;

    bool tagPresent_ = false;
    uint8_t lastUid_[kUidBufferMax] = {0};
    uint8_t lastUidLength_ = 0;
    uint8_t userMemory_[kUserMaxBytes] = {0};
};
