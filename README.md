# Nasreddins Magic Card Identifier

Eine RFID Variante um bei Zauberroutinen Gegenstände via RFID Technik zu identifizieren

Man nehme ein RFUD Breakout Board, hänge ein ESP32 dran und verstecke RFID Aufkleber zwischen Spielkarten und schon hat man ein Gerät mit dem man die verschiedensten Zauberroutinen entwicklen kann.

Dieses Projekt ist noch in er Planung. Erste Gerätschaften sind besorgt. Und Spielkarten, in diesem Fall Tarot Karten, sind bereits angefertigt.

# ESP32 Plattform Prototype

* Lillygo T-Display
  * Integriertes Display
  * Preisgünstig
  * Autonomer Betrieb mit Akku
  * Bauform eignet sich ganz gut um direkt hinter den RFID Leser montiert zu werden

# Der RFID Leser

Gefunden bei Amazon: https://www.amazon.de/dp/B0B1QB4347?ref=ppx_yo2ov_dt_b_fed_asin_title&th=1 (Aideepen 2 Stück PN532 NFC NXP RFID-Modul V3 Kit Reader Nahfeldkommunikationsleser-Modul-Kit I2C SPI HSU) Günstig, sieht vielversprechend aus.

* Datasheet
* Weitere Infos
* Arduino Lib

## Bilder

* <img src="Images/NMCI_NFC_MODULE_V3_ELECHOUSE_Oberseite.png" alt="Breakout Oberseite" width="400"> <img src="Images/NMCI_NFC_MODULE_V3_ELECHOUSE_Unterseite.png" alt="Breakout Unterseite" width="400">
* Der erste Prototype auf Breadboard: https://youtu.be/oPGHHWMIs64
   
# Roadmap

* ERLEDIGT - ~Projekt anlegen~
* ERLEDIGT - ~Hardware anbinden (Modus klären, I2C oder HSU)~
  * Anbindung in der einfachsten Version via HSU (High Speed UART)

* Einfache Tests Lesen (ggf. Schreiben) der Karte
  * ERLEDIGT ~Arduino Library auswählen~
    * Adafruit_PN532
      *  https://adafruit-pn532.readthedocs.io/en/latest/
      *  https://github.com/adafruit/Adafruit-PN532
      * Header mit allen Funktionen: https://github.com/adafruit/Adafruit-PN532/blob/master/Adafruit_PN532.h
* Projektkonzept weiter ausarbeiten
  * Einfaches Kunststück
    * Lesen auf Chip, anzeige z.B. in T-Display
* Weitere Ideen
  * Anbindung an Smarthone
    * als PWA  mit IP
    * MAUI mit Bluetooth BLE
    * ???
   
# Weitere Infos im WIKI

https://github.com/hesspet/NasrredinsMagicCardIdentifier/wiki

# Quellen:

* Projektintern, Kopien von Datasheets: https://github.com/hesspet/NasrredinsMagicCardIdentifier/tree/main/Datasheets
* https://www.espboards.dev/sensors/pn532/ ESP32 PN532 NFC Module Pinout, Wiring, ESP32 and more - Gute Übersicht über das Modul. Speziell in Richtung ESP32 gedacht.
* https://www.elechouse.com der Hersteller des Breakoutboards (PS: Bei dem Board von Amazon dürfte es sich um einen Clone handeln, die her gezeigten Boards sehen doch etwas anders aus: https://www.elechouse.com/product-category/communication-shield/rfid/)
* Eine etwas kürzere Dokumentation des Breakoutboards: https://components101.com/wireless/pn532-nfc-rfid-module
* Tasmota hat eine direkte Einbindung: https://tasmota.github.io/docs/PN532/ (Ist hier nicht projektrelevant, aber vielleicht kann man sich da die eine oder andere Idee mal anschauen.)
