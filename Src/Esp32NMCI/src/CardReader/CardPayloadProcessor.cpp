#include "CardPayloadProcessor.h"

#include "../Logger/Logger.h"

CardPayloadProcessor *CardPayloadProcessor::instCardPayloadProcessor_ = nullptr;

CardPayloadProcessor::CardPayloadProcessor(NdefHelper &ndefHelper)
    : ndefHelper_(ndefHelper)
{
    instCardPayloadProcessor_ = this;
}

void CardPayloadProcessor::ProcessPayload(const String &payload)
{
    int lineStart = 0;
    bool firstLine = true;

    while (lineStart < payload.length())
    {
        int retFlag;
        ProcessPayloadLine(payload, lineStart, firstLine, retFlag);
        if (retFlag == 2) break;
    }
}

void CardPayloadProcessor::ProcessPayloadLine(const String &payload, int &lineStart, bool &firstLine, int &retFlag)
{
    retFlag = 1;
    int lineEnd = payload.indexOf('\n', lineStart);
    String line = (lineEnd == -1) ? payload.substring(lineStart)
                                  : payload.substring(lineStart, lineEnd);

    if (line.length() > 0)
    {
        if (firstLine)
        {
            Logger::LogDebug(line);
        }
        else
        {
            Logger::LogInfo(line);
        }
    }
    else
    {
        Logger::LogInfo();
    }

    firstLine = false;

    if (lineEnd == -1)
    {
        retFlag = 2;
        return;
    }

    lineStart = lineEnd + 1;
}
