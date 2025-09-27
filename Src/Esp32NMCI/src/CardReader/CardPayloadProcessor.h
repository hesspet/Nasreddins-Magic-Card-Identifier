/***************************************************************************
 * Project: Esp32NMCI
 * File: CardPayloadProcessor.h
 * Repository: https://github.com/hesspet/NasrredinsMagicCardIdentifier
 * Author: Peter Heß, Büdingen DE
 * Description: Deklaration der Hilfsklasse, die Kartennutzdaten in aufbereitete
 *              Listenelemente für Anzeige und BLE-Transfer zerlegt.
 ***************************************************************************/

#pragma once

#include <Arduino.h>
#include <vector>

class CardPayloadProcessor
{
    public:
    CardPayloadProcessor() = default;

    std::vector<String> ProcessPayload(const String &payload);
};
