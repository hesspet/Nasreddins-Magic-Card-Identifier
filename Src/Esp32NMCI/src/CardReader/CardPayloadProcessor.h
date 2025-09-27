/***************************************************************************
 * Project: Esp32NMCI
 * File: CardPayloadProcessor.h
 * Repository: https://github.com/hesspet/NasrredinsMagicCardIdentifier
 * Author: Peter Heß, Büdingen DE
 * Description: Declares the helper class that splits card payload text into
 *              curated list entries for display and BLE transfer.
 ***************************************************************************/

#pragma once

#include <Arduino.h>
#include <vector>

class CardPayloadProcessor
{
public:
	CardPayloadProcessor() = default;

	std::vector<String> ProcessPayload(const String& payload);
};
