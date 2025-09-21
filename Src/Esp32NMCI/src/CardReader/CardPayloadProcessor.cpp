#include "CardPayloadProcessor.h"

#include "../Logger/Logger.h"

CardPayloadProcessor *CardPayloadProcessor::instance_ = nullptr;

CardPayloadProcessor::CardPayloadProcessor(NdefHelper &ndefHelper)
    : ndefHelper_(ndefHelper)
{
    instance_ = this;
}

void CardPayloadProcessor::OnPayloadRead(const NdefHelper::NdefRecord &record)
{
    Logger::LogInfo(F("OnPayloadRead - payload length: "), false);
    Logger::LogInfo(static_cast<unsigned long>(record.payloadLen));

    if (!instance_)
    {
        Logger::LogError(F("CardPayloadProcessor instance not initialized."));
        return;
    }

    if (!record.payload || record.payloadLen == 0)
    {
        Logger::LogWarn(F("Received record with empty payload."));
        return;
    }

    if (instance_->ndefHelper_.isTextRecord(record))
    {
        instance_->ndefHelper_.decodeAndPrintTextRecord(record);
    }
    else
    {
        Logger::LogWarn(F("First NDEF record is not a Text (RTD/T) record."));
        Logger::LogDebug(F("Payload (hex):"));

        const String payload = instance_->ndefHelper_.getString(record.payload, record.payloadLen);

        instance_->ProcessPayload(payload);
    }
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
