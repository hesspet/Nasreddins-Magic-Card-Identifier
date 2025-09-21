#pragma once
#include <TFT_eSPI.h>
#include <SPI.h>

class DisplayManager
{
public:
	void begin();
	void showMessage(const String& message, bool withOverlay = true);
private:
	TFT_eSPI tft = TFT_eSPI();
};

