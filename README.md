# Nasreddins Magic Card Identifier

Eine RFID Variante um bei Zauberroutinen Gegenstände via RFID Technik zu identifizieren

Man nehme ein RFID Breakout Board, hänge ein ESP32-Board dran und verstecke RFID Chips zwischen Spielkarten und schon hat man ein Gerät mit dem man die verschiedensten Zauberroutinen entwicklen kann.

Der erste Prototyp ist fertig und Spielkarten, in diesem Fall Tarot Karten, sind bereits angefertigt und das Ganze ist nun in Foren publiziert.

# ESP32 Plattform 

* Lillygo T-Display
  * Integriertes Display
  * Preisgünstig
  * Autonomer Betrieb mit Akku
  * Bauform eignet sich ganz gut um direkt hinter den RFID Leser montiert zu werden
 
 Hinweis: Natürlich sind wir nicht speziell auf das T-Display festgelegt. Im Prinzip sollte die Idee mit jedem normalen ESP Board mit einer Akkuladefunktion umgesetzt werden können. Je kleiner um so besser. In der Entwicklungsphase hat sich das T-Display aber als sehr hilfreich erwiesen, da man natürlich direkt auf dem Display Informationen ausgeben kann.

# Der RFID Leser (PN532)

Gefunden bei Amazon: https://www.amazon.de/dp/B0B1QB4347?ref=ppx_yo2ov_dt_b_fed_asin_title&th=1 (Aideepen 2 Stück PN532 NFC NXP RFID-Modul V3 Kit Reader Nahfeldkommunikationsleser-Modul-Kit I2C SPI HSU). Funktioniert hervorragend und ich konnte den Leser innerhalb kürzester Zeit zum Laufen bringen.

* Datasheet und weitere Infos findet Ihr unter: https://github.com/hesspet/Nasreddins-Magic-Card-Identifier/tree/main/Datasheets
* Weitere Infos
* Arduino Lib

## Bilder

* <img src="Images/NMCI_NFC_MODULE_V3_ELECHOUSE_Oberseite.png" alt="Breakout Oberseite" width="400"> <img src="Images/NMCI_NFC_MODULE_V3_ELECHOUSE_Unterseite.png" alt="Breakout Unterseite" width="400">
* Der erste Prototype auf Breadboard: https://youtu.be/oPGHHWMIs64

## Video

https://youtu.be/G3h274TyvUc - Beispiel wie man es vorführen kann. Bauhinweise und was mir sonst noch so eingefallen ist.
   
# Roadmap

* ERLEDIGT - ~Projekt anlegen~
* ERLEDIGT - ~Hardware anbinden (Modus klären, I2C oder HSU)~
  * ERLEDIGT - Anbindung in der einfachsten Version via HSU (High Speed UART)
* Einfache Tests Lesen (ggf. Schreiben) der Karte
  * ERLEDIGT ~Arduino Library auswählen~
    * Adafruit_PN532
      *  https://adafruit-pn532.readthedocs.io/en/latest/
      *  https://github.com/adafruit/Adafruit-PN532
      * Header mit allen Funktionen: https://github.com/adafruit/Adafruit-PN532/blob/master/Adafruit_PN532.h
* ERLEDIGT Projektkonzept weiter ausarbeiten
  * ERLEDIGT Einfaches Kunststück
    * ERLEDIGT Lesen auf Chip, anzeige z.B. in T-Display
* ERLEDIGT Weitere Ideen
  * ERLEDIGT Anbindung an Smarthone als Tastaturemulation
   
# Weitere Infos im WIKI

https://github.com/hesspet/NasrredinsMagicCardIdentifier/wiki

# Quellen:

* Projektintern, Kopien von Datasheets: https://github.com/hesspet/NasrredinsMagicCardIdentifier/tree/main/Datasheets
* https://www.espboards.dev/sensors/pn532/ ESP32 PN532 NFC Module Pinout, Wiring, ESP32 and more - Gute Übersicht über das Modul. Speziell in Richtung ESP32 gedacht.
* https://www.elechouse.com der Hersteller des Breakoutboards (PS: Bei dem Board von Amazon dürfte es sich um einen Clone handeln, die her gezeigten Boards sehen doch etwas anders aus: https://www.elechouse.com/product-category/communication-shield/rfid/)
* Eine etwas kürzere Dokumentation des Breakoutboards: https://components101.com/wireless/pn532-nfc-rfid-module
* Tasmota hat eine direkte Einbindung: https://tasmota.github.io/docs/PN532/ (Ist hier nicht projektrelevant, aber vielleicht kann man sich da die eine oder andere Idee mal anschauen.)
