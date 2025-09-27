/***************************************************************************
 * Project: Esp32NMCI
 * File: ButtonManager.h
 * Repository: https://github.com/hesspet/NasrredinsMagicCardIdentifier
 * Author: Peter Heß, Büdingen DE
 * Description: Declares management of the physical buttons including
 *              debouncing, state tracking, and callback registration.
 ***************************************************************************/

#pragma once
// ButtonManager.h
#include <Arduino.h>
#include <functional>

class ButtonManager
{
    public:
    ButtonManager(int pinA, int pinB);

    void begin();
    void update();

    bool isButtonAPressed() const;
    bool isButtonBPressed() const;
    bool areBothButtonsPressed() const;

    void setOnButton0Pressed(std::function<void()> callback);
    void setOnButton35Pressed(std::function<void()> callback);
    void setOnButton35LongPressed(std::function<void()> callback);

    private:
    int pinA;
    int pinB;
    bool stateA = false;
    bool stateB = false;
    bool lastStateA = false;
    bool lastStateB = false;

    std::function<void()> onButton0Pressed = nullptr;
    std::function<void()> onButton35Pressed = nullptr;
    std::function<void()> onButton35LongPressed = nullptr;

    unsigned long pressStart35 = 0;
    const unsigned long longPressThreshold = 7000; // ms
};
