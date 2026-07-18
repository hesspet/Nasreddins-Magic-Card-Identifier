# AGENTS.md — Nasreddins Magic Card Identifier

## Project Overview
ESP32/Arduino project for RFID-based magic card identification using PN532 NFC module. Current focus: BLE HID keyboard (German layout) on M5Stack AtomS3 (ESP32-S3) to send keystrokes to Android.

## Build / Develop
- **IDE**: Visual Studio 2022 + Visual Micro extension (Arduino plugin)
- **Project**: `Anarcho/SimpleTestEspAsKeyboard/SimpleTestEspAsKeyboard.vcxproj`
- **Board**: `esp32_m5stack_atoms3` (ESP32-S3, 8MB flash, 240MHz)
- **Framework**: Arduino core for ESP32 v3.3.0
- **Key library**: NimBLE-Arduino 2.3.6 (local copy in `Libraries/NimBLE-Arduino/2.3.6/`)
- **Compile standard**: C++2a / GNU11
- **Upload**: esptool_py via USB (CDC on boot enabled)

## Directory Structure
```
Anarcho/
  SimpleTestEspAsKeyboard/     # Main BLE HID Keyboard firmware
    src/
      HidKeyboard.h/.cpp       # BLE HID implementation (German layout)
      HidConsts.h              # HID report descriptors, UUIDs, key codes
    SimpleTestEspAsKeyboard.ino # Entry point: Serial→BLE HID bridge
    Libraries/                 # Local deps (NimBLE-Arduino)
  Android Helper/
    scrcpy launcher/           # Wireless ADB/scrcpy PowerShell scripts
```

## Key Conventions
- **German HID layout** in `HidKeyboard.cpp`: `cpToHid_DE()` handles ä/ö/ü/ß/€, Z/Y swap
- **Boot protocol forced** (Android compatibility): Protocol Mode = 0 (Boot) by default
- **UTF-8 decoder** in `.ino` for serial input → codepoints
- **NimBLE callbacks** use `ConnInfo` API (v2.3.6+)

## Common Tasks
| Task | Command / Action |
|------|------------------|
| Build/Upload | Open `.vcxproj` in VS → Build → Deploy (Visual Micro) |
| Serial monitor | VS Terminal (115200 baud) or Arduino IDE Serial Monitor |
| Pair BLE | Android: Settings → Bluetooth → Pair "ESP32 BLE Keyboard" |
| scrcpy wireless | Run `Anarcho/Android Helper/scrcpy launcher/StartScrcpyWireless.ps1` |

## Gotchas
- **Visual Micro paths** in `.vcxproj` are machine-specific (user `hesspet.TALOSLITTLESERV`); rebuild IntelliSense after clone
- **NimBLE local copy** pinned to 2.3.6; don't auto-update via Library Manager
- **No CI/CD**; all manual via VS + USB
- **Android BLE HID** requires Boot protocol (Report protocol often ignored)

## References
- PN532 wiring/datasheets: `Datasheets/`
- Hardware photos: `Images/`
- Wiki: https://github.com/hesspet/NasrredinsMagicCardIdentifier/wiki