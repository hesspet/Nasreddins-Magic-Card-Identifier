#include "CardReaderManager.h"

#include <Arduino.h>
#include <string.h>

#include "../Logger/Logger.h"

CardReaderManager::CardReaderManager(PN532 &nfc, Type2TagReader &tagReader, NdefHelper &ndefHelper)
    : nfc_(nfc), tagReader_(tagReader), ndefHelper_(ndefHelper)
{
}

void CardReaderManager::setPayloadCallback(PayloadCallback callback)
{
    payloadCallback_ = callback;
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
            notifyPayload(rec);
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

void CardReaderManager::notifyPayload(const NdefHelper::NdefRecord &record) const
{
    if (payloadCallback_)
    {
        payloadCallback_(record);
    }
}
