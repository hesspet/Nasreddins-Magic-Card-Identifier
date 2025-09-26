#include "LoggingBleKeyboard.h"

#if defined(USE_NIMBLE)
#include <NimBLEAdvertising.h>
#endif

void LoggingBleKeyboard::restartAdvertisingIfNecessary()
{
#if defined(USE_NIMBLE)
    NimBLEAdvertising* advertising = BLEDevice::getAdvertising();

    if (advertising == nullptr)
    {
        Logger::LogWarn(F("[BLE] Kein Advertising-Objekt verfügbar; versuche Neustart."));
        if (BLEDevice::startAdvertising())
        {
            Logger::LogInfo(F("[BLE] Advertising im Fallback erfolgreich gestartet."));
        }
        else
        {
            Logger::LogError(F("[BLE] Advertising konnte im Fallback nicht gestartet werden."));
        }
        return;
    }

    if (advertising->isAdvertising())
    {
        Logger::LogDebug(F("[BLE] Advertising läuft bereits; kein Neustart erforderlich."));
        return;
    }

    if (advertising->start())
    {
        Logger::LogInfo(F("[BLE] Advertising nach Trennung neu gestartet."));
    }
    else
    {
        Logger::LogError(F("[BLE] Advertising konnte nach Trennung nicht gestartet werden."));
    }
#else
    // Klassischer BLE-Stack: das Basismodul übernimmt den Neustart des Advertisings.
#endif
}
