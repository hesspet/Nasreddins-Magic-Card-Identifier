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
		Logger::LogWarn(F("[BLE] No advertising object available; attempting restart."));
		if (BLEDevice::startAdvertising())
		{
			Logger::LogInfo(F("[BLE] Advertising started successfully using fallback."));
		}
		else
		{
			Logger::LogError(F("[BLE] Failed to start advertising using fallback."));
		}
		return;
	}

	if (advertising->isAdvertising())
	{
		Logger::LogDebug(F("[BLE] Advertising already running; no restart required."));
		return;
	}

	if (advertising->start())
	{
		Logger::LogInfo(F("[BLE] Advertising restarted after disconnection."));
	}
	else
	{
		Logger::LogError(F("[BLE] Failed to restart advertising after disconnection."));
	}
}
