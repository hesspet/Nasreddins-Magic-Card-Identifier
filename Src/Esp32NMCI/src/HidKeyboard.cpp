#include "HidKeyboard.h"

#include <Arduino.h>

#include "HidConsts.h"

#if !defined(NIMBLE_CPP_CONNINFO_H_)
#    if defined(__has_include)
#        if __has_include(<NimBLEConnInfo.h>)
#            include <NimBLEConnInfo.h>
#        endif
#    endif
#endif

#if defined(NIMBLE_CPP_CONNINFO_H_)
#    define HID_KEYBOARD_HAS_CONN_INFO 1
#else
#    define HID_KEYBOARD_HAS_CONN_INFO 0
#endif

class HidKeyboardProtocolModeCallbacks : public NimBLECharacteristicCallbacks {
public:
    explicit HidKeyboardProtocolModeCallbacks(HidKeyboard& keyboard) : mKeyboard(keyboard) {}

#if HID_KEYBOARD_HAS_CONN_INFO
    void onWrite(NimBLECharacteristic* characteristic, NimBLEConnInfo&) override {
        mKeyboard.handleProtocolModeWrite(characteristic);
    }
#else
    void onWrite(NimBLECharacteristic* characteristic) override {
        mKeyboard.handleProtocolModeWrite(characteristic);
    }
#endif

private:
    HidKeyboard& mKeyboard;
};

class HidKeyboardInputSubscribeCallbacks : public NimBLECharacteristicCallbacks {
public:
    explicit HidKeyboardInputSubscribeCallbacks(HidKeyboard& keyboard) : mKeyboard(keyboard) {}

#if HID_KEYBOARD_HAS_CONN_INFO
    void onSubscribe(
        NimBLECharacteristic* characteristic,
        NimBLEConnInfo&,
        uint16_t subValue) override {
        mKeyboard.handleSubscription(characteristic, subValue);
    }
#else
    void onSubscribe(
        NimBLECharacteristic* characteristic,
        ble_gap_conn_desc*,
        uint16_t subValue) override {
        mKeyboard.handleSubscription(characteristic, subValue);
    }
#endif

private:
    HidKeyboard& mKeyboard;
};

class HidKeyboardServerCallbacks : public NimBLEServerCallbacks {
public:
    explicit HidKeyboardServerCallbacks(HidKeyboard& keyboard) : mKeyboard(keyboard) {}

#if HID_KEYBOARD_HAS_CONN_INFO
    void onConnect(NimBLEServer*, NimBLEConnInfo&) override {
        mKeyboard.afterConnect();
    }

    void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int) override {
        mKeyboard.afterDisconnect();
    }
#else
    void onConnect(NimBLEServer*) override {
        mKeyboard.afterConnect();
    }

    void onDisconnect(NimBLEServer*) override {
        mKeyboard.afterDisconnect();
    }
#endif

private:
    HidKeyboard& mKeyboard;
};

///
/// Konstruktor: initialisiert sämtliche Statuszeiger und Callback-Wrapper.
///
HidKeyboard::HidKeyboard()
    : mInputReport(nullptr)
    , mBootIn(nullptr)
    , mAdv(nullptr)
    , mConnected(false)
    , mProtocolMode(0x00)
    , mSubBootIn(false)
    , mSubReport(false)
    , mProtoCb(std::make_unique<HidKeyboardProtocolModeCallbacks>(*this))
    , mSubCb(std::make_unique<HidKeyboardInputSubscribeCallbacks>(*this))
    , mServerCb(std::make_unique<HidKeyboardServerCallbacks>(*this)) {
}

HidKeyboard::~HidKeyboard() = default;

/**
 * Initialisiert den NimBLE-Stack, legt den HID-Service samt
 * Charakteristiken an und startet anschließendes Advertising.
 *
 * Die Methode richtet ebenfalls einen Device-Information-Service ein und
 * hinterlegt die Report-Map, in der der Report mit der ID 0x01 definiert ist.
 * Dadurch weiß der Host, dass im Report-Modus ein führendes Report-ID-Byte
 * zu erwarten ist.
 */
