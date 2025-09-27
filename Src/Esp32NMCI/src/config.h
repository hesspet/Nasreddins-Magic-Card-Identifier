/***************************************************************************
 * Project: Esp32NMCI
 * File: config.h
 * Repository: https://github.com/hesspet/NasrredinsMagicCardIdentifier
 * Author: Peter Heß, Büdingen DE
 * Description: Central configuration values for pins, speeds, NFC reader
 *              thresholds, and display parameters.
 ***************************************************************************/

#pragma once

#include <stdint.h>

 // Remove comment if you like to wirte data to card withou card reader via serial input
 // #define SERIAL_INPUT_ALLOWED

  // Splash text shown during startup
#define VERSION_TEXT "NMCI V1.1"

// Serial port used to communicate with the PN532 module over High Speed UART.
#define PN532_HSU_PORT Serial2

// Baud rate for the PN532 HSU serial connection.
constexpr uint32_t PN532_HSU_BAUDRATE = 115200;

// ESP32 pin receiving data from the PN532 (connect to PN532 TX).
constexpr int PN532_HSU_RX_PIN = 26;

// ESP32 pin transmitting data to the PN532 (connect to PN532 RX).
constexpr int PN532_HSU_TX_PIN = 25;

// Maximum UID length supported (Type 2 tags are 4, 7, or 10 bytes).
constexpr uint8_t kUidBufferMax = 10;

// Maximum number of user memory bytes to read from a tag (covers NTAG216).
constexpr uint16_t kUserMaxBytes = 1024;


// Duration in milliseconds before the DisplayManager dims the content
constexpr uint32_t kDisplayManager_TimeToFadeOutDisplay = 5000U;
