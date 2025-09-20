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

#include <PN532_HSU.h>
#include <PN532.h>
#include <ctype.h>
#include <string.h>

#define PN532_HSU_PORT   Serial2
static const uint32_t PN532_HSU_BAUDRATE = 115200;
static const int      PN532_HSU_RX_PIN = 26;  // ESP32 RX  (an PN532 TX)
static const int      PN532_HSU_TX_PIN = 25;  // ESP32 TX  (an PN532 RX)

static PN532_HSU pn532hsu(PN532_HSU_PORT);
static PN532     nfc(pn532hsu);

namespace {

	constexpr uint8_t  kUidBufferMax = 10;    // 4/7/10 Byte UIDs
	constexpr uint16_t kUserMaxBytes = 1024;  // reicht für NTAG216 (888 B)
	constexpr uint8_t  kBytesPerPage = 4;
	constexpr uint8_t  kFirstUserPage = 4;

	struct TagInfo {
		bool     ccValid = false;
		uint8_t  ccMagic = 0x00;   // 0xE1 erwartet
		uint8_t  verMaj = 0;
		uint8_t  verMin = 0;
		uint8_t  size8 = 0;      // Anzahl 8-Byte-Blöcke
		uint8_t  access = 0;      // 0x00 meist RW, 0x0F RO
		uint16_t userBytes = 0;    // size8 * 8
		uint16_t userPages = 0;    // userBytes / 4
		const char* probableType = "Unknown Type 2";
	};

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

	/* ---------- CC lesen (Page 3) und TagInfo ableiten ---------- */
	bool readCapabilityContainer(TagInfo& out) {
		uint8_t page3[4] = { 0 };
		if (!nfc.mifareultralight_ReadPage(3, page3)) {
			return false;
		}
		out.ccMagic = page3[0];
		out.verMaj = (page3[1] >> 4) & 0x0F;
		out.verMin = page3[1] & 0x0F;
		out.size8 = page3[2];
		out.access = page3[3];

		out.ccValid = (out.ccMagic == 0xE1); // NDEF-kompatibel

		out.userBytes = (uint16_t)out.size8 * 8u;
		out.userPages = out.userBytes / kBytesPerPage;

		// Heuristik zur Typ-Benennung per Data Area Size
		// (häufige Werte – Hersteller-Varianten möglich)
		switch (out.size8) {
		case 0x06: out.probableType = "MIFARE Ultralight (48 B user)"; break;   // 48 B
		case 0x0C: out.probableType = "MIFARE Ultralight C (96 B user)"; break; // 96 B
		case 0x12: out.probableType = "NTAG213 (144 B user)"; break;            // 144 B
		case 0x3F: out.probableType = "NTAG215 (504 B user)"; break;            // 504 B
		case 0x6F: out.probableType = "NTAG216 (888 B user)"; break;            // 888 B
		default:   out.probableType = "Type 2 (unknown capacity)"; break;
		}
		return true;
	}

	/* ---------- Gesamten User-Memory lesen (ab Page 4) ---------- */
	bool readUserMemoryDynamic(uint8_t* buffer, size_t bufferCap, const TagInfo& ti) {
		if (!buffer || ti.userBytes == 0) return false;
		if (ti.userBytes > bufferCap)     return false;

		for (uint16_t i = 0; i < ti.userPages; ++i) {
			if (!nfc.mifareultralight_ReadPage(kFirstUserPage + i,
				buffer + i * kBytesPerPage)) {
				return false;
			}
		}
		return true;
	}

	/* ---------- TLV Parsing ---------- */
	struct Tlv {
		uint8_t tag;
		const uint8_t* value;
		size_t length;
		const uint8_t* next;
	};

	bool parseNextTlv(const uint8_t* start, const uint8_t* end, Tlv& out) {
		if (!start || start >= end) return false;
		const uint8_t* p = start;

		while (p < end && *p == 0x00) ++p; // NULL TLVs
		if (p >= end) return false;

		uint8_t tag = *p++;
		if (tag == 0xFE) { // Terminator
			out.tag = 0xFE; out.value = nullptr; out.length = 0; out.next = nullptr;
			return true;
		}

		if (p >= end) return false;

		size_t len = 0;
		if (*p == 0xFF) { // extended length
			++p; if (p + 1 >= end) return false;
			len = ((size_t)p[0] << 8) | (size_t)p[1]; p += 2;
		}
		else {
			len = *p++;
		}

		if ((size_t)(end - p) < len) return false;

		out.tag = tag; out.value = p; out.length = len;
		const uint8_t* next = p + len;
		out.next = (next < end) ? next : nullptr;
		return true;
	}

	/* ---------- NDEF Parsing (erster Record) ---------- */
	struct NdefRecord {
		uint8_t tnf; bool mb; bool me; bool sr; bool il;
		uint8_t  typeLen; uint32_t payloadLen; uint8_t idLen;
		const uint8_t* type; const uint8_t* id; const uint8_t* payload;
	};

