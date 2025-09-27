/***************************************************************************
 * Project: Esp32NMCI
 * File: Esp32NMCI.ino
 * Repository: https://github.com/hesspet/NasrredinsMagicCardIdentifier
 * Author: Peter Heß, Büdingen DE
 * Description: Main sketch that initializes the display, buttons, BLE keyboard,
 *              and NFC reader while orchestrating the complete card processing
 *              workflow.
 ***************************************************************************/

 /**************************************************************************/
 /*!

   Esp32NMCI.ino

   ESP32 T-Display + PN532 (HSU/UART)
   Detection of Type-2 tags via capability container (page 3),
   dynamic reading of the complete user memory (from page 4 onwards),
   TLV parsing, NDEF parsing (first record) with optional text output.

   Wiring (HSU/UART):
								 ESP32 TX (GPIO 25) → PN532 RX
								 ESP32 RX (GPIO 26) → PN532 TX
								 3.3 V logic level, PN532 configured for HSU/UART

   Serial output: 115200 baud
 */
 /**************************************************************************/

 // BLE handling – keyboard emulation
#include <NimBLEDevice.h> // 2.3.6
#include <BleKeyboard.h> // Manually patched for NimBLE 2.3.6!

// Required for the T-Display
#include <SPI.h>
#include <TFT_eSPI.h>

// Card reader
#include <PN532.h>
#include <PN532_HSU.h>

// Miscellaneous helpers :-)
#include <vector>

// Implementation
#include "src/config.h"
#include "src/CardReader/CardReaderManager.h"
#include "src/CardReader/CardPayloadProcessor.h"
#include "src/CardReader/NdefHelper.h"
#include "src/CardReader/Type2TagReader.h"
#include "src/Logger/Logger.h"
#include "src/Presentation/DisplayManager.h"
#include "src/Presentation/ButtonManager.h"
#include "src/Helper/Utf8Decoder.h"
#include "src/Helper/SystemHelper.h"
#include "src/Ble/BleKeyboardCallback.h"
#include "src/Ble/BleHelper.h"

// Card reader

/**
 * @brief High-speed UART interface to the PN532 module.
 */
static PN532_HSU _pn532hsu(PN532_HSU_PORT);

/**
 * @brief PN532 instance used to communicate with the NFC reader.
 */
static PN532 _nfc(_pn532hsu);

/**
 * @brief Reader for MIFARE Type 2 tags using the PN532.
 */
static Type2TagReader _tagReader(_nfc);

/**
 * @brief Helper class for interpreting NDEF data.
 */
static NdefHelper _ndefHelper;

/**
 * @brief Coordinates the card reader logic, including callback registration.
 */
static CardReaderManager _cardReaderManager(_nfc, _tagReader, _ndefHelper);

/**
 * @brief Processes card data into displayable or transferable lists.
 */
static CardPayloadProcessor _cardPayloadProcessor;

/**
 * @brief Temporary storage for the payload extracted from a card.
 */
static std::vector<String> _listElementsInPayloadFromCard;

// Display

/**
 * @brief Manager responsible for rendering content on the integrated T-Display.
 */
static DisplayManager _displayManager;

/**
 * @brief GPIO of the first button (boot button).
 */
constexpr int kButtonPin0 = 0;

/**
 * @brief GPIO of the second button, also used as the wake-up source.
 */
constexpr int kButtonPin35 = 35;

/**
 * @brief Manages the physical buttons, including debouncing.
 */
static ButtonManager _buttonManager(kButtonPin0, kButtonPin35);

/**
 * @brief Global instance of the BLE keyboard emulation.
 */
static BleKeyboardCallback _bleKeyboardCallbackHandler;

/**
 * @brief Callback invoked when new card data is available to display and
 *        forward via BLE.
 *
 * @param payloadText Raw text read from the card.
 */
static void OnNewData(const String& payloadText)
{
	_listElementsInPayloadFromCard = _cardPayloadProcessor.ProcessPayload(payloadText);

	if (_listElementsInPayloadFromCard.empty())
	{
		Logger::logWarn(F("Received empty card payload."));
		return;
	}

	Logger::logInfo(F("Processed card payload values:"));

	for (size_t index = 0; index < _listElementsInPayloadFromCard.size(); ++index)
	{
		if (index == 0)
		{
			// Display the primary entry immediately
			_displayManager.showMessage(_listElementsInPayloadFromCard[index]);

		}
		Logger::logf(Logger::Level::Info, "  [%u] %s\n", static_cast<unsigned>(index), _listElementsInPayloadFromCard[index].c_str());
	}

	_displayManager.showMessage(_listElementsInPayloadFromCard);

	BleHelper::sendTextToKeyboard(_bleKeyboardCallbackHandler, _listElementsInPayloadFromCard.front());

}

/**
 * @brief Arduino initialization: sets up the logger, display, card reader, and BLE.
 */
void setup()
{
	Logger::begin(115200, true, Logger::Level::Info);

	_displayManager.begin(DisplayManager::Orientation::UsbLeft);
	Logger::setDisplayManager(&_displayManager);
	
	_bleKeyboardCallbackHandler.setDisplayManager(&_displayManager);

	_cardReaderManager.setNewDataCallback(OnNewData);
	_cardReaderManager.begin();
	_buttonManager.begin();
	_buttonManager.setOnButton35LongPressed(SystemHelper::EnterDeepSleep);

	Serial.println();
	Serial.println(F("[BOOT] ESP32 BLE-HID Keyboard (DE) - Boot-Protocol-First"));
	_bleKeyboardCallbackHandler.begin();
}

/**
 * @brief Main loop handling serial input and user interactions.
 */
void loop()
{

#ifdef SERIAL_INPUT_ALLOWED
	while (Serial.available())
	{
		const uint8_t b = static_cast<uint8_t>(Serial.read());

		if (!gbleKeyboard.isConnected())
		{
			if (b <= 0x7F)
			{
				Serial.printf("[WARN] Not connected as keyboard: '%c' (0x%02X)\n", static_cast<char>(b), static_cast<unsigned>(b));
			}
			else
			{
				Serial.printf("[WARN] Not connected as keyboard: U+%04lX\n", static_cast<unsigned long>(b));
			}
			continue;
		}
		gbleKeyboard.print(b);
	}
#endif

	_buttonManager.update();
	_cardReaderManager.process();
	_displayManager.update();
}

