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

#include "src/config.h"
#include "src/CardReader/CardReaderManager.h"
#include "src/CardReader/NdefHelper.h"
#include "src/CardReader/Type2TagReader.h"
#include "src/Logger/Logger.h"

/* Card Reader */

static PN532_HSU pn532hsu(PN532_HSU_PORT);
static PN532 nfc(pn532hsu);
static Type2TagReader tagReader(nfc);

static NdefHelper ndefHelper;
static CardReaderManager cardReaderManager(nfc, tagReader, ndefHelper);

static void OnPayloadRead(const NdefHelper::NdefRecord &record);

static void ProcessPayload(const String& payload);

static void ProcessPayloadLine(const String& payloadDump, int& lineStart, bool& firstLine, int& retFlag);

void setup()
{
    Logger::begin(115200, true, Logger::Level::Info);
    cardReaderManager.setPayloadCallback(OnPayloadRead);
    cardReaderManager.begin();
}

void loop()
{
    cardReaderManager.process();
}

static void OnPayloadRead(const NdefHelper::NdefRecord &record)
{
    Logger::LogInfo(F("OnPayloadRead - payload length: "), false);
    Logger::LogInfo(static_cast<unsigned long>(record.payloadLen));

    if (!record.payload || record.payloadLen == 0)
    {
        Logger::LogWarn(F("Received record with empty payload."));
        return;
    }

    if (ndefHelper.isTextRecord(record))
    {
        ndefHelper.decodeAndPrintTextRecord(record);
    }
    else
    {
        Logger::LogWarn(F("First NDEF record is not a Text (RTD/T) record."));
        Logger::LogDebug(F("Payload (hex):"));

        const String payload = ndefHelper.getString(record.payload, record.payloadLen);
        
        ProcessPayload(payload);
    }
}

void ProcessPayload(const String& payload)
{
    int lineStart = 0;
    bool firstLine = true;

    while (lineStart < payload.length())
    {
        int retFlag;
        ProcessPayloadLine(payload, lineStart, firstLine, retFlag);
        if (retFlag == 2) break;
    }
}

void ProcessPayloadLine(const String& payload, int& lineStart, bool& firstLine, int& retFlag)
{
    retFlag = 1;
    int lineEnd = payload.indexOf('\n', lineStart);
    String line = (lineEnd == -1) ? payload.substring(lineStart)
        : payload.substring(lineStart, lineEnd);

    if (line.length() > 0)
    {
        if (firstLine)
        {
            Logger::LogDebug(line);
        }
        else
        {
            Logger::LogInfo(line);
        }
    }
    else
    {
        Logger::LogInfo();
    }

    firstLine = false;

    if (lineEnd == -1)
    {
        { retFlag = 2; return; };
    }
    lineStart = lineEnd + 1;
}
