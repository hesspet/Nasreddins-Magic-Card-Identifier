/**************************************************************************/
/*!
  ESP32 T-Display + PN532 (HSU/UART)
  Liest NTAG213-User-Memory (Pages 4..39), parst TLV + NDEF,
  dekodiert Text-Record (RTD/T) oder MIME-Record und gibt Inhalt aus.
*/
/**************************************************************************/

#include <PN532_HSU.h>
#include <PN532.h>
#include <ctype.h>
#include <string.h>

// --- PN532 an Serial2 (HSU) ---
#define PN532_HSU_PORT   Serial2
static const uint32_t PN532_HSU_BAUDRATE = 115200;
static const int      PN532_HSU_RX_PIN = 26;  // ESP32 RX  (an PN532 TX)
static const int      PN532_HSU_TX_PIN = 25;  // ESP32 TX  (an PN532 RX)

static PN532_HSU pn532hsu(PN532_HSU_PORT);
static PN532     nfc(pn532hsu);

namespace {

    // NTAG213: User-Memory pages 4..39 (36 Seiten) → 144 Bytes
    constexpr uint8_t kFirstUserPage = 4;
    constexpr uint8_t kUserPageCount = 36;
    constexpr uint8_t kBytesPerPage = 4;
    constexpr size_t  kUserBytes = kUserPageCount * kBytesPerPage; // 144 Bytes

    constexpr uint8_t kUidBufferMax = 10;

    // --- Utility ---
    bool isSameUid(const uint8_t* lhs, const uint8_t* rhs, uint8_t len) {
        return lhs && rhs && (memcmp(lhs, rhs, len) == 0);
    }

    void dumpHexAscii(const uint8_t* data, size_t len) {
        Serial.print(F("Data (")); Serial.print(len); Serial.println(F(" bytes):"));
        for (size_t i = 0; i < len; i += 16) {
            Serial.printf("%04u: ", (unsigned)i);
            for (size_t j = 0; j < 16; ++j) {
                if (i + j < len) Serial.printf("%02X ", data[i + j]);
                else             Serial.print("   ");
            }
            Serial.print(" | ");
            for (size_t j = 0; j < 16 && (i + j) < len; ++j) {
                char c = (char)data[i + j];
                Serial.print(isprint((unsigned char)c) ? c : '.');
            }
            Serial.println();
        }
    }

    // --- Lesen des kompletten User-Speichers (Pages 4..39) ---
    bool readUserMemory(uint8_t* buffer, size_t length) {
        if (!buffer || length < kUserBytes) return false;
        for (uint8_t i = 0; i < kUserPageCount; ++i) {
            if (!nfc.mifareultralight_ReadPage(kFirstUserPage + i,
                buffer + i * kBytesPerPage)) {
                return false;
            }
        }
        return true;
    }

    // --- TLV Parsing (0x00 NULL, 0x01 Lock, 0x02 Mem, 0x03 NDEF, 0xFE Term) ---
    struct Tlv {
        uint8_t tag;
        const uint8_t* value; // Zeiger auf Value-Beginn innerhalb des Puffers
        size_t length;        // Wertlänge in Bytes
        const uint8_t* next;  // Zeiger auf nächstes TLV (oder nullptr)
    };

    bool parseNextTlv(const uint8_t* start, const uint8_t* end, Tlv& out) {
        if (!start || start >= end) return false;
        const uint8_t* p = start;

        // Überspringe NULL TLVs (0x00)
        while (p < end && *p == 0x00) ++p;
        if (p >= end) return false;

        uint8_t tag = *p++;
        if (tag == 0xFE) { // Terminator
            out.tag = 0xFE;
            out.value = nullptr;
            out.length = 0;
            out.next = nullptr;
            return true;
        }

        if (p >= end) return false;

        size_t len = 0;
        if (*p == 0xFF) {
            // Extended length (3 Byte Länge): 0xFF, then two following bytes = length
            ++p;
            if (p + 1 >= end) return false;
            len = (size_t)p[0] << 8 | (size_t)p[1];
            p += 2;
        }
        else {
            len = *p++;
        }

        if ((size_t)(end - p) < len) {
            // Länge überschreitet Puffer – unvollständig
            return false;
        }

        out.tag = tag;
        out.value = p;
        out.length = len;
        const uint8_t* next = p + len;
        out.next = (next < end) ? next : nullptr;
        return true;
    }

