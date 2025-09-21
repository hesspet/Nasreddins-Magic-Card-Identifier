#include "NdefHelper.h"
#include <ctype.h>

bool NdefHelper::parseNextTlv(const uint8_t* start, const uint8_t* end, Tlv& out) const {
	if (!start || start >= end) return false;
	const uint8_t* p = start;

	while (p < end && *p == 0x00) {
		++p;
	}
	if (p >= end) return false;

	uint8_t tag = *p++;
	if (tag == 0xFE) {
		out.tag = 0xFE;
		out.value = nullptr;
		out.length = 0;
		out.next = nullptr;
		return true;
	}

	if (p >= end) return false;

	size_t len = 0;
	if (*p == 0xFF) {
		++p;
		if (p + 1 >= end) return false;
		len = ((size_t)p[0] << 8) | (size_t)p[1];
		p += 2;
	}
	else {
		len = *p++;
	}

	if ((size_t)(end - p) < len) return false;

	out.tag = tag;
	out.value = p;
	out.length = len;
	const uint8_t* next = p + len;
	out.next = (next < end) ? next : nullptr;
	return true;
}

bool NdefHelper::findFirstNdefTlv(const uint8_t* data, size_t length, Tlv& out) const {
	if (!data || length == 0) return false;
	const uint8_t* cursor = data;
	const uint8_t* end = data + length;
	Tlv current;
	while (parseNextTlv(cursor, end, current)) {
		if (current.tag == 0x03) {
			out = current;
			return true;
		}
		if (current.tag == 0xFE || !current.next) {
			break;
		}
		cursor = current.next;
	}
	return false;
}

bool NdefHelper::parseFirstRecord(const uint8_t* msg, size_t msgLen, NdefRecord& rec) const {
	if (!msg || msgLen < 3) return false;
	const uint8_t* p = msg;

	uint8_t hdr = *p++;
	rec.mb = hdr & 0x80;
	rec.me = hdr & 0x40;
	bool cf = hdr & 0x20;
	rec.sr = hdr & 0x10;
	rec.il = hdr & 0x08;
	rec.tnf = hdr & 0x07;
	if (cf) return false;

	if ((size_t)(msg + msgLen - p) < 1) return false;
	rec.typeLen = *p++;

	if (rec.sr) {
		if ((size_t)(msg + msgLen - p) < 1) return false;
		rec.payloadLen = *p++;
	}
	else {
		if ((size_t)(msg + msgLen - p) < 4) return false;
		rec.payloadLen = ((uint32_t)p[0] << 24) |
			((uint32_t)p[1] << 16) |
			((uint32_t)p[2] << 8) |
			(uint32_t)p[3];
		p += 4;
	}

	if (rec.il) {
		if ((size_t)(msg + msgLen - p) < 1) return false;
		rec.idLen = *p++;
	}
	else {
		rec.idLen = 0;
	}

	if ((size_t)(msg + msgLen - p) < rec.typeLen) return false;
	rec.type = p;
	p += rec.typeLen;

	if (rec.idLen) {
		if ((size_t)(msg + msgLen - p) < rec.idLen) return false;
		rec.id = p;
		p += rec.idLen;
	}
	else {
		rec.id = nullptr;
	}

	if ((size_t)(msg + msgLen - p) < rec.payloadLen) return false;
	rec.payload = p;
	return true;
}

bool NdefHelper::isTextRecord(const NdefRecord& rec) const {
	return rec.tnf == 0x01 &&
		rec.typeLen == 1 &&
		rec.type != nullptr &&
		rec.type[0] == 'T';
}

void NdefHelper::decodeAndPrintTextRecord(const NdefRecord& r) const {
	if (!isTextRecord(r)) {
		return;
	}
	if (r.payloadLen < 1) {
		Serial.println(F("Empty RTD/T payload"));
		return;
	}

	uint8_t status = r.payload[0];
	bool utf16 = (status & 0x80) != 0;
	uint8_t langLen = (status & 0x3F);
	if (r.payloadLen < (size_t)(1 + langLen)) {
		Serial.println(F("RTD/T payload too short"));
		return;
	}

	String lang;
	for (uint8_t i = 0; i < langLen; ++i) {
		lang += (char)r.payload[1 + i];
	}
	const uint8_t* textPtr = r.payload + 1 + langLen;
	size_t textLen = r.payloadLen - 1 - langLen;

	Serial.print(F("NDEF Text: ("));
	Serial.print(utf16 ? F("UTF-16") : F("UTF-8"));
	Serial.print(F(", "));
	Serial.print(lang);
	Serial.println(F(")"));
	Serial.println(F("Text payload:"));
	if (utf16) {
		dumpHexAscii(textPtr, textLen);
	}
	else {
		for (size_t i = 0; i < textLen; ++i) {
			char c = (char)textPtr[i];
			Serial.print(isprint((unsigned char)c) ? c : '.');
		}
		Serial.println();
	}
}

void NdefHelper::dumpHexAscii(const uint8_t* data, size_t len) const {
	Serial.print(F("Data ("));
	Serial.print(len);
	Serial.println(F(" bytes):"));
	for (size_t i = 0; i < len; i += 16) {
		Serial.printf("%04u: ", (unsigned)i);
		for (size_t j = 0; j < 16; ++j) {
			if (i + j < len) {
				Serial.printf("%02X ", data[i + j]);
			}
			else {
				Serial.print("   ");
			}
		}
		Serial.print(" | ");
		for (size_t j = 0; j < 16 && (i + j) < len; ++j) {
			char c = (char)data[i + j];
			Serial.print(isprint((unsigned char)c) ? c : '.');
		}
		Serial.println();
	}
}
