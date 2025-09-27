/***************************************************************************
 * Project: Esp32NMCI
 * File: BleHelper.h
 * Repository: https://github.com/hesspet/NasrredinsMagicCardIdentifier
 * Author: Peter Heß, Büdingen DE
 * Description: Declares helper utilities for BLE keyboard interactions.
 ***************************************************************************/

#pragma once

#include <Arduino.h>

class BleKeyboardCallback;

/**
 * @brief Helper functions related to BLE communication.
 */
class BleHelper
{
    public:
    /**
     * @brief Sends text via the provided BLE keyboard if a connection is established.
     *
     * @param keyboard BLE keyboard instance used to transmit the text.
     * @param text UTF-8 encoded content to transmit.
     */
    static void SendTextToKeyboard(BleKeyboardCallback& keyboard, const String& text);
};