    // --- NDEF Record Parsing (einfach: SR/!SR, IL optional) ---
    struct NdefRecord {
        uint8_t tnf;               // 0..7
        bool mb;
        bool me;
        bool sr;
        bool il;
        uint8_t typeLen;           // 1 Byte
        uint32_t payloadLen;       // 1 oder 4 Bytes je nach SR
        uint8_t idLen;             // falls IL gesetzt
        const uint8_t* type;       // zeigt in Message
        const uint8_t* id;         // optional
        const uint8_t* payload;    // zeigt in Message
    };

    bool parseFirstNdefRecord(const uint8_t* msg, size_t msgLen, NdefRecord& rec) {
        if (!msg || msgLen < 3) return false;

        const uint8_t* p = msg;
        uint8_t hdr = *p++;

        rec.mb = (hdr & 0x80) != 0;
        rec.me = (hdr & 0x40) != 0;
        bool cf = (hdr & 0x20) != 0; // Chunk Flag (ignorieren, nicht unterstützt)
        rec.sr = (hdr & 0x10) != 0;
        rec.il = (hdr & 0x08) != 0;
        rec.tnf = (hdr & 0x07);

        if (cf) return false; // Einfachheit: keine Chunked Records unterstützen

        if ((size_t)(msg + msgLen - p) < 1) return false;
        rec.typeLen = *p++;

        if (rec.sr) {
            if ((size_t)(msg + msgLen - p) < 1) return false;
            rec.payloadLen = *p++;
        }
        else {
            if ((size_t)(msg + msgLen - p) < 4) return false;
            rec.payloadLen = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
                ((uint32_t)p[2] << 8) | ((uint32_t)p[3]);
            p += 4;
        }

        if (rec.il) {
            if ((size_t)(msg + msgLen - p) < 1) return false;
            rec.idLen = *p++;
        }
        else {
            rec.idLen = 0;
        }

        // Type
        if ((size_t)(msg + msgLen - p) < rec.typeLen) return false;
        rec.type = p; p += rec.typeLen;

        // ID (optional)
        if (rec.idLen) {
            if ((size_t)(msg + msgLen - p) < rec.idLen) return false;
            rec.id = p; p += rec.idLen;
        }
        else {
            rec.id = nullptr;
        }

        // Payload
        if ((size_t)(msg + msgLen - p) < rec.payloadLen) return false;
        rec.payload = p; // p + payloadLen wäre Ende des Records
        return true;
    }

    // --- Dekodierung gängiger Record-Typen ---
    void decodeAndPrintRecord(const NdefRecord& r) {
        // Well-known Text (RTD/T)
        if (r.tnf == 0x01 && r.typeLen == 1 && r.type && r.type[0] == 'T') {
            if (r.payloadLen < 1) { Serial.println(F("Empty RTD/T payload")); return; }
            uint8_t status = r.payload[0];
            bool utf16 = (status & 0x80) != 0;
            uint8_t langLen = (status & 0x3F);

            if (r.payloadLen < (size_t)(1 + langLen)) {
                Serial.println(F("RTD/T payload too short"));
                return;
            }

            const char* enc = utf16 ? "UTF-16" : "UTF-8";
            String lang;
            for (uint8_t i = 0; i < langLen; ++i) lang += (char)r.payload[1 + i];

            size_t textLen = r.payloadLen - 1 - langLen;
            const uint8_t* textPtr = r.payload + 1 + langLen;

            Serial.print(F("NDEF Text: ("));
            Serial.print(enc);
            Serial.print(F(", "));
            Serial.print(lang);
            Serial.println(F(")"));

            // Bei UTF-16 ist reine Serial-Ausgabe heikel; hier Hex + ggf. Versuch:
            if (utf16) {
                Serial.println(F("UTF-16 payload (hex):"));
                dumpHexAscii(textPtr, textLen);
            }
            else {
                // Versuche als String auszugeben (nicht null-terminiert)
                Serial.println(F("Text payload:"));
                for (size_t i = 0; i < textLen; ++i) {
                    char c = (char)textPtr[i];
                    Serial.print(isprint((unsigned char)c) ? c : '.');
                }
                Serial.println();
            }
            return;
        }

        // MIME Media (TNF=0x02), z. B. "text/plain"
        if (r.tnf == 0x02 && r.type && r.typeLen > 0) {
            Serial.print(F("NDEF MIME type: "));
            for (uint8_t i = 0; i < r.typeLen; ++i) Serial.print((char)r.type[i]);
            Serial.println();

            Serial.println(F("Payload:"));
            dumpHexAscii(r.payload, r.payloadLen);
            return;
        }

        // Well-known URI (RTD/URI = 'U')
        if (r.tnf == 0x01 && r.typeLen == 1 && r.type[0] == 'U' && r.payloadLen >= 1) {
            // URI Identifier Code Tabelle wäre hier nötig; wir dumpen payload roh:
            Serial.println(F("NDEF URI record payload (raw):"));
            dumpHexAscii(r.payload, r.payloadLen);
            return;
        }

        // Fallback
        Serial.print(F("Unhandled NDEF record: TNF="));
        Serial.print(r.tnf);
        Serial.print(F(", Type=\""));
        for (uint8_t i = 0; i < r.typeLen; ++i) Serial.print((char)r.type[i]);
        Serial.println(F("\""));
        Serial.println(F("Payload (hex):"));
        dumpHexAscii(r.payload, r.payloadLen);
    }

} // namespace


