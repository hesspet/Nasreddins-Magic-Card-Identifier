#pragma once

#include <Arduino.h>
#include <type_traits>

class Logger
{
public:
    enum class Level
    {
        Info,
        Warn,
        Error,
        Debug
    };

    static constexpr int kNoBase = -1;

    /**
     * @brief Initializes the serial interface used for logging.
     *
     * Example:
     * @code
     * void setup()
     * {
     *     Logger::begin(115200);
     * }
     * @endcode
     *
     * @param baudRate Desired baud rate for the serial port.
     * @param waitForSerial When true, blocks until the serial port becomes available.
     */
    static void begin(unsigned long baudRate = 115200, bool waitForSerial = true);

    /**
     * @brief Emits an empty log line, separating two messages.
     *
     * Example:
     * @code
     * Logger::log();
     * @endcode
     */
    static void log();

    /**
     * @brief Convenience wrapper for emitting an empty informational log line.
     */
    static void LogInfo();

    /**
     * @brief Logs arithmetic values with optional base, newline handling and severity level.
     *
     * Example:
     * @code
     * Logger::log(42, true, Logger::kNoBase, Logger::Level::Debug);
     * Logger::log(255, true, 16, Logger::Level::Info);
     * @endcode
     */
    template <typename T>
    static typename std::enable_if<std::is_arithmetic<typename std::decay<T>::type>::value>::type log(
        const T &value, bool newline = true, int base = kNoBase, Level level = Level::Info)
    {
        if (base == kNoBase)
        {
            printValue(value, newline, level);
        }
        else
        {
            printValue(value, newline, base, level);
        }
    }

    /**
     * @brief Logs non-arithmetic values with optional newline handling and severity level.
     *
     * Example:
     * @code
     * Logger::log(F("Tag detected"));
     * Logger::log(String("Raw data"), false, Logger::kNoBase, Logger::Level::Warn);
     * @endcode
     */
    template <typename T>
    static typename std::enable_if<!std::is_arithmetic<typename std::decay<T>::type>::value>::type log(
        const T &value, bool newline = true, int base = kNoBase, Level level = Level::Info)
    {
        (void)base;
        printValue(value, newline, level);
    }

    template <typename T>
    static void LogInfo(const T &value, bool newline = true, int base = kNoBase)
    {
        log(value, newline, base, Level::Info);
    }

    /**
     * @brief Logs a message as informational output.
     *
     * Example:
     * @code
     * Logger::logInfo(F("System ready"));
     * @endcode
     */
    template <typename T>
    static void logInfo(const T &value, bool newline = true, int base = kNoBase)
    {
        log(value, newline, base, Level::Info);
    }

    /**
     * @brief Logs a message indicating a potential problem.
     *
     * Example:
     * @code
     * Logger::logWarn(F("Battery level low"));
     * @endcode
     */
    template <typename T>
    static void logWarn(const T &value, bool newline = true, int base = kNoBase)
    {
        log(value, newline, base, Level::Warn);
    }

    /**
     * @brief Logs a message indicating an error condition.
     *
     * Example:
     * @code
     * Logger::logError(F("Sensor timeout"));
     * @endcode
     */
    template <typename T>
    static void logError(const T &value, bool newline = true, int base = kNoBase)
    {
        log(value, newline, base, Level::Error);
    }

    /**
     * @brief Logs a message useful for debugging purposes.
     *
     * Example:
     * @code
     * Logger::logDebug(String("Raw frame:"), false);
     * Logger::logDebug(0xAB, true, 16);
     * @endcode
     */
    template <typename T>
    static void logDebug(const T &value, bool newline = true, int base = kNoBase)
    {
        log(value, newline, base, Level::Debug);
    }

    /**
     * @brief Logs a formatted message with the specified severity level.
     *
     * Example:
     * @code
     * Logger::logf(Logger::Level::Debug, "Value: %u\n", value);
     * @endcode
     */
    template <typename... Args>
    static void logf(Level level, const char *format, Args... args)
    {
        if (s_atLineStart)
        {
            printPrefix(level);
        }
        Serial.printf(format, args...);
        s_atLineStart = formatEndsWithNewline(format);
    }

    /**
     * @brief Logs a formatted message using the info severity level.
     *
     * Example:
     * @code
     * Logger::logf("Progress: %u%%", progress);
     * @endcode
     */
    template <typename... Args>
    static void logf(const char *format, Args... args)
    {
        logf(Level::Info, format, args...);
    }

    /**
     * @brief Flushes the serial buffer, ensuring all log messages are transmitted.
     *
     * Example:
     * @code
     * Logger::flush();
     * @endcode
     */
    static void flush();

private:
    template <typename T>
    static void printValue(const T &value, bool newline, Level level)
    {
        if (s_atLineStart)
        {
            printPrefix(level);
        }

        if (newline)
        {
            Serial.println(value);
            s_atLineStart = true;
        }
        else
        {
            Serial.print(value);
            s_atLineStart = false;
        }
    }

    template <typename T>
    static void printValue(const T &value, bool newline, int base, Level level)
    {
        if (s_atLineStart)
        {
            printPrefix(level);
        }

        if (newline)
        {
            Serial.println(value, base);
            s_atLineStart = true;
        }
        else
        {
            Serial.print(value, base);
            s_atLineStart = false;
        }
    }

    static void printPrefix(Level level);
    static bool formatEndsWithNewline(const char *format);

    static bool s_atLineStart;
};
