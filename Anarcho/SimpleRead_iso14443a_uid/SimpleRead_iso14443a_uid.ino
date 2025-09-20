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
#include <string.h>

#include "NdefHelper.h"

#define PN532_HSU_PORT   Serial2
static const uint32_t PN532_HSU_BAUDRATE = 115200;
static const int      PN532_HSU_RX_PIN = 26;  // ESP32 RX  (an PN532 TX)
static const int      PN532_HSU_TX_PIN = 25;  // ESP32 TX  (an PN532 RX)

static PN532_HSU pn532hsu(PN532_HSU_PORT);
static PN532     nfc(pn532hsu);

constexpr uint8_t  kUidBufferMax = 10;    // 4/7/10 Byte UIDs
constexpr uint16_t kUserMaxBytes = 1024;  // reicht für NTAG216 (888 B)
constexpr uint8_t  kBytesPerPage = 4;
constexpr uint8_t  kFirstUserPage = 4;

static NdefHelper ndefHelper;

struct Type2TagInfo {
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

/* ---------- CC lesen (Page 3) und Type2TagInfo ableiten ---------- */
bool readCapabilityContainer(Type2TagInfo& out) {
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
bool readUserMemoryDynamic(uint8_t* buffer, size_t bufferCap, const Type2TagInfo& ti) {
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
			Type2TagInfo ti{};
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
			// ndefHelper.dumpHexAscii(user, ti.userBytes);

			// --- 3) TLV scannen: NDEF (0x03) finden ---
			NdefHelper::Tlv tlv{};
			bool foundNdef = ndefHelper.findFirstNdefTlv(user, ti.userBytes, tlv);

			if (foundNdef) {
				Serial.print(F("NDEF length (TLV): "));
				Serial.println((unsigned)tlv.length);

				// --- 4) Ersten NDEF-Record parsen & bei Text ausgeben ---
				NdefHelper::NdefRecord rec{};
				if (ndefHelper.parseFirstRecord(tlv.value, tlv.length, rec)) {
					// Wenn es ein Text-Record ist, gib den Text direkt aus (wie bei dir genutzt)
					if (ndefHelper.isTextRecord(rec)) {
						ndefHelper.decodeAndPrintTextRecord(rec);
					}
					else {
						Serial.println(F("First NDEF record is not a Text (RTD/T) record."));
						Serial.println(F("Record header / payload (hex):"));
						ndefHelper.dumpHexAscii(tlv.value, tlv.length);
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
