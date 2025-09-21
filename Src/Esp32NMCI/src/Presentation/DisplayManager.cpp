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

void DisplayManager::showMessage(const String& message, bool withOverlay) {
    
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);  // Mitte zentriert
    tft.setTextColor(TFT_GREEN, TFT_BLACK);

    // Maximale Displaygröße
    int maxWidth = tft.width() - 10;   // etwas Rand lassen
    int maxHeight = tft.height() - 10;

    int bestSize = 1;
    for (int size = 1; size <= 8; ++size) {  // 1 bis 7 sind vernünftig
        tft.setTextSize(size);
        int16_t w = tft.textWidth(message);
        int16_t h = 8 * size;  // Standardhöhe der Schrift bei Größe 1 = 8px

        if (w > maxWidth || h > maxHeight) {
            break;  // letzte gültige Größe war bestSize
        }
        bestSize = size;
    }

    tft.setTextSize(bestSize);
    tft.drawString(message, tft.width() / 2, tft.height() / 2);

    // drawStatusOverlay();  // Sprite neu zeichnen
}
