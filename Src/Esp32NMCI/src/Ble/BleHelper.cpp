/***************************************************************************
 * Project: Esp32NMCI
 * File: BleHelper.cpp
 * Repository: https://github.com/hesspet/NasrredinsMagicCardIdentifier
 * Author: Peter Heß, Büdingen DE
 * Description: Implements helper utilities for BLE keyboard interactions.
 ***************************************************************************/

#include "BleHelper.h"

#include "src/Ble/BleKeyboardCallback.h"
#include "src/Logger/Logger.h"

void BleHelper::SendTextToKeyboard(BleKeyboardCallback& keyboard, const String& text)
{
        if (text.length() == 0)
        {
                return;
        }

        if (!keyboard.isConnected())
        {
                Logger::LogWarn(F("BLE keyboard not connected; skipping payload transfer."));
                return;
        }

        keyboard.print(text);
}