void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }

    PN532_HSU_PORT.begin(PN532_HSU_BAUDRATE, SERIAL_8N1,
        PN532_HSU_RX_PIN, PN532_HSU_TX_PIN);

    nfc.begin();

    uint32_t versiondata = nfc.getFirmwareVersion();
    if (!versiondata) {
        Serial.println(F("PN532 not found (check wiring & HSU mode)"));
        while (true) { delay(1000); }
    }

    Serial.print(F("Found chip PN5"));
    Serial.println((versiondata >> 24) & 0xFF, HEX);
    Serial.print(F("Firmware ver. "));
    Serial.print((versiondata >> 16) & 0xFF, DEC);
    Serial.print('.');
    Serial.println((versiondata >> 8) & 0xFF, DEC);

    nfc.SAMConfig();
    nfc.setPassiveActivationRetries(0xFF);

    Serial.println(F("Waiting for an ISO14443A Card ..."));
}

void loop() {
    static uint8_t lastUid[kUidBufferMax] = { 0 };
    static uint8_t lastUidLength = 0;
    static bool    tagPresent = false;

    uint8_t uid[kUidBufferMax] = { 0 };
    uint8_t uidLength = 0;

    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
        bool newTagDetected = (!tagPresent) ||
            (uidLength != lastUidLength) ||
            (!isSameUid(uid, lastUid, uidLength));

        if (newTagDetected) 
        {
            tagPresent = true;
            lastUidLength = uidLength;
            memcpy(lastUid, uid, uidLength);

            Serial.print(F("Tag detected. UID length: "));
            Serial.println(uidLength);
            Serial.print(F("UID: "));
            nfc.PrintHex(uid, uidLength);

            // 1) User-Memory lesen
            uint8_t user[kUserBytes] = { 0 };
            if (!readUserMemory(user, sizeof(user))) {
                Serial.println(F("Failed to read user memory (pages 4..39)"));
                return;
            }

            // Optional: Rohdump
            // dumpHexAscii(user, sizeof(user));

            // 2) TLV finden: NDEF (0x03)
            const uint8_t* p = user;
            const uint8_t* end = user + sizeof(user);

            Tlv tlv{};
            bool foundNdef = false;
            while (parseNextTlv(p, end, tlv)) {
                if (tlv.tag == 0x03) { // NDEF Message TLV
                    foundNdef = true;
                    break;
                }
                if (tlv.tag == 0xFE) break; // Terminator
                if (!tlv.next) break;
                p = tlv.next;
            }

            if (!foundNdef) {
                Serial.println(F("No NDEF TLV found"));
                return;
            }

            Serial.print(F("NDEF length (TLV): "));
            Serial.println((unsigned)tlv.length);

            // 3) Ersten NDEF-Record parsen
            NdefRecord rec{};
            if (!parseFirstNdefRecord(tlv.value, tlv.length, rec)) {
                Serial.println(F("Failed to parse first NDEF record"));
                return;
            }

            // 4) Inhalt interpretieren/ausgeben
            decodeAndPrintRecord(rec);
        }
    }
    else if (tagPresent) {
        tagPresent = false;
        lastUidLength = 0;
        memset(lastUid, 0, sizeof(lastUid));
        Serial.println(F("Tag removed"));
    }

    delay(250);
}
