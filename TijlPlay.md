TijlPlay is een slim toestel dat weet hoe je wandelt. Het doet dit door je bewegingspatroon te volgen.
Stap je correct, dan werkt de mp3 speler, anders pauzeert die. 

## Componenten

### Wemos D1 mini Lite

De Wemos D1 mini heeft alle aansluitingen die we nodig hebben. Het vormt ons centrale brein. Niet het meest efficient, maar eenvoudig om mee te starten.

### Adafruit feather 328p – atmega328p 3.3 V @ 8 MHz 
De [Feather 328p](https://learn.adafruit.com/adafruit-feather-328p-atmega328-atmega328p) zou de wemos kunnen vervangen, maar we hebben er geen zo'n goede ervaringen mee. We gebruiken dit bord enkel omdat het een automatisch laadcircuit heeft voor een LiPo batterij. De batterij plugt in de Feather, de Feather geeft ons 3.3V voeding, welke we gebruiken om de Wemos aan te sturen.

WiFi is geen vereiste voor ons project, maar kan wel handig zijn om remote te debuggen.

#### Feather in Arduino IDE
Om deze beschikbaak te maken in de Arduino IDE dien je de handleiding te volgen van [adafruit](https://learn.adafruit.com/adafruit-feather-328p-atmega328-atmega328p/arduino-ide-setup#advanced-arduino-ide-setup-5-13).  
Dus, voeg `https://adafruit.github.io/arduino-board-index/package_adafruit_index.json` toe in je settings als **Additional Boards Manager URL**. 

Vervolgens via **Bord Beheer** de borden _Adafruit AVR Boards_ toevoegen. Je kan dan als bord selecteren **Adafruit Feather 328P**

#### Alternatieven

1. We kunnen een externe power bank gebruiken om de Wemos rechtstreeks te voeden. Deze dan regelmatig laden.
2. Een [Wemos battery shield](https://wiki.wemos.cc/products:d1_mini_shields:battery_shield) kan gebruikt worden ipv de Feather. Is ook kleiner. Voltage kun je via A0 opvolgen [en zo bepalen of batterij bijna leeg is](https://arduinodiy.wordpress.com/2016/12/25/monitoring-lipo-battery-voltage-with-wemos-d1-minibattery-shield-and-thingspeak/)

### Batterij
De Feather of Wemos Shield kan een LiPo batterij opladen, en de lading omzetten naar 5V voor gebruik door de processor. We gebruiken een 3.7V 600mAh batterij. 


### OLED 128x32 Display

The display is connected over I2C. We use on the feather pin SDA and pin SCL for the SDA SCK pins of the OLED

Initialization of the U8g2 depends on the OLED you bought. You might need to change this for another OLED. In our case it is

// Create a display object
U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, pSCL, pSDA, U8X8_PIN_NONE);

### MPU-6050 3-Assige gyro en versnellingsmeter GY-521

Hiermee meten we de beweging van de wandelaar. Voor het uitlezen van de waarden gebruiken we [TinyMPU6050](https://github.com/gabriel-milan/TinyMPU6050), welke via bilbiothekenbeheer kan geinstalleerd worden.

Evenwel dienen we voor de WeMos enkele wijzigingen te doen, dus gebruiken we onze [eigen versie van deze bibliotheek](https://github.com/ingegno/TinyMPU6050)

### MP3 DFPlayer Mini

Voor de MP3 gebruiken we de [DFPlayer Mini](https://wiki.dfrobot.com/DFPlayer_Mini_SKU_DFR0299). 
Om deze in de Arduino IDE te kunnen gebruiken dien je de [DFRobotDFPlayerMini zip libary](https://github.com/DFRobot/DFRobotDFPlayerMini/archive/1.0.3.zip) in de Arduino IDE toe te voegen

### 3.5 mm Jack

De 3.5 mm jack is beschikbaar als een breakout board met connecties Sleeve, Tip, Ring 1 en Ring 2. De Sleeve moet naar GND. De Tip en Ring1 connecteer je met SPK1 en SPK2 van de DFPlayer. 

### SD Card Adapter
Optional to store data. This works with SPI connection to the wemos. See connections on [this guide](https://www.instructables.com/id/SD-Card-Module-With-ESP8266/), as well as examples

## References

1.  [test](https://test)
