#include <TFT_eSPI.h>
#include <SPI.h>
#include "DisplayManager.h"
#include "../Logger/Logger.h"

void DisplayManager::begin(Orientation orientation)
{
    tft.init();

    // Map the requested physical orientation to the library's rotation indices.
    uint8_t rotation = 1;
    switch (orientation)
    {
        case Orientation::UsbRight:
            rotation = 1;
            break;
        case Orientation::UsbLeft:
            rotation = 3;
            break;
        default:
            rotation = 1;
            break;
    }

    tft.setRotation(rotation);

#ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
#endif

    showMessage("Hello", false);
}

void DisplayManager::showMessage(const String& message, bool withOverlay)
{
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM); // Mitte zentriert
    tft.setTextColor(TFT_GREEN, TFT_BLACK);

    // Maximale Displaygröße
    int maxWidth = tft.width() - 10;   // etwas Rand lassen
    int maxHeight = tft.height() - 10;

    int bestSize = 1;
    for (int size = 1; size <= 8; ++size) // 1 bis 7 sind vernünftig
    {
        tft.setTextSize(size);
        int16_t w = tft.textWidth(message);
        int16_t h = 8 * size;  // Standardhöhe der Schrift bei Größe 1 = 8px

        if (w > maxWidth || h > maxHeight)
        {
            break;  // letzte gültige Größe war bestSize
        }
        bestSize = size;
    }

    tft.setTextSize(bestSize);
    tft.drawString(message, tft.width() / 2, tft.height() / 2);

    // drawStatusOverlay();  // Sprite neu zeichnen
}

void DisplayManager::showMessage(const std::vector<String>& messages)
{
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);

    if (messages.empty())
    {
        showMessage(String("no data in record 1"));
        return;
    }

    constexpr uint8_t maxLines = 3;
    const size_t lineCount = messages.size() < maxLines ? messages.size() : maxLines;

    const int16_t margin = 10;
    const int16_t availableWidth = tft.width() - (margin * 2);
    const int16_t availableHeight = tft.height() - (margin * 2);

    int bestSize = 1;
    for (int size = 1; size <= 8; ++size)
    {
        tft.setTextSize(size);
        const int16_t lineHeight = 8 * size;
        const int16_t totalHeight = lineHeight * maxLines;

        if (totalHeight > availableHeight)
        {
            break;
        }

        bool fits = true;
        for (size_t i = 0; i < lineCount; ++i)
        {
            if (tft.textWidth(messages[i]) > availableWidth)
            {
                fits = false;
                break;
            }
        }

        if (!fits)
        {
            break;
        }

        bestSize = size;
    }

    tft.setTextSize(bestSize);
    const int16_t lineHeight = 8 * bestSize;
    const int16_t totalVisibleHeight = lineHeight * static_cast<int16_t>(lineCount);
    const int16_t startY = (tft.height() - totalVisibleHeight) / 2 + (lineHeight / 2);

    for (size_t i = 0; i < lineCount; ++i)
    {
        tft.drawString(messages[i], tft.width() / 2, startY + (lineHeight * static_cast<int16_t>(i)));
    }
}
