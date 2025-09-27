/***************************************************************************
 * Project: Esp32NMCI
 * File: SystemHelper.cpp
 * Repository: https://github.com/hesspet/NasrredinsMagicCardIdentifier
 * Author: Peter Heß, Büdingen DE
 * Description: Implements helper utilities for interacting with ESP32 system
 *              features such as deep sleep handling.
 ***************************************************************************/

#include "SystemHelper.h"

#include <esp_sleep.h>

#include "../Logger/Logger.h"

void SystemHelper::EnterDeepSleep()
{
        Logger::LogInfo(F("[System] Entering deep sleep"));
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 0);
        esp_deep_sleep_start();
}