	bool parseFirstNdefRecord(const uint8_t* msg, size_t msgLen, NdefRecord& rec) {
		if (!msg || msgLen < 3) return false;
		const uint8_t* p = msg;

		uint8_t hdr = *p++;
		rec.mb = hdr & 0x80; rec.me = hdr & 0x40;
		bool cf = hdr & 0x20; rec.sr = hdr & 0x10; rec.il = hdr & 0x08;
		rec.tnf = hdr & 0x07;
		if (cf) return false; // keine Chunk-Unterstützung

		if ((size_t)(msg + msgLen - p) < 1) return false;
		rec.typeLen = *p++;

		if (rec.sr) {
			if ((size_t)(msg + msgLen - p) < 1) return false;
			rec.payloadLen = *p++;
		}
		else {
			if ((size_t)(msg + msgLen - p) < 4) return false;
			rec.payloadLen = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
				((uint32_t)p[2] << 8) | ((uint32_t)p[3]); p += 4;
		}

		if (rec.il) { if ((size_t)(msg + msgLen - p) < 1) return false; rec.idLen = *p++; }
		else rec.idLen = 0;

		if ((size_t)(msg + msgLen - p) < rec.typeLen) return false;
		rec.type = p; p += rec.typeLen;

		if (rec.idLen) { if ((size_t)(msg + msgLen - p) < rec.idLen) return false; rec.id = p; p += rec.idLen; }
		else rec.id = nullptr;

		if ((size_t)(msg + msgLen - p) < rec.payloadLen) return false;
		rec.payload = p;
		return true;
	}

	void decodeAndPrintTextRecord(const NdefRecord& r) {
		if (!(r.tnf == 0x01 && r.typeLen == 1 && r.type && r.type[0] == 'T')) return;
		if (r.payloadLen < 1) { Serial.println(F("Empty RTD/T payload")); return; }

		uint8_t status = r.payload[0];
		bool utf16 = (status & 0x80) != 0;
		uint8_t langLen = (status & 0x3F);
		if (r.payloadLen < (size_t)(1 + langLen)) { Serial.println(F("RTD/T payload too short")); return; }

		String lang; for (uint8_t i = 0; i < langLen; ++i) lang += (char)r.payload[1 + i];
		const uint8_t* textPtr = r.payload + 1 + langLen;
		size_t textLen = r.payloadLen - 1 - langLen;

		Serial.print(F("NDEF Text: ("));
		Serial.print(utf16 ? F("UTF-16") : F("UTF-8"));
		Serial.print(F(", ")); Serial.print(lang); Serial.println(F(")"));
		Serial.println(F("Text payload:"));
		if (utf16) dumpHexAscii(textPtr, textLen);
		else {
			for (size_t i = 0; i < textLen; ++i) {
				char c = (char)textPtr[i];
				Serial.print(isprint((unsigned char)c) ? c : '.');
			}
			Serial.println();
		}
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

		if (newTagDetected) {
			tagPresent = true;
			lastUidLength = uidLength;
			memcpy(lastUid, uid, uidLength);

			Serial.print(F("Tag detected. UID length: "));
			Serial.println(uidLength);
			Serial.print(F("UID: ")); nfc.PrintHex(uid, uidLength);

			// --- 1) CC lesen & interpretieren ---
			TagInfo ti{};
			if (!readCapabilityContainer(ti)) {
				Serial.println(F("Failed to read Capability Container (page 3)"));
				return;
			}

			Serial.print(F("CC: Magic=0x")); Serial.print(ti.ccMagic, HEX);
			Serial.print(F(", Ver=")); Serial.print(ti.verMaj); Serial.print('.'); Serial.print(ti.verMin);
			Serial.print(F(", Size8=0x")); Serial.print(ti.size8, HEX);
			Serial.print(F(" (")); Serial.print(ti.userBytes); Serial.print(F(" bytes user)"));
			Serial.print(F(", Access=0x")); Serial.print(ti.access, HEX);
			Serial.println();

			if (!ti.ccValid) {
				Serial.println(F("Warning: CC Magic != 0xE1 (evtl. kein NDEF-Tag oder CC korrupt)"));
			}

			Serial.print(F("Probable type: "));
			Serial.println(ti.probableType);

			// --- 2) User Memory vollständig lesen (dynamisch) ---
			static uint8_t user[kUserMaxBytes];
			if (!readUserMemoryDynamic(user, sizeof(user), ti)) {
				Serial.println(F("Failed to read user memory"));
				return;
			}

			// Optional: Rohdump
			// dumpHexAscii(user, ti.userBytes);

			// --- 3) TLV scannen: NDEF (0x03) finden ---
			const uint8_t* p = user;
			const uint8_t* end = user + ti.userBytes;

			Tlv tlv{};
			bool foundNdef = false;
			while (parseNextTlv(p, end, tlv)) {
				if (tlv.tag == 0x03) { foundNdef = true; break; }
				if (tlv.tag == 0xFE) break;
				if (!tlv.next) break;
				p = tlv.next;
			}

			if (foundNdef) {
				Serial.print(F("NDEF length (TLV): "));
				Serial.println((unsigned)tlv.length);

				// --- 4) Ersten NDEF-Record parsen & bei Text ausgeben ---
				NdefRecord rec{};
				if (parseFirstNdefRecord(tlv.value, tlv.length, rec)) {
					// Wenn es ein Text-Record ist, gib den Text direkt aus (wie bei dir genutzt)
					if (rec.tnf == 0x01 && rec.typeLen == 1 && rec.type && rec.type[0] == 'T') {
						decodeAndPrintTextRecord(rec);
					}
					else {
						Serial.println(F("First NDEF record is not a Text (RTD/T) record."));
						Serial.println(F("Record header / payload (hex):"));
						dumpHexAscii(tlv.value, tlv.length);
					}
				}
				else {
					Serial.println(F("Failed to parse first NDEF record"));
				}
			}
			else {
				Serial.println(F("No NDEF TLV found (0x03)"));
			}
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
