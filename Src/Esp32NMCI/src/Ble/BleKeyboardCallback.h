#pragma once

#include <NimBLEDevice.h> // 2.3.6
#include <BleKeyboard.h> // Hinweis: Manuell gepatched für NimBLE 2.3.6!
#include "../Presentation/DisplayManager.h"
#include "../Logger/Logger.h"


/**
 * @brief Spezialisierte Tastaturemulation mit zusätzlichen Logmeldungen.
 */
class BleKeyboardCallback final : public BleKeyboard
{
public:
    using BleKeyboard::BleKeyboard;

    void setDisplayManager(DisplayManager* manager)
    {
        displayManager = manager;

        if (displayManager != nullptr)
        {
            displayManager->setBleConnectionState(isConnected());
        }
    }

protected:
#if defined(USE_NIMBLE)
    void onConnect(BLEServer* server, NimBLEConnInfo& connInfo) override
    {
        BleKeyboard::onConnect(server, connInfo);
        Logger::LogInfo(F("[BLE] Verbindung hergestellt."));
        if (displayManager != nullptr)
        {
            displayManager->setBleConnectionState(true);
        }
    }

    void onDisconnect(BLEServer* server, NimBLEConnInfo& connInfo, int reason) override
    {
        BleKeyboard::onDisconnect(server, connInfo, reason);
        Logger::logf(Logger::Level::Info,
            "[BLE] Verbindung getrennt (Grund: %d).\n",
            reason);
        if (displayManager != nullptr)
        {
            displayManager->setBleConnectionState(false);
        }
        restartAdvertisingIfNecessary();
    }
#else
    void onConnect(BLEServer* server) override
    {
        BleKeyboard::onConnect(server);
        Logger::LogInfo(F("[BLE] Verbindung hergestellt."));
        if (displayManager != nullptr)
        {
            displayManager->setBleConnectionState(true);
        }
    }

    void onDisconnect(BLEServer* server) override
    {
        BleKeyboard::onDisconnect(server);
        Logger::LogInfo(F("[BLE] Verbindung getrennt."));
        if (displayManager != nullptr)
        {
            displayManager->setBleConnectionState(false);
        }
        restartAdvertisingIfNecessary();
    }
#endif

private:
    void restartAdvertisingIfNecessary();
    DisplayManager* displayManager = nullptr;
};
