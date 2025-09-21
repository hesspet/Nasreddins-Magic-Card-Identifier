#pragma once

#include <Arduino.h>
#include <type_traits>

class Logger
{
    public:
    static constexpr int kNoBase = -1;

    static void begin(unsigned long baudRate = 115200, bool waitForSerial = true);

    static void log();

    template <typename T>
    static typename std::enable_if<std::is_arithmetic<typename std::decay<T>::type>::value>::type log(
        const T &value, bool newline = true, int base = kNoBase)
    {
        if (base == kNoBase)
        {
            printValue(value, newline);
        }
        else
        {
            printValue(value, newline, base);
        }
    }

    template <typename T>
    static typename std::enable_if<!std::is_arithmetic<typename std::decay<T>::type>::value>::type log(
        const T &value, bool newline = true, int base = kNoBase)
    {
        (void)base;
        printValue(value, newline);
    }

    template <typename... Args>
    static void logf(const char *format, Args... args)
    {
        Serial.printf(format, args...);
    }

    static void flush();

private:
    template <typename T>
    static void printValue(const T &value, bool newline)
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

    template <typename T>
    static void printValue(const T &value, bool newline, int base)
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
};
