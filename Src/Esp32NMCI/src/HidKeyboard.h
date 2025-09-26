#pragma once

#include <NimBLEDevice.h>
#include <stdint.h>
#include <memory>

class HidKeyboardProtocolModeCallbacks;
class HidKeyboardInputSubscribeCallbacks;
class HidKeyboardServerCallbacks;

/**
 * @brief Verwaltet den kompletten BLE-HID-Stack für das virtuelle Keyboard.
 *
 * Die Klasse kapselt sowohl das Anlegen der notwendigen Services und
 * Charakteristiken als auch das Erzeugen der Tastatur-Reports. Sie dient somit
 * als zentrale Abstraktion, um Unicode-Codepoints in HID-Events zu übersetzen
 * und zuverlässig an einen verbundenen BLE-Host zu übertragen.
 */
class HidKeyboard {
public:
        /**
         * @brief Konstruktor initialisiert alle Pointer und Statusvariablen.
         *
         * Die Callback-Objekte werden hier erstellt, so dass sie während der
         * gesamten Lebensdauer der Klasse verfügbar sind.
         */
        HidKeyboard();

        /**
         * @brief Destruktor sorgt für die Freigabe der Callback-Objekte.
         *
         * Durch die Verwendung von smarten Zeigern ist hier keine zusätzliche
         * Logik notwendig, dennoch dokumentiert die Methode die Intention.
         */
        ~HidKeyboard();

        /**
         * @brief Initialisiert NimBLE, richtet den HID-Service ein und startet Advertising.
         *
         * Neben dem HID-Service werden auch die obligatorischen
         * Device-Information-Charakteristiken sowie die Werbeparameter
         * konfiguriert. Die Methode muss einmalig in setup() aufgerufen werden.
         */
        void begin();

        /**
         * @brief Gibt zurück, ob aktuell ein zentraler BLE-Client verbunden ist.
         *
         * @return true, wenn ein Host gekoppelt ist; andernfalls false.
         */
        bool isConnected() const;

        /**
         * @brief Konvertiert einen Unicode-Codepoint und sendet ihn als Tastendruck.
         *
         * Der Codepoint wird gegen das deutsche Tastaturlayout abgebildet und
         * anschließend als Press/Release-Sequenz gesendet. Für nicht
         * unterstützte Zeichen werden Diagnosemeldungen im seriellen Monitor
         * ausgegeben.
         *
         * @param cp Unicode-Codepoint (UTF-32) des zu sendenden Zeichens.
         */
        void typeCodepoint(uint32_t cp);

private:
	friend class HidKeyboardProtocolModeCallbacks;
	friend class HidKeyboardInputSubscribeCallbacks;
	friend class HidKeyboardServerCallbacks;

        /**
         * @brief Verarbeitet Schreibzugriffe auf die Protocol-Mode-Charakteristik.
         *
         * Aktualisiert den internen Status, sobald der Host zwischen Boot- und
         * Report-Modus wechselt.
         *
         * @param characteristic Charakteristik, die den Schreibzugriff
         *        ausgelöst hat.
         */
        void handleProtocolModeWrite(NimBLECharacteristic* characteristic);

        /**
         * @brief Aktualisiert den Subskriptionsstatus für Boot- und Report-Input.
         *
         * BLE-Hosts können unabhängig voneinander die Benachrichtigungen der
         * Boot- und der Report-Charakteristik abonnieren. Diese Methode merkt
         * sich, welche Variante aktiv ist.
         *
         * @param characteristic Charakteristik, deren Subskription sich
         *        geändert hat.
         * @param subValue Neuer CCCD-Wert aus dem onSubscribe-Callback.
         */
        void handleSubscription(NimBLECharacteristic* characteristic, uint16_t subValue);

        /**
         * @brief Bereitet das Gerät nach erfolgreichem Verbindungsaufbau vor.
         *
         * Hier werden Statusvariablen gesetzt und zur Verbindung ein kurzer
         * Funktionstest (Shift+A, Enter) an den Host gesendet.
         */
        void afterConnect();

        /**
         * @brief Setzt den Status nach einem Verbindungsabbruch zurück und startet Advertising.
         */
        void afterDisconnect();

        /**
         * @brief Sendet einen Rohreport an die angegebene Charakteristik.
         *
         * Die Methode bereitet abhängig vom Ziel (Boot- oder Report-Charakteristik)
         * das passende Datenlayout auf, ergänzt ggf. die Report-ID und stößt
         * anschließend die BLE-Benachrichtigung an.
         *
         * @param characteristic Zielcharakteristik (Boot- oder Report-Input).
         * @param tag Menschlich lesbares Tag für das Debug-Log.
         * @param mods Aktive Modifier-Bits.
         * @param k1 Erster Keycode.
         * @param k2 Zweiter Keycode.
         * @param k3 Dritter Keycode.
         * @param k4 Vierter Keycode.
         * @param k5 Fünfter Keycode.
         * @param k6 Sechster Keycode.
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
         *
         * Abhängig davon, ob der Host Boot- oder Report-Mode aktiviert hat, wird
         * die passende Charakteristik verwendet. Falls keine aktive
         * Subskription vorliegt, werden beide Kanäle bedient, um die
         * Kompatibilität zu maximieren.
         *
         * @param mods Aktive Modifier-Bits.
         * @param k1 Erster Keycode.
         * @param k2 Zweiter Keycode.
         * @param k3 Dritter Keycode.
         * @param k4 Vierter Keycode.
         * @param k5 Fünfter Keycode.
         * @param k6 Sechster Keycode.
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
         *
         * @param mods Modifier-Bits des Tastendrucks.
         * @param key Keycode des Zeichens.
         */
        void pressAndRelease(uint8_t mods, uint8_t key);

        /**
         * @brief Wandelt einen Codepoint ins deutsche HID-Layout um.
         *
         * @param cp Eingangs-Codepoint.
         * @param mods Referenz auf die zu setzenden Modifier-Bits.
         * @param key Referenz auf den resultierenden HID-Keycode.
         * @return true, wenn eine Zuordnung existiert; andernfalls false.
         */
        bool cpToHid_DE(uint32_t cp, uint8_t& mods, uint8_t& key);

        /** Zeiger auf die Report-Mode-Input-Charakteristik. */
        NimBLECharacteristic* mInputReport;
        /** Zeiger auf die Boot-Mode-Input-Charakteristik. */
        NimBLECharacteristic* mBootIn;
        /** Handle für das Advertising-Objekt von NimBLE. */
        NimBLEAdvertising* mAdv;
        /** Verbindungstatus, wird in den Server-Callbacks aktualisiert. */
        volatile bool mConnected;
        /** Aktueller Protokollmodus (0 = Boot, 1 = Report). */
        uint8_t mProtocolMode;
        /** Merker, ob der Host Boot-Reports abonniert hat. */
        volatile bool mSubBootIn;
        /** Merker, ob der Host Report-Mode abonniert hat. */
        volatile bool mSubReport;
        /** Callback für Schreibzugriffe auf den Protocol-Mode. */
        std::unique_ptr<HidKeyboardProtocolModeCallbacks> mProtoCb;
        /** Callback für Subskriptionen der Input-Reports. */
        std::unique_ptr<HidKeyboardInputSubscribeCallbacks> mSubCb;
        /** Callback für Connect/Disconnect-Ereignisse. */
        std::unique_ptr<HidKeyboardServerCallbacks> mServerCb;
};

