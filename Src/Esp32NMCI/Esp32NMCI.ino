/**************************************************************************/
/*!

  Esp32NMCI.ino

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

#include <SPI.h>
#include <TFT_eSPI.h>
#include <PN532.h>
#include <PN532_HSU.h>
#include <vector>

#include "src/config.h"
#include "src/CardReader/CardReaderManager.h"
#include "src/CardReader/CardPayloadProcessor.h"
#include "src/CardReader/NdefHelper.h"
#include "src/CardReader/Type2TagReader.h"
#include "src/Logger/Logger.h"
#include "src/Presentation/DisplayManager.h"


/* Card Reader */

static PN532_HSU pn532hsu(PN532_HSU_PORT);
static PN532 nfc(pn532hsu);
static Type2TagReader tagReader(nfc);

static NdefHelper ndefHelper;
static CardReaderManager cardReaderManager(nfc, tagReader, ndefHelper);
static CardPayloadProcessor cardPayloadProcessor;
static std::vector<String> ListElementsInPayloadFromCard;
static DisplayManager displayManager;

static void OnNewData(const String& payloadText)
{
	ListElementsInPayloadFromCard = cardPayloadProcessor.ProcessPayload(payloadText);

	if (ListElementsInPayloadFromCard.empty())
	{
		Logger::LogWarn(F("Received empty card payload."));
		return;
	}

	Logger::LogInfo(F("Processed card payload values:"));

	for (size_t index = 0; index < ListElementsInPayloadFromCard.size(); ++index)
	{
		if (index == 0)
		{
			// Ausgabe des Kürzels
			displayManager.showMessage(ListElementsInPayloadFromCard[index]);

		}
		Logger::logf(Logger::Level::Info, "  [%u] %s\n", static_cast<unsigned>(index), ListElementsInPayloadFromCard[index].c_str());
	}

	// add here showMessage with vector

}

void setup()
{
	Logger::begin(115200, true, Logger::Level::Info);

	displayManager.begin(DisplayManager::Orientation::UsbLeft);

	cardReaderManager.setNewDataCallback(OnNewData);
	cardReaderManager.begin();
}

void loop()
{
	cardReaderManager.process();
}

