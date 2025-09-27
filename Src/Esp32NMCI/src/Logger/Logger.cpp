/***************************************************************************
 * Project: Esp32NMCI
 * File: Logger.cpp
 * Repository: https://github.com/hesspet/NasrredinsMagicCardIdentifier
 * Author: Peter Heß, Büdingen DE
 * Description: Implementiert die Ausgabe- und Formatierungsfunktionen des
 *              Loggers inklusive Serial-Setup und Displaybenachrichtigung.
 ***************************************************************************/

#include "Logger.h"

#include <cstring>

bool Logger::s_atLineStart = true;
Logger::Level Logger::s_filterLevel = Logger::Level::Debug;
DisplayManager *Logger::s_displayManager = nullptr;

void Logger::begin(unsigned long baudRate, bool waitForSerial, Level filterLevel)
{
    Serial.begin(baudRate);
    if (waitForSerial)
    {
        while (!Serial)
        {
            delay(100);
        }
    }
    s_filterLevel = filterLevel;
    s_atLineStart = true;
}

void Logger::setDisplayManager(DisplayManager *displayManager)
{
    s_displayManager = displayManager;
}

void Logger::log()
{
    if (!isLevelEnabled(Level::Info))
    {
        return;
    }
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

bool Logger::isLogLevel(Level level)
{
    return isLevelEnabled(level);
}

bool Logger::isLogLevelInfo()
{
    return isLogLevel(Level::Info);
}

bool Logger::isLogLevelWarn()
{
    return isLogLevel(Level::Warn);
}

bool Logger::isLogLevelDebug()
{
    return isLogLevel(Level::Debug);
}

bool Logger::isLevelEnabled(Level level)
{
    return levelPriority(level) >= levelPriority(s_filterLevel);
}

uint8_t Logger::levelPriority(Level level)
{
    switch (level)
    {
    case Level::Error:
        return 4;
    case Level::Warn:
        return 3;
    case Level::Info:
        return 2;
    case Level::Debug:
    default:
        return 1;
    }
}
