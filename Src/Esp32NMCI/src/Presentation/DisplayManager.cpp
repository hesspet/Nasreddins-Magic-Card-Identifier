/***************************************************************************
 * Project: Esp32NMCI
 * File: DisplayManager.cpp
 * Repository: https://github.com/hesspet/NasrredinsMagicCardIdentifier
 * Author: Peter Heß, Büdingen DE
 ***************************************************************************/

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <algorithm>
#include "DisplayManager.h"
#include "../Logger/Logger.h"
#include "../config.h"

namespace
{
constexpr int16_t kTextMargin = 10;
constexpr int16_t kOverlayEdgeMargin = 4;
constexpr int16_t kOverlayIconSpacing = 4;
constexpr int16_t kOverlayTextSpacing = 6;
constexpr int16_t kBleIndicatorSize = 10;
}

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

        overlays.clear();
        lastRenderedTopOverlayAreaHeight = 0;
        lastRenderedBottomOverlayAreaHeight = 0;
        bleOverlayIndex = registerOverlay(OverlayPlacement::Top,
                                          kBleIndicatorSize,
                                          kBleIndicatorSize,
                                          [](TFT_eSPI& tftDisplay, int16_t x, int16_t y) {
                                                  tftDisplay.fillRect(x, y, kBleIndicatorSize, kBleIndicatorSize, TFT_BLUE);
                                          });
        bleConnectionActive = false;

        showMessage(VERSION_TEXT, false);
}

void DisplayManager::renderSingleMessage(const String& message, uint16_t textColor)
{
        clearScreen();

        tft.setTextDatum(MC_DATUM); // Mitte zentriert
        tft.setTextColor(textColor, currentBackgroundColor);

        const uint16_t topArea = calculateOverlayAreaHeight(OverlayPlacement::Top);
        const uint16_t bottomArea = calculateOverlayAreaHeight(OverlayPlacement::Bottom);
        const int16_t textAreaHeight = tft.height() - static_cast<int16_t>(topArea + bottomArea);
        if (textAreaHeight <= 0)
        {
                renderOverlays();
                return;
        }

        const int16_t textAreaTop = static_cast<int16_t>(topArea);
        const int16_t textAreaCenterY = textAreaTop + (textAreaHeight / 2);

        // Maximale Displaygröße
        int maxWidth = tft.width() - kTextMargin;   // etwas Rand lassen
        int maxHeight = textAreaHeight - kTextMargin;

        if (maxHeight < 8)
        {
                maxHeight = 8;
        }

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
        tft.drawString(message, tft.width() / 2, textAreaCenterY);

        renderOverlays();
}

void DisplayManager::showError(const String& message, bool withOverlay)
{
        (void)withOverlay;

        currentBackgroundColor = TFT_RED;
        tft.fillScreen(currentBackgroundColor);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_YELLOW, currentBackgroundColor);

        constexpr uint8_t maxLines = 4;
        constexpr size_t maxCharactersPerLine = 12;

        String flattenedMessage;
        flattenedMessage.reserve(message.length());

        const size_t originalLength = message.length();
        for (size_t index = 0; index < originalLength; ++index)
        {
                const char currentChar = message.charAt(static_cast<unsigned int>(index));

                if (currentChar == '\r' || currentChar == '\n' || currentChar == '\t')
                {
                        flattenedMessage += ' ';
                }
                else
                {
                        flattenedMessage += currentChar;
                }
        }

        if (flattenedMessage.length() == 0U)
        {
                lastDisplayedLines.clear();
                lastMessageWasList = false;
                lastMessageValid = false;
                lastMessageCurrentlyGreen = false;
                renderOverlays();
                return;
        }

        std::vector<String> lines;
        lines.reserve(maxLines);

        size_t startIndex = 0;
        const size_t flattenedLength = flattenedMessage.length();
        while (lines.size() < maxLines && startIndex < flattenedLength)
        {
                size_t endIndex = startIndex + maxCharactersPerLine;
                if (endIndex > flattenedLength)
                {
                        endIndex = flattenedLength;
                }

                lines.emplace_back(flattenedMessage.substring(static_cast<unsigned int>(startIndex),
                                                              static_cast<unsigned int>(endIndex)));
                startIndex = endIndex;
        }

        const uint16_t topArea = calculateOverlayAreaHeight(OverlayPlacement::Top);
        const uint16_t bottomArea = calculateOverlayAreaHeight(OverlayPlacement::Bottom);
        const int16_t textAreaHeight = tft.height() - static_cast<int16_t>(topArea + bottomArea);

        if (textAreaHeight <= 0)
        {
                renderOverlays();
                return;
        }

        const int16_t margin = 10;
        const int16_t availableWidth = tft.width() - (margin * 2);
        int16_t availableHeight = textAreaHeight - (margin * 2);
        if (availableHeight < 8)
        {
                availableHeight = 8;
        }

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
                for (const auto& line : lines)
                {
                        if (tft.textWidth(line) > availableWidth)
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
        const int16_t totalVisibleHeight = lineHeight * maxLines;
        const int16_t textAreaTop = static_cast<int16_t>(topArea);
        const int16_t startY = textAreaTop + ((textAreaHeight - totalVisibleHeight) / 2);

        for (size_t i = 0; i < lines.size(); ++i)
        {
                tft.drawString(lines[i], margin, startY + (lineHeight * static_cast<int16_t>(i)));
        }

        lastDisplayedLines.clear();
        lastMessageWasList = false;
        lastMessageValid = false;
        lastMessageCurrentlyGreen = false;

        renderOverlays();
}

