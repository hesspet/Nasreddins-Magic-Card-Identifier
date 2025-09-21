#pragma once
#include <TFT_eSPI.h>
#include <SPI.h>
#include <stdint.h>

class DisplayManager
{
public:
    enum class Orientation : uint8_t
    {
        UsbRight = 0,
        UsbLeft = 180,
    };

    void begin(Orientation orientation = Orientation::UsbRight);
    void showMessage(const String& message, bool withOverlay = true);

private:
    TFT_eSPI tft = TFT_eSPI();
};

