#pragma once
#include <TFT_eSPI.h>
#include <SPI.h>
#include <stdint.h>
#include <vector>

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
    void showMessage(const std::vector<String>& messages);
    void update();

private:
    TFT_eSPI tft = TFT_eSPI();
    void clearScreen();
    void renderSingleMessage(const String& message, uint16_t textColor);
    void renderMultipleMessages(const std::vector<String>& messages, uint16_t textColor);
    void cacheMessageToRemember(const std::vector<String>& messages, bool isList);

    std::vector<String> lastDisplayedLines;
    bool lastMessageWasList = false;
    bool lastMessageValid = false;
    bool lastMessageCurrentlyGreen = false;
    uint32_t lastMessageTimestamp = 0;
};

