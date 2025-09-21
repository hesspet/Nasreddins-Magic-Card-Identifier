#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "DisplayManager.h"
#include "../Logger/Logger.h"
#include "../config.h"

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

	showMessage("NMCI V1", false);
}

void DisplayManager::renderSingleMessage(const String& message, uint16_t textColor)
{
	clearScreen();

	tft.setTextDatum(MC_DATUM); // Mitte zentriert
	tft.setTextColor(textColor, TFT_BLACK);

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

void DisplayManager::showError(const String& message, bool withOverlay)
{
        (void)withOverlay;

        tft.fillScreen(TFT_RED);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_YELLOW, TFT_RED);

        constexpr uint8_t maxLines = 3;
        constexpr size_t maxCharactersPerLine = 12;

        std::vector<String> lines;
        lines.reserve(maxLines);

        size_t start = 0;
        const size_t length = message.length();

        while (lines.size() < maxLines && start < length)
        {
                while (start < length)
                {
                        const char currentChar = message.charAt(start);
                        if (currentChar == ' ' || currentChar == '\n' || currentChar == '\r' || currentChar == '\t')
                        {
                                ++start;
                                continue;
                        }
                        break;
                }

                if (start >= length)
                {
                        break;
                }

                const size_t remaining = length - start;
                size_t endExclusive = start + (remaining < maxCharactersPerLine ? remaining : maxCharactersPerLine);

                const int newlineIndex = message.indexOf('\n', static_cast<unsigned int>(start));
                int explicitBreakIndex = newlineIndex;

                const int carriageReturnIndex = message.indexOf('\r', static_cast<unsigned int>(start));
                if (carriageReturnIndex != -1)
                {
                        if (explicitBreakIndex == -1 || carriageReturnIndex < explicitBreakIndex)
                        {
                                explicitBreakIndex = carriageReturnIndex;
                        }
                }

                if (explicitBreakIndex != -1 && static_cast<size_t>(explicitBreakIndex) < endExclusive)
                {
                        endExclusive = static_cast<size_t>(explicitBreakIndex);
                }
                else if (remaining > maxCharactersPerLine)
                {
                        const int spaceIndex = message.lastIndexOf(' ', static_cast<unsigned int>(endExclusive - 1));
                        if (spaceIndex >= static_cast<int>(start))
                        {
                                endExclusive = static_cast<size_t>(spaceIndex);
                        }
                }

                String line = message.substring(start, endExclusive);
                line.trim();

                if (line.length() > 0)
                {
                        lines.push_back(line);
                }

                start = endExclusive;
        }

        if (lines.empty())
        {
                lastDisplayedLines.clear();
                lastMessageWasList = false;
                lastMessageValid = false;
                lastMessageCurrentlyGreen = false;
                return;
        }

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

        const size_t lineCount = lines.size();
        const int16_t lineHeight = 8 * bestSize;
        const int16_t totalVisibleHeight = lineHeight * static_cast<int16_t>(lineCount);
        const int16_t startY = (tft.height() - totalVisibleHeight) / 2 + (lineHeight / 2);

        for (size_t i = 0; i < lineCount; ++i)
        {
                tft.drawString(lines[i], tft.width() / 2, startY + (lineHeight * static_cast<int16_t>(i)));
        }

        lastDisplayedLines.clear();
        lastMessageWasList = false;
        lastMessageValid = false;
        lastMessageCurrentlyGreen = false;
}

void DisplayManager::showMessage(const String& message, bool withOverlay)
{
	renderSingleMessage(message, TFT_GREEN);

	std::vector<String> displayedLines;
	displayedLines.push_back(message);
	cacheMessageToRemember(displayedLines, false);
}

void DisplayManager::clearScreen() {

	tft.fillScreen(TFT_BLACK);
}

void DisplayManager::renderMultipleMessages(const std::vector<String>& messages, uint16_t textColor)
{
	clearScreen();

	tft.setTextDatum(MC_DATUM);
	tft.setTextColor(textColor, TFT_BLACK);

	if (messages.empty())
	{
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
	const int16_t startY = (tft.height() - totalVisibleHeight) / 2 + (lineHeight / 2);

	for (size_t i = 0; i < lineCount; ++i)
	{
		tft.drawString(truncatedMessages[i], tft.width() / 2, startY + (lineHeight * static_cast<int16_t>(i)));
	}
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
