/**************************************************************************/
/*!
  ESP32 T-Display + PN532 (HSU/UART)
  Erkennung von Type-2-Tag via Capability Container (Page 3),
  dynamisches Lesen des gesamten User-Memory (ab Page 4),
  TLV-Parsing, NDEF-Parsing (erster Record) optional Textausgabe.

  Verkabelung (HSU/UART):
                ESP32 TX (GPIO 25) → PN532 RX
                ESP32 RX (GPIO 26) → PN532 TX
                3,3V Pegel, PN532 auf HSU/UART gestellt

  Serielle Ausgabe: 115200 Baud
*/
/**************************************************************************/

#include <PN532.h>
#include <PN532_HSU.h>
#include <string.h>

#include "config.h"
#include "src/CardReader/NdefHelper.h"
#include "src/CardReader/Type2TagReader.h"
#include "src/Logger/Logger.h"

/* Card Reader */

static PN532_HSU pn532hsu(PN532_HSU_PORT);
static PN532 nfc(pn532hsu);
static Type2TagReader tagReader(nfc);

static NdefHelper ndefHelper;

void setup()
{
    Logger::begin(115200, true, Logger::Level::Info);

    // Setup Card Reader

    PN532_HSU_PORT.begin(PN532_HSU_BAUDRATE, SERIAL_8N1, PN532_HSU_RX_PIN, PN532_HSU_TX_PIN);
    nfc.begin();

    uint32_t versiondata = nfc.getFirmwareVersion();
    if (!versiondata)
    {
        Logger::LogInfo(F("PN532 not found (check wiring & HSU mode)"));
        while (true)
        {
            delay(1000); // Endless Loop Stop TODO: Write ERROR TO THE T-DISPLAY
        }
    }

    Logger::LogInfo(F("Found chip PN5"), false);
    Logger::LogInfo((versiondata >> 24) & 0xFF, true, HEX);
    Logger::LogInfo(F("Firmware ver. "), false);
    Logger::LogInfo((versiondata >> 16) & 0xFF, false, DEC);
    Logger::LogInfo('.', false);
    Logger::LogInfo((versiondata >> 8) & 0xFF, true, DEC);

    nfc.SAMConfig();
    nfc.setPassiveActivationRetries(0xFF);

    Logger::LogInfo("Initialized");
    Logger::LogDebug(F("Waiting for an ISO14443A Card ..."));
}

void loop()
{
    static uint8_t lastUid[kUidBufferMax] = {0};
    static uint8_t lastUidLength = 0;
    static bool tagPresent = false;

    uint8_t uid[kUidBufferMax] = {0};
    uint8_t uidLength = 0;

    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength))
    {
        bool newTagDetected = (!tagPresent) ||
                              (uidLength != lastUidLength) ||
                              (!Type2TagReader::isSameUid(uid, lastUid, uidLength));

        if (newTagDetected)
        {
            tagPresent = true;
            lastUidLength = uidLength;
            memcpy(lastUid, uid, uidLength);

            Logger::LogDebug(F("Tag detected. UID length: "), false);
            Logger::LogDebug(uidLength);
            Logger::LogDebug(F("UID: "), false);
            nfc.PrintHex(uid, uidLength);

            // --- 1) CC lesen & interpretieren ---
            Type2TagReader::TagInfo ti{};
            if (!tagReader.readCapabilityContainer(ti))
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

            // --- 2) User Memory vollständig lesen (dynamisch) ---
            static uint8_t user[kUserMaxBytes];
            if (!tagReader.readUserMemory(user, sizeof(user), ti))
            {
                Logger::LogError(F("Failed to read user memory"));
                return;
            }

            // Optional: Rohdump
            // ndefHelper.dumpHexAscii(user, ti.userBytes);

            // --- 3) TLV scannen: NDEF (0x03) finden ---
            NdefHelper::Tlv tlv{};
            bool foundNdef = ndefHelper.findFirstNdefTlv(user, ti.userBytes, tlv);

            if (foundNdef)
            {
                Logger::LogDebug(F("NDEF length (TLV): "), false);
                Logger::LogDebug(static_cast<unsigned>(tlv.length));

                // --- 4) Ersten NDEF-Record parsen & bei Text ausgeben ---
                NdefHelper::NdefRecord rec{};
                if (ndefHelper.parseFirstRecord(tlv.value, tlv.length, rec))
                {
                    // Wenn es ein Text-Record ist, gib den Text direkt aus (wie bei dir genutzt)
                    if (ndefHelper.isTextRecord(rec))
                    {
                        ndefHelper.decodeAndPrintTextRecord(rec);
                    }
                    else
                    {
                        Logger::LogWarn(F("First NDEF record is not a Text (RTD/T) record."));
                        Logger::LogWarn(F("Record header / payload (hex):"));
                        ndefHelper.dumpHexAscii(tlv.value, tlv.length);
                    }
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
    }
    else if (tagPresent)
    {
        tagPresent = false;
        lastUidLength = 0;
        memset(lastUid, 0, sizeof(lastUid));
        Logger::LogDebug(F("Tag removed"));
    }

    delay(250);
}