void HidKeyboard::begin() {
    NimBLEDevice::init("ESP32 DE Keyboard");
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
    NimBLEDevice::setSecurityAuth(true, false, true);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    NimBLEServer* server = NimBLEDevice::createServer();
    server->setCallbacks(mServerCb.get());

    NimBLEService* hid = server->createService((uint16_t)UUID_HID_SERVICE);

    NimBLECharacteristic* chProtocol = hid->createCharacteristic(
        (uint16_t)UUID_PROTOCOL_MODE,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE_NR
    );
    chProtocol->setCallbacks(mProtoCb.get());
    chProtocol->setValue(&mProtocolMode, 1);

    NimBLECharacteristic* chHidInfo = hid->createCharacteristic(
        (uint16_t)UUID_HID_INFORMATION,
        NIMBLE_PROPERTY::READ
    );
    uint8_t hidInfo[4] = { 0x11, 0x01, 0x00, 0x02 };
    chHidInfo->setValue(hidInfo, sizeof(hidInfo));

    NimBLECharacteristic* chCtrlPt = hid->createCharacteristic(
        (uint16_t)UUID_HID_CONTROL_POINT,
        NIMBLE_PROPERTY::WRITE_NR
    );
    uint8_t ctl = 0x00;
    chCtrlPt->setValue(&ctl, 1);

    NimBLECharacteristic* chReportMap = hid->createCharacteristic(
        (uint16_t)UUID_REPORT_MAP,
        NIMBLE_PROPERTY::READ
    );
    chReportMap->setValue((uint8_t*)KEYBOARD_REPORTMAP, sizeof(KEYBOARD_REPORTMAP));

    mInputReport = hid->createCharacteristic(
        (uint16_t)UUID_REPORT,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    {
        NimBLEDescriptor* repRef = mInputReport->createDescriptor(
            (uint16_t)UUID_RPT_REF_DESC, NIMBLE_PROPERTY::READ, 2
        );
        uint8_t repRefVal[2] = { 0x01, 0x01 };
        repRef->setValue(repRefVal, sizeof(repRefVal));
    }
    mInputReport->setCallbacks(mSubCb.get());

    mBootIn = hid->createCharacteristic(
        (uint16_t)UUID_BOOT_KB_INPUT,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    mBootIn->setCallbacks(mSubCb.get());

    hid->createCharacteristic(
        (uint16_t)UUID_BOOT_KB_OUTPUT,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );

    hid->start();

    NimBLEService* dis = server->createService((uint16_t)0x180A);
    NimBLECharacteristic* cMan = dis->createCharacteristic((uint16_t)0x2A29, NIMBLE_PROPERTY::READ);
    cMan->setValue("TestCo");
    dis->start();

    mAdv = NimBLEDevice::getAdvertising();
    mAdv->addServiceUUID((uint16_t)UUID_HID_SERVICE);
    mAdv->setAppearance(961);
    mAdv->start();

    Serial.println("[BLE] Advertising als HID Keyboard. Bei Service-Änderungen Bond löschen & neu koppeln.");
    Serial.println("[INFO] Im seriellen Monitor tippen; Enter = Zeilenumbruch. (UTF-8)");
}

/**
 * Gibt an, ob aktuell eine BLE-Verbindung besteht.
 */
bool HidKeyboard::isConnected() const {
    return mConnected;
}

/**
 * Übersetzt einen UTF-32-Codepoint ins deutsche Tastaturlayout und sendet
 * anschließend einen Press/Release-Zyklus.
 *
 * Nicht abbildbare Zeichen erzeugen lediglich eine Debug-Ausgabe, so dass der
 * Aufrufer über fehlende Zuordnungen informiert ist.
 */
void HidKeyboard::typeCodepoint(uint32_t cp) {
    uint8_t mods;
    uint8_t key;
    if (!cpToHid_DE(cp, mods, key)) {
        if (cp <= 0x7F) {
            Serial.printf("[HID] Unmapped ASCII: 0x%02X '%c'\n", (unsigned)cp, (char)cp);
        } else {
            Serial.printf("[HID] Unmapped U+%04lX\n", (unsigned long)cp);
        }
        return;
    }
    pressAndRelease(mods, key);
}

/**
 * Callback-Handler für Schreibzugriffe auf die Protocol-Mode-Charakteristik.
 *
 * Der Host schreibt hier 0x00 (Boot) oder 0x01 (Report). Der Wert wird
 * validiert und anschließend gespeichert, damit zukünftige Reports in das
 * korrekte Format gebracht werden.
 */
void HidKeyboard::handleProtocolModeWrite(NimBLECharacteristic* characteristic) {
    std::string value = characteristic->getValue();
    if (value.size() == 1 && (value[0] == 0x00 || value[0] == 0x01)) {
        mProtocolMode = static_cast<uint8_t>(value[0]);
        Serial.printf("[HID] Host set ProtocolMode = %s\n", mProtocolMode ? "Report(1)" : "Boot(0)");
    }
}

/**
 * Aktualisiert die Merker, ob Boot- bzw. Report-Input-Charakteristiken
 * Benachrichtigungen senden dürfen.
 *
 * Der CCCD-Wert (subValue) wird daraufhin untersucht, ob das Notify-Bit
 * gesetzt ist. Nur wenn dies der Fall ist, werden spätere Reports auf dem
 * entsprechenden Kanal verschickt.
 */
void HidKeyboard::handleSubscription(NimBLECharacteristic* characteristic, uint16_t subValue) {
    const bool notifyEnabled = (subValue & 0x0001);
    if (characteristic == mBootIn) {
        mSubBootIn = notifyEnabled;
        Serial.printf("[HID] BootInput subscribe = %d\n", static_cast<int>(mSubBootIn));
    } else if (characteristic == mInputReport) {
        mSubReport = notifyEnabled;
        Serial.printf("[HID] ReportInput subscribe = %d\n", static_cast<int>(mSubReport));
    }
}

/**
 * Wird unmittelbar nach erfolgreichem Verbindungsaufbau aufgerufen.
 *
 * Nach einem kurzen Delay wird eine Testsequenz bestehend aus "Shift+A" und
 * Enter gesendet, damit sowohl Boot- als auch Report-Input eine initiale
 * Aktivität verzeichnen. Das hilft insbesondere bei Hosts, die erst nach dem
 * ersten Key-Event die Bildschirmanzeige aktivieren.
 */
void HidKeyboard::afterConnect() {
    mConnected = true;
    Serial.println("[BLE] Verbunden");
    unsigned long t0 = millis();
    while (millis() - t0 < 800) {
        delay(1);
    }

    auto sendRelease = [this]() {
        sendKeyReportRaw(mBootIn, "Boot", 0);
        sendKeyReportRaw(mInputReport, "Report", 0);
    };

    sendKeyReportRaw(mBootIn, "Boot", KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_A);
    sendKeyReportRaw(mInputReport, "Report", KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_A);
    delay(10);
    sendRelease();
    delay(10);

    sendKeyReportRaw(mBootIn, "Boot", 0, HID_KEY_ENTER);
    sendKeyReportRaw(mInputReport, "Report", 0, HID_KEY_ENTER);
    delay(10);
    sendRelease();

    Serial.printf("[HID] ProtocolMode on connect (may change) = %s\n", mProtocolMode ? "Report(1)" : "Boot(0)");
}

/**
 * Rücksetzen aller Statusmerker und Neustart des Advertisings nach
 * Verbindungsabbruch.
 */
void HidKeyboard::afterDisconnect() {
    mConnected = false;
    mSubBootIn = false;
    mSubReport = false;
    Serial.println("[BLE] Getrennt. Advertising neu...");
    NimBLEDevice::startAdvertising();
}

/**
 * Sendet einen Keyboard-Report an die angegebene Charakteristik.
 *
 * Für die Boot-Charakteristik besteht der Report aus 8 Bytes (Modifier,
 * Reserved, sechs Keycodes). Für den Report-Mode wird automatisch die
 * Report-ID 0x01 vorangestellt, womit der Report insgesamt 9 Bytes umfasst.
 *
 * @param characteristic Zielcharakteristik für den Report.
 * @param tag Debug-Tag für die serielle Ausgabe.
 * @param mods Modifier-Bits.
 * @param k1..k6 Bis zu sechs gleichzeitig gedrückte Tasten.
 */
void HidKeyboard::sendKeyReportRaw(
    NimBLECharacteristic* characteristic,
    const char* tag,
    uint8_t mods,
    uint8_t k1,
    uint8_t k2,
    uint8_t k3,
    uint8_t k4,
    uint8_t k5,
    uint8_t k6) {
    if (!characteristic) {
        return;
    }
    uint8_t rpt[9] = { 0 };
    size_t len = 0;
    if (characteristic == mInputReport) {
        rpt[0] = 0x01; // Report ID as defined in KEYBOARD_REPORTMAP
        rpt[1] = mods;
        rpt[2] = 0x00;
        rpt[3] = k1;
        rpt[4] = k2;
        rpt[5] = k3;
        rpt[6] = k4;
        rpt[7] = k5;
        rpt[8] = k6;
        len = 9;
    } else {
        rpt[0] = mods;
        rpt[1] = 0x00;
        rpt[2] = k1;
        rpt[3] = k2;
        rpt[4] = k3;
        rpt[5] = k4;
        rpt[6] = k5;
        rpt[7] = k6;
        len = 8;
    }
    characteristic->setValue(rpt, len);
    bool ok = characteristic->notify();
    Serial.printf("[HID] send %s notify=%d mods=%02X keys=%02X %02X %02X %02X %02X %02X\n",
        tag,
        static_cast<int>(ok),
        mods,
        k1,
        k2,
        k3,
        k4,
        k5,
        k6);
}

/**
 * Öffentliche Helfermethode, die abhängig vom Protokollmodus den passenden
 * Kanal wählt.
 *
 * Ist der Host noch nicht verbunden oder hat keine Benachrichtigungen
 * aktiviert, wird der Report vorsorglich an beide Charakteristiken gesendet,
 * damit kein Tastenanschlag verloren geht.
 */
void HidKeyboard::sendKeyReport(
    uint8_t mods,
    uint8_t k1,
    uint8_t k2,
    uint8_t k3,
    uint8_t k4,
    uint8_t k5,
    uint8_t k6) {
    if (!mConnected) {
        return;
    }

    if (mProtocolMode == 0x00) {
        if (mSubBootIn) {
            sendKeyReportRaw(mBootIn, "Boot", mods, k1, k2, k3, k4, k5, k6);
            return;
        }
    } else {
        if (mSubReport) {
            sendKeyReportRaw(mInputReport, "Report", mods, k1, k2, k3, k4, k5, k6);
            return;
        }
    }

    sendKeyReportRaw(mBootIn, "Boot*", mods, k1, k2, k3, k4, k5, k6);
    sendKeyReportRaw(mInputReport, "Report*", mods, k1, k2, k3, k4, k5, k6);
}

/**
 * Sendet erst einen Tastendruck und anschließend – nach kurzen Delays – den
 * zugehörigen Release-Report.
 */
void HidKeyboard::pressAndRelease(uint8_t mods, uint8_t key) {
    sendKeyReport(mods, key);
    delay(8);
    sendKeyReport(0);
    delay(5);
}

/**
 * Konvertiert einen Codepoint in das deutsche HID-Layout.
 *
 * Neben den Standard-ASCII-Zeichen werden auch Umlaute und Sonderzeichen
 * behandelt. Modifier-Bits (Shift/AltGr) werden nach Bedarf gesetzt.
 */
bool HidKeyboard::cpToHid_DE(uint32_t cp, uint8_t& mods, uint8_t& key) {
    mods = 0;
    if (cp == '\n' || cp == '\r') { key = HID_KEY_ENTER; return true; }
    if (cp == '\t') { key = HID_KEY_TAB; return true; }
    if (cp == ' ') { key = HID_KEY_SPACEBAR; return true; }

    if (cp >= '1' && cp <= '9') { key = static_cast<uint8_t>(HID_KEY_1 + (cp - '1')); return true; }
    if (cp == '0') { key = HID_KEY_0; return true; }

    if (cp >= 'a' && cp <= 'z') {
        if (cp == 'z') { key = HID_KEY_Y; return true; }
        if (cp == 'y') { key = HID_KEY_Z; return true; }
        key = static_cast<uint8_t>(HID_KEY_A + (cp - 'a'));
        return true;
    }
    if (cp >= 'A' && cp <= 'Z') {
        if (cp == 'Z') { key = HID_KEY_Y; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true; }
        if (cp == 'Y') { key = HID_KEY_Z; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true; }
        key = static_cast<uint8_t>(HID_KEY_A + (cp - 'A'));
        mods = KEYBOARD_MODIFIER_LEFTSHIFT;
        return true;
    }

    switch (cp) {
    case 0x00E4: key = HID_KEY_APOSTROPHE; return true;
    case 0x00C4: key = HID_KEY_APOSTROPHE; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case 0x00F6: key = HID_KEY_SEMICOLON;  return true;
    case 0x00D6: key = HID_KEY_SEMICOLON;  mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case 0x00FC: key = HID_KEY_LEFT_BRACKET; return true;
    case 0x00DC: key = HID_KEY_LEFT_BRACKET; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case 0x00DF: key = HID_KEY_MINUS; return true;
    case 0x20AC: mods = KEYBOARD_MODIFIER_RIGHTALT; key = HID_KEY_E; return true;
    }

    switch (cp) {
    case '.': key = HID_KEY_PERIOD; return true;
    case ',': key = HID_KEY_COMMA; return true;
    case '-': key = HID_KEY_SLASH; return true;
    case '_': key = HID_KEY_SLASH; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case '+': key = HID_KEY_RIGHT_BRACKET; return true;
    case '*': key = HID_KEY_RIGHT_BRACKET; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case ':': key = HID_KEY_PERIOD; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case ';': key = HID_KEY_COMMA;  mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case '!': key = HID_KEY_1; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case '?': key = HID_KEY_MINUS; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case '=': key = HID_KEY_EQUAL; return true;

    case '/':  key = HID_KEY_7;  mods = KEYBOARD_MODIFIER_RIGHTALT; return true;
    case '\\': key = HID_KEY_MINUS; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;
    case '#':  key = HID_KEY_BACKSLASH; return true;
    case '"':  key = HID_KEY_2; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case '\'': key = HID_KEY_BACKSLASH; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;

    case '(': key = HID_KEY_8; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case ')': key = HID_KEY_9; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case '[': key = HID_KEY_8; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;
    case ']': key = HID_KEY_9; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;
    case '{': key = HID_KEY_7; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;
    case '}': key = HID_KEY_0; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;
    case '<': key = HID_KEY_NON_US_BACKSLASH; return true;
    case '>': key = HID_KEY_NON_US_BACKSLASH; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case '|': key = HID_KEY_NON_US_BACKSLASH; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;
    }
    return false;
}

