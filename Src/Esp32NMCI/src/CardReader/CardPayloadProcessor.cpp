/***************************************************************************
 * Project: Esp32NMCI
 * File: CardPayloadProcessor.cpp
 * Repository: https://github.com/hesspet/NasrredinsMagicCardIdentifier
 * Author: Peter Heß, Büdingen DE
 * Description: Implements splitting payload text into up to five sanitized
 *              values and logs exceptional input conditions.
 ***************************************************************************/

#include "CardPayloadProcessor.h"

#include "../Logger/Logger.h"

namespace
{
	constexpr size_t kMaxElements = 5;
}

std::vector<String> CardPayloadProcessor::ProcessPayload(const String& payload)
{
	std::vector<String> elements;
	elements.reserve(kMaxElements);

	String trimmedPayload = payload;
	trimmedPayload.trim();

	if (trimmedPayload.length() == 0)
	{
		return elements;
	}

	// Enforce a single line payload by discarding everything after the first newline.
	int newlineIndex = trimmedPayload.indexOf('\n');
	int carriageReturnIndex = trimmedPayload.indexOf('\r');
	int firstLineBreak = -1;

	if (newlineIndex != -1)
	{
		firstLineBreak = newlineIndex;
	}

	if (carriageReturnIndex != -1 && (firstLineBreak == -1 || carriageReturnIndex < firstLineBreak))
	{
		firstLineBreak = carriageReturnIndex;
	}

	if (firstLineBreak != -1)
	{
		Logger::logWarn(F("Payload contains multiple rows; ignoring additional lines."));
		trimmedPayload = trimmedPayload.substring(0, firstLineBreak);
		trimmedPayload.trim();

		if (trimmedPayload.length() == 0)
		{
			return elements;
		}
	}

	int start = 0;

	while (start < trimmedPayload.length() && elements.size() < kMaxElements)
	{
		const int delimiterIndex = trimmedPayload.indexOf(',', start);
		String value;

		if (delimiterIndex == -1)
		{
			value = trimmedPayload.substring(start);
			start = trimmedPayload.length();
		}
		else
		{
			value = trimmedPayload.substring(start, delimiterIndex);
			start = delimiterIndex + 1;
		}

		value.trim();
		elements.push_back(value);
	}

	if (start < trimmedPayload.length())
	{
		Logger::logWarn(F("Payload exceeded maximum element count (5); remaining values ignored."));
	}

	return elements;
}
