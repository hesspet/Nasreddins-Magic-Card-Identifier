#pragma once

#include <Arduino.h>

class CardPayloadProcessor
{
    public:
    CardPayloadProcessor() = default;

    String ProcessPayload(const String &payload);

    private:
    String ProcessPayloadLine(const String &payload, int &lineStart, bool &firstLine, int &retFlag);
};
