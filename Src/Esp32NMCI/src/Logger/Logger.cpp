#include "Logger.h"

#include <cstring>

bool Logger::s_atLineStart = true;

void Logger::begin(unsigned long baudRate, bool waitForSerial)
{
    Serial.begin(baudRate);
    if (waitForSerial)
    {
        while (!Serial)
        {
            delay(100);
        }
    }
    s_atLineStart = true;
}

void Logger::log()
{
    Serial.println();
    s_atLineStart = true;
}

void Logger::LogInfo()
{
    log();
}

void Logger::flush()
{
    Serial.flush();
}

void Logger::printPrefix(Level level)
{
    switch (level)
    {
    case Level::Info:
        Serial.print(F("[INF] "));
        break;
    case Level::Warn:
        Serial.print(F("[WRN] "));
        break;
    case Level::Error:
        Serial.print(F("[ERR] "));
        break;
    case Level::Debug:
    default:
        Serial.print(F("[DBG] "));
        break;
    }
    s_atLineStart = false;
}

bool Logger::formatEndsWithNewline(const char *format)
{
    if (format == nullptr)
    {
        return false;
    }

    const char *lastNewline = std::strrchr(format, '\n');
    return lastNewline != nullptr && lastNewline[1] == '\0';
}
