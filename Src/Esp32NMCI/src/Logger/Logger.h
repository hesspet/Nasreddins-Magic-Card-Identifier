#pragma once

#include <Arduino.h>

class Logger
{
    public:
    static constexpr int kNoBase = -1;

    static void begin(unsigned long baudRate = 115200, bool waitForSerial = true);

    static void log();

    template <typename T>
    static void log(const T &value, bool newline = true, int base = kNoBase)
    {
        if (base == kNoBase)
        {
            if (newline)
            {
                Serial.println(value);
            }
            else
            {
                Serial.print(value);
            }
        }
        else
        {
            if (newline)
            {
                Serial.println(value, base);
            }
            else
            {
                Serial.print(value, base);
            }
        }
    }

    template <typename... Args>
    static void logf(const char *format, Args... args)
    {
        Serial.printf(format, args...);
    }

    static void flush();
};
