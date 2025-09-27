/***************************************************************************
 * Project: Esp32NMCI
 * File: Logger.h
 * Repository: https://github.com/hesspet/NasrredinsMagicCardIdentifier
 * Author: Peter Heß, Büdingen DE
 * Description: Deklariert das Logging-Subsystem mit Anzeigeintegration, das
 *              formatierte Ausgaben verschiedener Schweregrade bereitstellt.
 ***************************************************************************/

#pragma once

#include <Arduino.h>
#include <type_traits>

#include "../Presentation/DisplayManager.h"

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
     * @param filterLevel Minimum severity level that will be emitted. Defaults to
     *        Logger::Level::Debug to allow full output.
     */
    static void begin(unsigned long baudRate = 115200, bool waitForSerial = true,
                      Level filterLevel = Level::Debug);

    static void setDisplayManager(DisplayManager *displayManager);

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
        if (!isLevelEnabled(level))
        {
            return;
        }
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
        if (!isLevelEnabled(level))
        {
            return;
        }
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
    static void LogWarn(const T &value, bool newline = true, int base = kNoBase,
                        bool showOnDisplay = false)
    {
        log(value, newline, base, Level::Warn);
        handleDisplayOutput(value, base, showOnDisplay);
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
    static void LogError(const T &value, bool newline = true, int base = kNoBase,
                         bool showOnDisplay = false)
    {
        log(value, newline, base, Level::Error);
        handleDisplayOutput(value, base, showOnDisplay);
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
    static void LogDebug(const T &value, bool newline = true, int base = kNoBase)
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
        if (!isLevelEnabled(level))
        {
            return;
        }
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

    /**
     * @brief Checks whether logging is enabled for a specific level.
     */
    static bool isLogLevel(Level level);

    /**
     * @brief Checks whether info level logging is enabled.
     */
    static bool isLogLevelInfo();

    /**
     * @brief Checks whether warning level logging is enabled.
     */
    static bool isLogLevelWarn();

    /**
     * @brief Checks whether debug level logging is enabled.
     */
    static bool isLogLevelDebug();

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
    static bool isLevelEnabled(Level level);
    static uint8_t levelPriority(Level level);

    template <typename T>
    static void handleDisplayOutput(const T &value, int base, bool showOnDisplay)
    {
        if (!showOnDisplay || s_displayManager == nullptr)
        {
            return;
        }

        s_displayManager->showError(makeDisplayString(value, base));
    }

    template <typename T>
    static typename std::enable_if<std::is_integral<typename std::decay<T>::type>::value, String>::type
    makeDisplayString(const T &value, int base)
    {
        if (base != kNoBase)
        {
            return String(value, static_cast<unsigned char>(base));
        }
        return String(value);
    }

    template <typename T>
    static typename std::enable_if<!std::is_integral<typename std::decay<T>::type>::value, String>::type
    makeDisplayString(const T &value, int base)
    {
        (void)base;
        return String(value);
    }

    static bool s_atLineStart;
    static Level s_filterLevel;
    static DisplayManager *s_displayManager;
};
