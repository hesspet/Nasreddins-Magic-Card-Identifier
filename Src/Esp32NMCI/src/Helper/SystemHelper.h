/***************************************************************************
 * Project: Esp32NMCI
 * File: SystemHelper.h
 * Repository: https://github.com/hesspet/NasrredinsMagicCardIdentifier
 * Author: Peter Heß, Büdingen DE
 * Description: Declares helper utilities for interacting with ESP32 system
 *              features such as deep sleep handling.
 ***************************************************************************/

#pragma once

/**
 * @brief Utility helpers for system level features of the ESP32.
 */
class SystemHelper
{
    public:
    /**
     * @brief Puts the ESP32 into deep sleep and enables wake-up via GPIO 35.
     */
    static void EnterDeepSleep();
};
