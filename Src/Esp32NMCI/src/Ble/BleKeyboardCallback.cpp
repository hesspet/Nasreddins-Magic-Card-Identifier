/***************************************************************************
 * Project: Esp32NMCI
 * File: BleKeyboardCallback.cpp
 * Repository: https://github.com/hesspet/NasrredinsMagicCardIdentifier
 * Author: Peter Heß, Büdingen DE
 * Description: Implements the logic that restarts BLE advertising after
 *              disconnects and adds diagnostic output.
 ***************************************************************************/

#include "BleKeyboardCallback.h"
#include <NimBLEAdvertising.h>

void BleKeyboardCallback::restartAdvertisingIfNecessary()
{
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
}
