#include "CardReaderManager.h"

#include <Arduino.h>
#include <string.h>

#include "../Logger/Logger.h"

CardReaderManager::CardReaderManager(PN532 &nfc, Type2TagReader &tagReader, NdefHelper &ndefHelper)
    : nfc_(nfc), tagReader_(tagReader), ndefHelper_(ndefHelper)
{
}

void CardReaderManager::setNewDataCallback(NewDataCallback callback)
{
    newDataCallback_ = callback;
}

void CardReaderManager::begin()
{
    PN532_HSU_PORT.begin(PN532_HSU_BAUDRATE, SERIAL_8N1, PN532_HSU_RX_PIN, PN532_HSU_TX_PIN);
    nfc_.begin();

    uint32_t versiondata = nfc_.getFirmwareVersion();
    if (!versiondata)
    {
        Logger::LogInfo(F("PN532 not found (check wiring & HSU mode)"));
        while (true)
        {
            delay(1000);
        }
    }

    Logger::LogInfo(F("Found chip PN5"), false);
    Logger::LogInfo((versiondata >> 24) & 0xFF, true, HEX);
    Logger::LogInfo(F("Firmware ver. "), false);
    Logger::LogInfo((versiondata >> 16) & 0xFF, false, DEC);
    Logger::LogInfo('.', false);
    Logger::LogInfo((versiondata >> 8) & 0xFF, true, DEC);

    nfc_.SAMConfig();
    nfc_.setPassiveActivationRetries(0xFF);

    Logger::LogInfo(F("Initialized"));
    Logger::LogDebug(F("Waiting for an ISO14443A Card ..."));
}

void CardReaderManager::process()
{
    uint8_t uid[kUidBufferMax] = {0};
    uint8_t uidLength = 0;

    if (nfc_.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength))
    {
        bool newTagDetected = (!tagPresent_) ||
                              (uidLength != lastUidLength_) ||
                              (!Type2TagReader::isSameUid(uid, lastUid_, uidLength));

        if (newTagDetected)
        {
            tagPresent_ = true;
            lastUidLength_ = uidLength;
            memcpy(lastUid_, uid, uidLength);
            handleTagDetected(uid, uidLength);
        }
    }
    else if (tagPresent_)
    {
        handleTagRemoved();
    }

    delay(250);
}

void CardReaderManager::handleTagDetected(const uint8_t *uid, uint8_t uidLength)
{
    Logger::LogDebug(F("Tag detected. UID length: "), false);
    Logger::LogDebug(uidLength);
    Logger::LogDebug(F("UID: "), false);
    nfc_.PrintHex(uid, uidLength);

    Type2TagReader::TagInfo ti{};
    if (!tagReader_.readCapabilityContainer(ti))
    {
        Logger::LogWarn(F("Failed to read Capability Container (page 3)"));
        return;
    }

    Logger::LogDebug(F("CC: Magic=0x"), false);
    Logger::LogDebug(ti.ccMagic, false, HEX);
    Logger::LogDebug(F(", Ver="), false);
    Logger::LogDebug(ti.verMaj, false);
    Logger::LogDebug('.', false);
    Logger::LogDebug(ti.verMin, false);
    Logger::LogDebug(F(", Size8=0x"), false);
    Logger::LogDebug(ti.size8, false, HEX);
    Logger::LogDebug(F(" ("), false);
    Logger::LogDebug(ti.userBytes, false);
    Logger::LogDebug(F(" bytes user)"), false);
    Logger::LogDebug(F(", Access=0x"), false);
    Logger::LogDebug(ti.access, false, HEX);

    if (!ti.ccValid)
    {
        Logger::LogWarn(F("Warning: CC Magic != 0xE1 (evtl. kein NDEF-Tag oder CC korrupt)"));
    }

    Logger::LogDebug(F("Probable type: "));
    Logger::LogDebug(ti.probableType);

    if (!tagReader_.readUserMemory(userMemory_, sizeof(userMemory_), ti))
    {
        Logger::LogError(F("Failed to read user memory"));
        return;
    }

    NdefHelper::Tlv tlv{};
    bool foundNdef = ndefHelper_.findFirstNdefTlv(userMemory_, ti.userBytes, tlv);

    if (foundNdef)
    {
        Logger::LogDebug(F("NDEF length (TLV): "), false);
        Logger::LogDebug(static_cast<unsigned>(tlv.length));

        NdefHelper::NdefRecord rec{};
        if (ndefHelper_.parseFirstRecord(tlv.value, tlv.length, rec))
        {
            processRecord(rec);
        }
        else
        {
            Logger::LogWarn(F("Failed to parse first NDEF record"));
        }
    }
    else
    {
        Logger::LogWarn(F("No NDEF TLV found (0x03)"));
    }
}

void CardReaderManager::handleTagRemoved()
{
    tagPresent_ = false;
    lastUidLength_ = 0;
    memset(lastUid_, 0, sizeof(lastUid_));
    Logger::LogDebug(F("Tag removed"));
}

void CardReaderManager::processRecord(const NdefHelper::NdefRecord &record)
{
    Logger::LogInfo(F("OnNewData - payload length: "), false);
    Logger::LogInfo(static_cast<unsigned long>(record.payloadLen));

    String resolvedText;
    if (!resolvePayloadText(record, resolvedText))
    {
        return;
    }

    notifyNewData(resolvedText);
}

bool CardReaderManager::resolvePayloadText(const NdefHelper::NdefRecord &record, String &resolvedText) const
{
    if (!record.payload || record.payloadLen == 0)
    {
        Logger::LogWarn(F("Received record with empty payload."));
        return false;
    }

    if (ndefHelper_.isTextRecord(record))
    {
        if (record.payloadLen < 1)
        {
            Logger::LogInfo(F("Empty RTD/T payload"));
            return false;
        }

        uint8_t status = record.payload[0];
        bool utf16 = (status & 0x80) != 0;
        uint8_t langLen = (status & 0x3F);

        if (record.payloadLen < static_cast<size_t>(1 + langLen))
        {
            Logger::LogInfo(F("RTD/T payload too short"));
            return false;
        }

        String lang;
        for (uint8_t i = 0; i < langLen; ++i)
        {
            lang += static_cast<char>(record.payload[1 + i]);
        }

        const uint8_t *textPtr = record.payload + 1 + langLen;
        size_t textLen = record.payloadLen - 1 - langLen;

        resolvedText.reserve(lang.length() + textLen + 32);
        resolvedText = F("NDEF Text: (");
        resolvedText += utf16 ? F("UTF-16") : F("UTF-8");
        resolvedText += F(", ");
        resolvedText += lang;
        resolvedText += F(")\n");
        resolvedText += F("Text payload:\n");

        if (utf16)
        {
            resolvedText += ndefHelper_.getString(textPtr, textLen);
        }
        else
        {
            for (size_t i = 0; i < textLen; ++i)
            {
                resolvedText += static_cast<char>(textPtr[i]);
            }
        }

        return true;
    }

    Logger::LogWarn(F("First NDEF record is not a Text (RTD/T) record."));
    Logger::LogDebug(F("Payload (hex):"));
    resolvedText = ndefHelper_.getString(record.payload, record.payloadLen);
    return true;
}

void CardReaderManager::notifyNewData(const String &payloadText) const
{
    if (newDataCallback_)
    {
        newDataCallback_(payloadText);
    }
}
