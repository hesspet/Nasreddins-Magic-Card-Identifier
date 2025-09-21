#pragma once

#include <Arduino.h>

#include "NdefHelper.h"

class CardPayloadProcessor
{
    public:
    explicit CardPayloadProcessor(NdefHelper &ndefHelper);

    static void OnPayloadRead(const NdefHelper::NdefRecord &record);

    private:
    void ProcessPayload(const String &payload);
    void ProcessPayloadLine(const String &payload, int &lineStart, bool &firstLine, int &retFlag);

    static CardPayloadProcessor *instance_;
    NdefHelper &ndefHelper_;
};