void DisplayManager::showMessage(const String& message, bool withOverlay)
{
        renderSingleMessage(message, TFT_GREEN);

	std::vector<String> displayedLines;
	displayedLines.push_back(message);
	cacheMessageToRemember(displayedLines, false);
}

void DisplayManager::clearScreen() {

        currentBackgroundColor = TFT_BLACK;
        tft.fillScreen(currentBackgroundColor);
}

void DisplayManager::renderMultipleMessages(const std::vector<String>& messages, uint16_t textColor)
{
        clearScreen();

        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(textColor, currentBackgroundColor);

        if (messages.empty())
        {
                renderOverlays();
                return;
        }

        constexpr uint8_t maxLines = 3;
        constexpr size_t maxCharactersPerLine = 12;
	const size_t lineCount = messages.size() < maxLines ? messages.size() : maxLines;

	std::vector<String> truncatedMessages;
	truncatedMessages.reserve(lineCount);
	for (size_t i = 0; i < lineCount; ++i)
	{
		const String& message = messages[i];
		if (message.length() > maxCharactersPerLine)
		{
			truncatedMessages.emplace_back(message.substring(0, maxCharactersPerLine));
		}
		else
		{
			truncatedMessages.emplace_back(message);
		}
        }

        const uint16_t topArea = calculateOverlayAreaHeight(OverlayPlacement::Top);
        const uint16_t bottomArea = calculateOverlayAreaHeight(OverlayPlacement::Bottom);
        const int16_t textAreaHeight = tft.height() - static_cast<int16_t>(topArea + bottomArea);

        if (textAreaHeight <= 0)
        {
                renderOverlays();
                return;
        }

        const int16_t margin = 10;
        const int16_t availableWidth = tft.width() - (margin * 2);
        int16_t availableHeight = textAreaHeight - (margin * 2);
        if (availableHeight < 8)
        {
                availableHeight = 8;
        }

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
			if (tft.textWidth(truncatedMessages[i]) > availableWidth)
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
        const int16_t textAreaTop = static_cast<int16_t>(topArea);
        const int16_t startY = textAreaTop + ((textAreaHeight - totalVisibleHeight) / 2) + (lineHeight / 2);

        for (size_t i = 0; i < lineCount; ++i)
        {
                tft.drawString(truncatedMessages[i], tft.width() / 2, startY + (lineHeight * static_cast<int16_t>(i)));
        }

        renderOverlays();
}

void DisplayManager::showMessage(const std::vector<String>& messages)
{
	if (messages.empty())
	{
		showMessage(String("no data in record 1"));
		return;
	}

	constexpr uint8_t maxLines = 3;
	const size_t lineCount = messages.size() < maxLines ? messages.size() : maxLines;

	std::vector<String> displayedLines(messages.begin(), messages.begin() + lineCount);
	renderMultipleMessages(displayedLines, TFT_GREEN);
	cacheMessageToRemember(displayedLines, true);
}

void DisplayManager::cacheMessageToRemember(const std::vector<String>& messages, bool isList)
{
        lastDisplayedLines = messages;
        lastMessageWasList = isList;
        lastMessageValid = !messages.empty();
        lastMessageCurrentlyGreen = lastMessageValid;
        lastMessageTimestamp = millis();
}

void DisplayManager::redrawCachedMessage()
{
        if (!lastMessageValid)
        {
                renderOverlays();
                return;
        }

        const uint16_t textColor = lastMessageCurrentlyGreen ? TFT_GREEN : TFT_DARKGREY;

        if (lastMessageWasList)
        {
                renderMultipleMessages(lastDisplayedLines, textColor);
        }
        else if (!lastDisplayedLines.empty())
        {
                renderSingleMessage(lastDisplayedLines.front(), textColor);
        }
}

