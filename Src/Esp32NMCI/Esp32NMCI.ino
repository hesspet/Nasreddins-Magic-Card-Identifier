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

#include <NimBLEDevice.h>
#include <BleKeyboard.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <PN532.h>
#include <PN532_HSU.h>
#include <vector>
#include <esp_sleep.h>

#include "src/config.h"
#include "src/CardReader/CardReaderManager.h"
#include "src/CardReader/CardPayloadProcessor.h"
#include "src/CardReader/NdefHelper.h"
#include "src/CardReader/Type2TagReader.h"
#include "src/Logger/Logger.h"
#include "src/Presentation/DisplayManager.h"
#include "src/Presentation/ButtonManager.h"
#include "src/Helper/Utf8Decoder.h"


/* Card Reader */

static PN532_HSU pn532hsu(PN532_HSU_PORT);
static PN532 nfc(pn532hsu);
static Type2TagReader tagReader(nfc);

static NdefHelper ndefHelper;
static CardReaderManager cardReaderManager(nfc, tagReader, ndefHelper);
static CardPayloadProcessor cardPayloadProcessor;
static std::vector<String> ListElementsInPayloadFromCard;
static DisplayManager displayManager;
constexpr int kButtonPin0 = 0;
constexpr int kButtonPin35 = 35;
static ButtonManager buttonManager(kButtonPin0, kButtonPin35);
BleKeyboard gbleKeyboard;

static void SendTextToKeyboard(const String& text)
{
	if (text.length() == 0)
	{
		return;
	}

	if (!gbleKeyboard.isConnected())
	{
		Logger::LogWarn(F("BLE keyboard not connected; skipping payload transfer."));
		return;
	}

	gbleKeyboard.print(text);
}

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

	displayManager.showMessage(ListElementsInPayloadFromCard);

	SendTextToKeyboard(ListElementsInPayloadFromCard.front());

}

static void EnterDeepSleep()
{
	Logger::LogInfo(F("[System] Entering deep sleep"));
	esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 0);
	esp_deep_sleep_start();
}

void setup()
{
	Logger::begin(115200, true, Logger::Level::Info);

	displayManager.begin(DisplayManager::Orientation::UsbLeft);
	Logger::setDisplayManager(&displayManager);

	cardReaderManager.setNewDataCallback(OnNewData);
	cardReaderManager.begin();
	buttonManager.begin();
	buttonManager.setOnButton35LongPressed(EnterDeepSleep);

	Serial.println();
	Serial.println(F("[BOOT] ESP32 BLE-HID Keyboard (DE) – Boot-Protocol-First"));
	gbleKeyboard.begin();
}

void loop()
{
	while (Serial.available())
	{
		const uint8_t b = static_cast<uint8_t>(Serial.read());

		if (!gbleKeyboard.isConnected())
		{
			if (b <= 0x7F)
			{
				Serial.printf("[WARN] Nicht verbunden: '%c' (0x%02X)\n", static_cast<char>(b), static_cast<unsigned>(b));
			}
			else
			{
				Serial.printf("[WARN] Nicht verbunden: U+%04lX\n", static_cast<unsigned long>(b));
			}
			continue;
		}
		gbleKeyboard.print(b);
	}

	buttonManager.update();
	cardReaderManager.process();
	displayManager.update();
}

