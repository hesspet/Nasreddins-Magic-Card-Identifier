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
static CardPayloadProcessor cardPayloadProcessor;

void setup()
{
    Logger::begin(115200, true, Logger::Level::Info);
    cardReaderManager.setNewDataCallback(CardPayloadProcessor::OnNewData);
    cardReaderManager.begin();
}

void loop()
{
    cardReaderManager.process();
}

