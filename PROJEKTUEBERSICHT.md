# PROJEKTUEBERSICHT.md — Nasreddins Magic Card Identifier

## Projektüberblick
ESP32/Arduino Projekt für RFID-basierte Erkennung von Zauberkarten mittels PN532 NFC Modul. Aktueller Fokus: BLE HID Tastatur (deutsches Layout) auf M5Stack AtomS3 (ESP32-S3) zum Senden von Tastenanschlägen an Android-Geräte.

## Hardware
- **Board**: M5Stack AtomS3 (ESP32-S3, 8MB Flash, 240MHz)
- **NFC Leser**: PN532 Modul (I2C/SPI/HSU) – derzeit HSU (High Speed UART)
- **Karten**: RFID Aufkleber in Tarot-Spielkarten versteckt

## Software Stack
- **IDE**: Visual Studio 2022 + Visual Micro (Arduino Plugin)
- **Framework**: Arduino Core für ESP32 v3.3.0
- **BLE Bibliothek**: NimBLE-Arduino 2.3.6 (lokal in `Libraries/NimBLE-Arduino/2.3.6/`)
- **Standard**: C++2a / GNU11

## Hauptkomponenten
| Komponente | Pfad | Zweck |
|------------|------|-------|
| BLE HID Tastatur | `Anarcho/SimpleTestEspAsKeyboard/src/HidKeyboard.h/.cpp` | Deutsche Tastatur-Layout Implementierung |
| Konstanten | `Anarcho/SimpleTestEspAsKeyboard/src/HidConsts.h` | HID Report Descriptors, UUIDs, Keycodes |
| Entry Point | `Anarcho/SimpleTestEspAsKeyboard/SimpleTestEspAsKeyboard.ino` | Serial → BLE HID Bridge mit UTF-8 Decoder |
| scrcpy Launcher | `Anarcho/Android Helper/scrcpy launcher/` | Wireless ADB/scrcpy PowerShell Scripts |

## Wichtige Konventionen
- **Deutsches HID Layout**: `cpToHid_DE()` behandelt ä/ö/ü/ß/€ und Z/Y Vertauschung
- **Boot Protocol erzwungen**: Protocol Mode = 0 (Boot) für Android Kompatibilität
- **UTF-8 Decoder**: In `.ino` für serielle Eingabe → Codepoints
- **NimBLE Callbacks**: Nutzen `ConnInfo` API (v2.3.6+)

## Entwicklung
```bash
# Build & Upload
# 1. SimpleTestEspAsKeyboard.vcxproj in Visual Studio öffnen
# 2. Build → Deploy (Visual Micro)
# 3. Serial Monitor: 115200 Baud

# BLE Pairing
# Android: Einstellungen → Bluetooth → "ESP32 BLE Keyboard" koppeln

# Wireless scrcpy
powershell -File "Anarcho/Android Helper/scrcpy launcher/StartScrcpyWireless.ps1"
```

## Bekannte Stolpersteine
- Visual Micro Pfade in `.vcxproj` sind maschinen-spezifisch (User `hesspet.TALOSLITTLESERV`) → IntelliSense nach Clone neu aufbauen
- NimBLE lokal auf 2.3.6 gepinnt → NICHT über Library Manager aktualisieren
- Kein CI/CD → alles manuell via VS + USB
- Android BLE HID ignoriert oft Report Protocol → Boot Protocol zwingend

## Referenzen
- PN532 Datasheets & Verkabelung: `Datasheets/`
- Hardware Fotos: `Images/`
- Wiki: https://github.com/hesspet/NasrredinsMagicCardIdentifier/wiki