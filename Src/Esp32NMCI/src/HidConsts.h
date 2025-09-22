/**
 * @file HidConsts.h
 * @brief Stellt Konstanten und Report-Definitionen für das HID-Tastaturprofil bereit.
 */

#pragma once

#include <stdint.h>

 // ---------- HID Keycodes / Modifier Fallbacks ----------
#ifndef HID_KEY_ENTER
#define HID_KEY_A              0x04
#define HID_KEY_B              0x05
#define HID_KEY_C              0x06
#define HID_KEY_D              0x07
#define HID_KEY_E              0x08
#define HID_KEY_F              0x09
#define HID_KEY_G              0x0A
#define HID_KEY_H              0x0B
#define HID_KEY_I              0x0C
#define HID_KEY_J              0x0D
#define HID_KEY_K              0x0E
#define HID_KEY_L              0x0F
#define HID_KEY_M              0x10
#define HID_KEY_N              0x11
#define HID_KEY_O              0x12
#define HID_KEY_P              0x13
#define HID_KEY_Q              0x14
#define HID_KEY_R              0x15
#define HID_KEY_S              0x16
#define HID_KEY_T              0x17
#define HID_KEY_U              0x18
#define HID_KEY_V              0x19
#define HID_KEY_W              0x1A
#define HID_KEY_X              0x1B
#define HID_KEY_Y              0x1C
#define HID_KEY_Z              0x1D
#define HID_KEY_1              0x1E
#define HID_KEY_2              0x1F
#define HID_KEY_3              0x20
#define HID_KEY_4              0x21
#define HID_KEY_5              0x22
#define HID_KEY_6              0x23
#define HID_KEY_7              0x24
#define HID_KEY_8              0x25
#define HID_KEY_9              0x26
#define HID_KEY_0              0x27
#define HID_KEY_ENTER          0x28
#define HID_KEY_ESCAPE         0x29
#define HID_KEY_BACKSPACE      0x2A
#define HID_KEY_TAB            0x2B
#define HID_KEY_SPACEBAR       0x2C
#define HID_KEY_MINUS          0x2D
#define HID_KEY_EQUAL          0x2E
#define HID_KEY_LEFT_BRACKET   0x2F
#define HID_KEY_RIGHT_BRACKET  0x30
#define HID_KEY_BACKSLASH      0x31
#define HID_KEY_SEMICOLON      0x33
#define HID_KEY_APOSTROPHE     0x34
#define HID_KEY_GRAVE          0x35
#define HID_KEY_COMMA          0x36
#define HID_KEY_PERIOD         0x37
#define HID_KEY_SLASH          0x38
#define HID_KEY_NON_US_BACKSLASH 0x64
#endif

#ifndef KEYBOARD_MODIFIER_LEFTSHIFT
#define KEYBOARD_MODIFIER_LEFTCTRL    0x01
#define KEYBOARD_MODIFIER_LEFTSHIFT   0x02
#define KEYBOARD_MODIFIER_LEFTALT     0x04
#define KEYBOARD_MODIFIER_LEFTGUI     0x08
#define KEYBOARD_MODIFIER_RIGHTCTRL   0x10
#define KEYBOARD_MODIFIER_RIGHTSHIFT  0x20
#define KEYBOARD_MODIFIER_RIGHTALT    0x40 // AltGr
#define KEYBOARD_MODIFIER_RIGHTGUI    0x80
#endif

// ---------- UUIDs ----------

// Primärer Dienst, über den das Gerät als HID-Tastatur identifiziert wird.
static const uint16_t UUID_HID_SERVICE = 0x1812;

// Characteristic mit den allgemeinen Geräteeigenschaften (Version, Landeskode, Flags).
static const uint16_t UUID_HID_INFORMATION = 0x2A4A;

// Characteristic, welche die gesamte HID-Report-Map (Layout der Reports) bereitstellt.
static const uint16_t UUID_REPORT_MAP = 0x2A4B;

// Characteristic, über die der Host das Gerät z. B. in den Suspend-Zustand schicken kann.
static const uint16_t UUID_HID_CONTROL_POINT = 0x2A4C;

// Characteristic für die eigentlichen HID-Reports (Input, Output und Feature Reports).
static const uint16_t UUID_REPORT = 0x2A4D;

// Characteristic zum Umschalten zwischen Boot- und Report-Protokollmodus.
static const uint16_t UUID_PROTOCOL_MODE = 0x2A4E;

// Boot-Protokoll-Eingabereport für Tastaturen (wird vor allem von BIOS/UEFI genutzt).

static const uint16_t UUID_BOOT_KB_INPUT = 0x2A22;

// Boot-Protokoll-Ausgabereport für Tastaturen (z. B. LED-Steuerung wie CapsLock).
static const uint16_t UUID_BOOT_KB_OUTPUT = 0x2A32;

// Descriptor, der zu einem Report die Report-ID und den Report-Typ referenziert.
static const uint16_t UUID_RPT_REF_DESC = 0x2908;

// ---------- Report-Map ----------

/*
 * KEYBOARD_REPORTMAP beschreibt den Aufbau des vom Gerät gelieferten HID-Tastaturreports:
 *  - Die ersten Einträge (0x05, 0x01, 0x09, 0x06, 0xA1, 0x01) setzen Usage Page und Usage
 *    auf "Generic Desktop / Keyboard" und eröffnen die Application Collection.
 *  - Die darauffolgenden Definitionen (Report-ID 1) legen acht Eingabebits für die Modifier
 *    E0..E7 fest, gefolgt von einem reservierten Byte zur Kompatibilität mit dem Boot-Protokoll.
 *  - Danach folgt ein LED-Output-Report mit fünf Bits (NumLock bis Kana) sowie drei Padding-Bits.
 *  - Der abschließende Abschnitt definiert sechs Byte breite Keycode-Slots für bis zu sechs
 *    gleichzeitig gedrückte Tasten im Bereich 0x00 bis 0x65 (Standard 6-Key-Rollover).
 */
static const uint8_t KEYBOARD_REPORTMAP[] = {
  0x05, 0x01, 0x09, 0x06, 0xA1, 0x01,
	0x85, 0x01,                   // Report ID = 1 (Input)
	0x05, 0x07,
	0x19, 0xE0, 0x29, 0xE7,       // Modifiers
	0x15, 0x00, 0x25, 0x01,
	0x75, 0x01, 0x95, 0x08,
	0x81, 0x02,                   // Input (Data,Var,Abs)

	0x75, 0x08, 0x95, 0x01,
	0x81, 0x01,                   // Reserved

	0x05, 0x08,
	0x19, 0x01, 0x29, 0x05,       // LEDs
	0x75, 0x01, 0x95, 0x05,
	0x91, 0x02,                   // Output (Data,Var,Abs)
	0x75, 0x03, 0x95, 0x01,
	0x91, 0x01,                   // Output (Const,Arr,Abs) padding

	0x05, 0x07,
	0x19, 0x00, 0x29, 0x65,       // Keys 0..101
	0x15, 0x00, 0x25, 0x65,
	0x75, 0x08, 0x95, 0x06,
	0x81, 0x00,                   // Input (Data,Arr,Abs)
  0xC0
};
