/***************************************************************************
 * Project: Esp32NMCI
 * File: DisplayManager.h
 * Repository: https://github.com/hesspet/NasrredinsMagicCardIdentifier
 * Author: Peter Heß, Büdingen DE
 * Description: Declares the rendering control for the T-Display including
 *              overlay management, BLE status indicators, and message caching.
 ***************************************************************************/

#pragma once
#include <TFT_eSPI.h>
#include <SPI.h>
#include <stdint.h>
#include <limits>
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
	void showError(const String& message, bool withOverlay = true);
	void update();
	void setBleConnectionState(bool connected);

private:
	TFT_eSPI tft = TFT_eSPI();
	void clearScreen();
	void renderSingleMessage(const String& message, uint16_t textColor);
	void renderMultipleMessages(const std::vector<String>& messages, uint16_t textColor);
	void cacheMessageToRemember(const std::vector<String>& messages, bool isList);
	void redrawCachedMessage();
	void renderOverlays();

	enum class OverlayPlacement : uint8_t
	{
		Top,
		Bottom,
	};

	using OverlayDrawFunction = void (*)(TFT_eSPI&, int16_t, int16_t);

	struct OverlayItem
	{
		OverlayPlacement placement;
		uint16_t width;
		uint16_t height;
		bool visible;
		OverlayDrawFunction drawFunction;
	};

	size_t registerOverlay(OverlayPlacement placement, uint16_t width, uint16_t height, OverlayDrawFunction drawFunction);
	void setOverlayVisibility(size_t index, bool visible);
	uint16_t getMaxOverlayHeight(OverlayPlacement placement) const;
	uint16_t calculateOverlayAreaHeight(OverlayPlacement placement) const;
	void renderOverlayRow(OverlayPlacement placement);

	std::vector<String> lastDisplayedLines;
	bool lastMessageWasList = false;
	bool lastMessageValid = false;
	bool lastMessageCurrentlyGreen = false;
	uint32_t lastMessageTimestamp = 0;
	std::vector<OverlayItem> overlays;
	size_t bleOverlayIndex = std::numeric_limits<size_t>::max();
	bool bleConnectionActive = false;
	uint16_t lastRenderedTopOverlayAreaHeight = 0;
	uint16_t lastRenderedBottomOverlayAreaHeight = 0;
	uint16_t currentBackgroundColor = TFT_BLACK;
};

