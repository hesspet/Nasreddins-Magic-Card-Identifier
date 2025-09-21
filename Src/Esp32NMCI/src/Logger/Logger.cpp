#include "Logger.h"

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
}

void Logger::log()
{
    Serial.println();
}

void Logger::flush()
{
    Serial.flush();
}
