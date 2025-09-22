#pragma once

#include <NimBLEDevice.h>
#include <stdint.h>
#include <memory>

class HidKeyboardProtocolModeCallbacks;
class HidKeyboardInputSubscribeCallbacks;
class HidKeyboardServerCallbacks;

/**
 * @brief Verwaltet alle BLE-HID-Funktionen für das virtuelle Keyboard.
 */
class HidKeyboard {
public:
	/**
	 * @brief Konstruktor initialisiert alle Pointer und Statusvariablen.
	 */
	HidKeyboard();

	/**
	 * @brief Destruktor sorgt für die Freigabe der Callback-Objekte.
	 */
	~HidKeyboard();

	/**
	 * @brief Initialisiert NimBLE, richtet den HID-Service ein und startet Advertising.
	 */
	void begin();

	/**
	 * @brief Gibt zurück, ob aktuell ein zentraler BLE-Client verbunden ist.
	 */
	bool isConnected() const;

	/**
	 * @brief Konvertiert einen Unicode-Codepoint und sendet ihn als Tastendruck.
	 *
	 * Für nicht unterstützte Zeichen werden Diagnosemeldungen im Seriellen Monitor ausgegeben.
	 */
	void typeCodepoint(uint32_t cp);

private:
	friend class HidKeyboardProtocolModeCallbacks;
	friend class HidKeyboardInputSubscribeCallbacks;
	friend class HidKeyboardServerCallbacks;

	/**
	 * @brief Verarbeitet Schreibzugriffe auf die Protocol-Mode-Charakteristik.
	 */
	void handleProtocolModeWrite(NimBLECharacteristic* characteristic);

	/**
	 * @brief Aktualisiert den Subskriptionsstatus für Boot- und Report-Input.
	 */
	void handleSubscription(NimBLECharacteristic* characteristic, uint16_t subValue);

	/**
	 * @brief Bereitet das Gerät nach erfolgreichem Verbindungsaufbau vor.
	 */
	void afterConnect();

	/**
	 * @brief Setzt den Status nach einem Verbindungsabbruch zurück und startet Advertising.
	 */
	void afterDisconnect();

	/**
	 * @brief Sendet einen Rohreport an die angegebene Charakteristik.
	 */
	void sendKeyReportRaw(
		NimBLECharacteristic* characteristic,
		const char* tag,
		uint8_t mods,
		uint8_t k1 = 0,
		uint8_t k2 = 0,
		uint8_t k3 = 0,
		uint8_t k4 = 0,
		uint8_t k5 = 0,
		uint8_t k6 = 0);

	/**
	 * @brief Sendet einen Report unter Berücksichtigung des aktiven Protokolls.
	 */
	void sendKeyReport(
		uint8_t mods,
		uint8_t k1 = 0,
		uint8_t k2 = 0,
		uint8_t k3 = 0,
		uint8_t k4 = 0,
		uint8_t k5 = 0,
		uint8_t k6 = 0);

	/**
	 * @brief Sendet einen Tastendruck mit anschließendem Release.
	 */
	void pressAndRelease(uint8_t mods, uint8_t key);

	/**
	 * @brief Wandelt einen Codepoint ins deutsche HID-Layout um.
	 */
	bool cpToHid_DE(uint32_t cp, uint8_t& mods, uint8_t& key);

	NimBLECharacteristic* mInputReport;
	NimBLECharacteristic* mBootIn;
	NimBLEAdvertising* mAdv;
	volatile bool mConnected;
	uint8_t mProtocolMode;
	volatile bool mSubBootIn;
	volatile bool mSubReport;
	std::unique_ptr<HidKeyboardProtocolModeCallbacks> mProtoCb;
	std::unique_ptr<HidKeyboardInputSubscribeCallbacks> mSubCb;
	std::unique_ptr<HidKeyboardServerCallbacks> mServerCb;
};

