#include "CardPayloadProcessor.h"

#include "../Logger/Logger.h"

String CardPayloadProcessor::ProcessPayload(const String &payload)
{
    int lineStart = 0;
    bool firstLine = true;
    bool appendNewline = false;
    String result;
    const bool endsWithNewline = payload.length() > 0 && payload.charAt(payload.length() - 1) == '\n';

    while (lineStart < payload.length())
    {
        int retFlag;
        const String lineResult = ProcessPayloadLine(payload, lineStart, firstLine, retFlag);

        if (appendNewline)
        {
            result += '\n';
        }

        result += lineResult;
        appendNewline = true;

        if (retFlag == 2) break;
    }

    if (endsWithNewline)
    {
        result += '\n';
    }

    return result;
}

String CardPayloadProcessor::ProcessPayloadLine(const String &payload, int &lineStart, bool &firstLine, int &retFlag)
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
    }
    else
    {
        lineStart = lineEnd + 1;
    }

    return line;
}
