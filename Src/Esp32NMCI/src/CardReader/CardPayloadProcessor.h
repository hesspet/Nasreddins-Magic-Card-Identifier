#pragma once

#include <Arduino.h>

class CardPayloadProcessor
{
    public:
    CardPayloadProcessor();

    static void OnNewData(const String &payloadText);

    private:
    void ProcessPayload(const String &payload);
    void ProcessPayloadLine(const String &payload, int &lineStart, bool &firstLine, int &retFlag);

    static CardPayloadProcessor *instCardPayloadProcessor_;
};
