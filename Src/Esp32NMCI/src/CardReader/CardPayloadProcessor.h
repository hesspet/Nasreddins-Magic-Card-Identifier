#pragma once

#include <Arduino.h>
#include <vector>

class CardPayloadProcessor
{
    public:
    CardPayloadProcessor() = default;

    std::vector<String> ProcessPayload(const String &payload);
};