/*
        Prüft ob die Zeit abgelaufen ist um das Display auszugrauen.

        @see kDisplayManager_TimeToFadeOutDisplay Konfiguration des Delay bis zum Ausgrauen
*/
void DisplayManager::update()
{
	if (!lastMessageValid || !lastMessageCurrentlyGreen)
	{
		return;
	}

	const uint32_t now = millis();
	if ((now - lastMessageTimestamp) < kDisplayManager_TimeToFadeOutDisplay)
	{
		return;
	}

	if (lastMessageWasList)
	{
		renderMultipleMessages(lastDisplayedLines, TFT_DARKGREY);
	}
	else if (!lastDisplayedLines.empty())
	{
		renderSingleMessage(lastDisplayedLines.front(), TFT_DARKGREY);
	}

        lastMessageCurrentlyGreen = false;
}

size_t DisplayManager::registerOverlay(OverlayPlacement placement, uint16_t width, uint16_t height, OverlayDrawFunction drawFunction)
{
        overlays.push_back({placement, width, height, false, drawFunction});
        return overlays.size() - 1U;
}

void DisplayManager::setOverlayVisibility(size_t index, bool visible)
{
        if (index >= overlays.size())
        {
                return;
        }

        OverlayItem& overlay = overlays[index];
        if (overlay.visible == visible)
        {
                return;
        }

        overlay.visible = visible;
        redrawCachedMessage();
}

uint16_t DisplayManager::getMaxOverlayHeight(OverlayPlacement placement) const
{
        uint16_t maxHeight = 0;
        for (const auto& overlay : overlays)
        {
                if (overlay.placement == placement && overlay.visible)
                {
                        maxHeight = std::max<uint16_t>(maxHeight, overlay.height);
                }
        }
        return maxHeight;
}

uint16_t DisplayManager::calculateOverlayAreaHeight(OverlayPlacement placement) const
{
        const uint16_t maxHeight = getMaxOverlayHeight(placement);
        if (maxHeight == 0)
        {
                return 0;
        }

        return static_cast<uint16_t>(kOverlayEdgeMargin + maxHeight + kOverlayTextSpacing);
}

void DisplayManager::renderOverlayRow(OverlayPlacement placement)
{
        uint16_t& lastHeight = (placement == OverlayPlacement::Top) ? lastRenderedTopOverlayAreaHeight : lastRenderedBottomOverlayAreaHeight;
        const uint16_t areaHeight = calculateOverlayAreaHeight(placement);
        const uint16_t clearHeight = std::max(areaHeight, lastHeight);

        if (clearHeight > 0)
        {
                const int16_t clearY = (placement == OverlayPlacement::Top) ? 0 : tft.height() - static_cast<int16_t>(clearHeight);
                tft.fillRect(0, clearY, tft.width(), clearHeight, currentBackgroundColor);
        }

        if (areaHeight == 0)
        {
                lastHeight = 0;
                return;
        }

        int16_t x = kOverlayEdgeMargin;

        if (placement == OverlayPlacement::Top)
        {
                const int16_t y = kOverlayEdgeMargin;
                for (const auto& overlay : overlays)
                {
                        if (overlay.placement != OverlayPlacement::Top || !overlay.visible || overlay.drawFunction == nullptr)
                        {
                                continue;
                        }

                        overlay.drawFunction(tft, x, y);
                        x += static_cast<int16_t>(overlay.width) + kOverlayIconSpacing;
                }
        }
        else
        {
                for (const auto& overlay : overlays)
                {
                        if (overlay.placement != OverlayPlacement::Bottom || !overlay.visible || overlay.drawFunction == nullptr)
                        {
                                continue;
                        }

                        const int16_t y = tft.height() - kOverlayEdgeMargin - static_cast<int16_t>(overlay.height);
                        overlay.drawFunction(tft, x, y);
                        x += static_cast<int16_t>(overlay.width) + kOverlayIconSpacing;
                }
        }

        lastHeight = areaHeight;
}

void DisplayManager::renderOverlays()
{
        renderOverlayRow(OverlayPlacement::Top);
        renderOverlayRow(OverlayPlacement::Bottom);
}

void DisplayManager::setBleConnectionState(bool connected)
{
        if (bleConnectionActive == connected)
        {
                return;
        }

        bleConnectionActive = connected;

        if (bleOverlayIndex < overlays.size())
        {
                setOverlayVisibility(bleOverlayIndex, connected);
        }
}
