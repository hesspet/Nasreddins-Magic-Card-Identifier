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
#include "src/CardReader/CardPayloadProcessor.h"
#include "src/CardReader/NdefHelper.h"
#include "src/CardReader/Type2TagReader.h"
#include "src/Logger/Logger.h"

/* Card Reader */

static PN532_HSU pn532hsu(PN532_HSU_PORT);
static PN532 nfc(pn532hsu);
static Type2TagReader tagReader(nfc);

static NdefHelper ndefHelper;
static CardReaderManager cardReaderManager(nfc, tagReader, ndefHelper);
static CardPayloadProcessor cardPayloadProcessor(ndefHelper);

void CardPayloadProcessor::OnPayloadRead(const NdefHelper::NdefRecord &record)
{
    Logger::LogInfo(F("OnPayloadRead - payload length: "), false);
    Logger::LogInfo(static_cast<unsigned long>(record.payloadLen));

    if (!instCardPayloadProcessor_)
    {
        Logger::LogError(F("CardPayloadProcessor instance not initialized."));
        return;
    }

    if (!record.payload || record.payloadLen == 0)
    {
        Logger::LogWarn(F("Received record with empty payload."));
        return;
    }

    if (instCardPayloadProcessor_->ndefHelper_.isTextRecord(record))
    {
        instCardPayloadProcessor_->ndefHelper_.decodeAndPrintTextRecord(record);
    }
    else
    {
        Logger::LogWarn(F("First NDEF record is not a Text (RTD/T) record."));
        Logger::LogDebug(F("Payload (hex):"));

        const String payload = instCardPayloadProcessor_->ndefHelper_.getString(record.payload, record.payloadLen);

        instCardPayloadProcessor_->ProcessPayload(payload);
    }
}

void setup()
{
    Logger::begin(115200, true, Logger::Level::Info);
    cardReaderManager.setPayloadCallback(CardPayloadProcessor::OnPayloadRead);
    cardReaderManager.begin();
}

void loop()
{
    cardReaderManager.process();
}

