# Nasreddins Magic Card Identifier

A usb keyboard emulation - using RFID to hide keypresses.

An RFID-based approach to identify objects via RFID technology for magic routines. It emulates a keyboard via Bluetooth LE. Keyvalues are stored on RFID Chip and send as Keypress to a smartphone and can be used in several magic applications.

Take an RFID breakout board, connect an ESP32 board, hide RFID chips between playing cards, and you have a device with which you can develop all kinds of magic routines.

The first prototype is finished, playing cards—Tarot cards in this case—have already been prepared, and the whole project has now been published in forums.

# ESP32 Platform

* Lillygo T-Display
  * Integrated display
  * Inexpensive
  * Autonomous operation with a battery
  * Form factor is well suited to be mounted directly behind the RFID reader

  Note: Of course, we are not specifically tied to the T-Display. In principle, the idea should work with any standard ESP board that includes a battery charging function. The smaller, the better. During development the T-Display proved very helpful, because you can output information directly on the display.

# The RFID Reader (PN532)

Found on Amazon: https://www.amazon.de/dp/B0B1QB4347?ref=ppx_yo2ov_dt_b_fed_asin_title&th=1 (Aideepen 2 pieces PN532 NFC NXP RFID Module V3 Kit Reader Near Field Communication Reader Module Kit I2C SPI HSU). Works excellently, and I was able to get the reader running in no time.

* Datasheet and more information can be found at: https://github.com/hesspet/Nasreddins-Magic-Card-Identifier/tree/main/Datasheets
* Additional information: https://github.com/hesspet/Nasreddins-Magic-Card-Identifier/wiki/PN532-Readings

## Images

* <img src="Images/NMCI_NFC_MODULE_V3_ELECHOUSE_Oberseite.png" alt="Breakout top" width="400"> <img src="Images/NMCI_NFC_MODULE_V3_ELECHOUSE_Unterseite.png" alt="Breakout bottom" width="400">
* The first prototype on a breadboard: https://youtu.be/oPGHHWMIs64


# Roadmap

* DONE - ~Create project~
* DONE - ~Connect hardware (determine mode, I2C or HSU)~
  * DONE - ~Connect in the simplest version via HSU (High Speed UART)~
* ~Simple tests for reading (possibly writing) the card~
  * DONE ~Select Arduino library~
    * ~Adafruit_PN532~
      *  ~https://adafruit-pn532.readthedocs.io/en/latest/~
      *  ~https://github.com/adafruit/Adafruit-PN532~
      * ~Header with all functions: https://github.com/adafruit/Adafruit-PN532/blob/master/Adafruit_PN532.h~
* DONE ~Further develop the project concept~
  * DONE ~Simple trick~
    * DONE ~Read from chip, display e.g. on T-Display~
* DONE ~Additional ideas~
  * DONE ~Connect to smartphone as a keyboard emulation~

# More Information in the WIKI

Since everything would be too much here, please take a look at the WIKI!

https://github.com/hesspet/NasrredinsMagicCardIdentifier/wiki

# Sources:

* Internal project copies of datasheets: https://github.com/hesspet/NasrredinsMagicCardIdentifier/tree/main/Datasheets
* https://www.espboards.dev/sensors/pn532/ ESP32 PN532 NFC Module Pinout, Wiring, ESP32 and more - Good overview of the module. Especially aimed at the ESP32.
* https://www.elechouse.com the manufacturer of the breakout board (PS: The board from Amazon is probably a clone, the boards shown there look slightly different: https://www.elechouse.com/product-category/communication-shield/rfid/)
* A somewhat shorter documentation of the breakout board: https://components101.com/wireless/pn532-nfc-rfid-module
* Tasmota has direct integration: https://tasmota.github.io/docs/PN532/ (Not relevant to this project, but maybe you can find one or two ideas there.)

## Netzwerkfähige Kamera-Webanwendung

Im Ordner `NCS-TEST1` befindet sich jetzt eine kleine Node.js-Anwendung, die eine Weboberfläche bereitstellt. Diese öffnet auf kompatiblen Smartphones sofort die eingebaute Kamera, nimmt automatisch ein Foto auf und zeigt es anschließend an. Über den Button „Neues Foto aufnehmen“ kann der Vorgang wiederholt werden.

### Starten

1. Abhängigkeiten installieren (`npm install` im Verzeichnis `NCS-TEST1`).
2. Server starten (`npm start`).
3. Die Anwendung ist anschließend über `http://<IP-des-Servers>:3000` im gesamten lokalen Netzwerk erreichbar.

Die Kamera-Nutzung erfolgt über die Browser-API `navigator.mediaDevices.getUserMedia`. Für mobile Geräte wird automatisch die Rückkamera (`facingMode: environment`) angefragt.
