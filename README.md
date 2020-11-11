Smartshoe, a shoe that knows how you walk!

Een slimme schoen die weet hoe je wandelt!

# Ontwikkelaars

## Algemeen

Alle info van Microbit terug te vinden op [microbit.org](https://microbit.org)

Je kan code schrijven in de python programmeertaal voor MicroBit online, zie [Microbit v1.1 editor](https://python.microbit.org/v/1.1)

## Microbit Linux

We ontwikkelen op linux voor Microbit. Op linux doe voor installatie

    pip3 install mu-editor
    
Daarna start de editor via

    python3 ~/.local/lib/python3.6/site-packages/mu/__main__.py
    
Open de test code van `/testsketches/test01_microbit.py`, connecteer je microbit via een micro usb kabel met je PC. Je PC zal die herkennen als een usb drive, open die met de file manager. Je kan nu via de mu editor code flashen via de Flash knop in het menu.


## Accelerometer

Test nu de accellerometer in de microbit met de test sketch `/home/mcciocci/git/smartshoe/testsketches/test02_microbit_accelerometer.py`.

Druk in de mu-editor op de plotter om de output te zien.
Je stelt vast dat de output rond 980 correspondeert met 1g dus 9.8 m/s2. Als microbit naast je schoen hangt, met je zool plat op de grond, 
krijg je Y-waarde tussen 950 and 1050, dit omdat Y+ op Microbit naar beneden wijst. Hiermee kunnen we de beweging van de voet volgen, 
maar schokken zullen problematisch zijn... wiskunde nodig!

### Stap Patroon meten
We dienen te meten wat effect is van verschillende manieren van stappen. Hiermee zoeken we een patroon welke we kunnen gebruiken om juist of fout
stappen te herkennen. 

Laat op de microbit de code `test02a_microbit_accelerometer.py`, welke accelerometer data naar serial schrijft elke 20 milliseconden, en we gebruiken 
om het patroon te besturen. Op de computer die je aan de microbit connecteert laat je `test02b_saveserial2csv.py` lopen. Deze slaat de data op in een 
csv file welke je met een spreadsheet kunt bestuderen.

Met een PC in de hand lopen is niet handig. We passen de code daarom aan om via bluetooth de data door te sturen ipv via serial connectie. Op je PC, schakel
bluetooth aan. Kijk naar de devices die aanwezig zijn. Op de microbit flash je de hex code `test03a_sendaccel2bluetooth.hex` door de microbit aan de PC te hangen
en deze hex op de usb folder te copieren.
Deze hex file komt overeen met javascript `test03a_sendaccel2bluetooth.js` welke je via [makecode editor](https://makecode.microbit.org/#editor) in een 
hex kan omvormen.

We vangen de data op die de microbit uitstuurt. Hiertoe hebben we bluepy nodig op onze linux PC. Doe hiervoor:

    sudo apt-get install python3-pip libglib2.0-dev
    sudo pip3 install bluepy
 
Eerst moet je nog je PC pairen met de microbit. **Dit moet je doen telkens je een nieuwe hex op de microbit zet**. 
Volg de guide op [microbit](https://support.microbit.org/support/solutions/articles/19000051025-pairing-and-flashing-code-via-bluetooth).
Dus 3 toesten indrukken, wachten op LEDs op scherm, reset loslaten. Hierna zou je in de bluetooth app van je PC de microbit moeten zien verschijnen.
Doe connect, om te zorgen dat je paired bent met de microbit. Enkel deze PC zal nu aanvaard worden. Doe disconnect als gelukt.

We zullen nu een connect doen met de microbit vanuit een python script. Dit script zal dus het paspatroon opvangen en opslaan om zo het te 
kunnen besturen. Start script met

    python3 test03b_capturebluetooth.py

Lees output data via een spreadsheet, en kijk welke patronen je kan herkennen.

## Accelerometer-gyroscope MPU 6050

Dit is de beste accelerometer-gyroscope op de markt op dit moment voor makers. De logic is 3.3V, dus geschikt voor wemos of nodemcu, 
anders een logic voltage converter nodig.

Uitlezen van data kan via [i2cdevlib](https://www.i2cdevlib.com/devices/mpu6050#source). Download de source code

    git clone https://github.com/jrowberg/i2cdevlib.git
    
## MP3 player
We connecteren een mp3 speler. Zie [eigen documentatie](MP3_play.md)

## Nano + Microbit
Als finale prototype combineren we een microbit met een arduino nano. De nano draait programma
[ToeWalkBelt.ino](ToeWalkBelt/ToeWalkBelt.ino) en stuurt de MP3 player aan. Dit door een pin op de Microbit te lezen die aanduidt of correct of fout gewandeld wordt. De mp3 kan via een knop ook in MP3 modus gebracht worden waarbij het mediabestand afgespeeld wordt (en via pauze knop gepauzeerd).

De microbit doet de berekening om te weten of een persoon al dan niet juist wandelt. Deze code is [test04_detect_toewalk.py](testsketches/test04_detect_toewalk.py). Er zijn twee calibratiecurves opgenomen. Met knop B van de microbit schakel je tussen de twee. Je dient te wandelen met een snelheid > 80 stappen per minuut (1 been neer is 1 stap, dus normale snelheid is 100 tot 140 stappen), en de gemeten schokken dienen onder de calibratiecurve te liggen. In dat geval wordt pin1 LOW, anders HIGH. De nano leest dit in om hierop te reageren.

Alles wordt gevoed via een batterij: Samsung Li-ion INR18650 25R 20A 2500 MAH - ONBEVEILIGD. Een laadcircuit is aanwezig (TP4056), en een step-up cicuit om dit naar 5V te brengen nodig voor de Nano. This setup is explained for example [here](https://rlopezxl.com/2018/05/17/power-your-projects-with-a-built-in-lithium-battery-and-a-tp4056-charger/)

## Adafruit Feather gebaseerd device

Er is aangetoond dat het geheel moet werken. Voor het eindproduct willen we het aantal componenten reduceren. We gebruiken daarom een Adafruit Feather.

1. [Power Switch op Feather](https://io.adafruit.com/blog/tip/2016/12/14/feather-power-switch/)
2. [Power management pins](https://learn.adafruit.com/adafruit-feather-32u4-bluefruit-le/power-management)
3. [All pinouts](https://learn.adafruit.com/adafruit-feather-328p-atmega328-atmega328p/pinouts)
4. [OLED display wiring](https://learn.adafruit.com/adafruit-oled-featherwing/python-circuitpython-wiring)

# References

1. [arduino thread on toe walker](https://forum.arduino.cc/index.php?topic=194029.0)
2. [Toe walking video](https://www.youtube.com/watch?v=Gwdmnnh4-Y4&hd=1)
3. [Adafruit sneakers with pressure/step detection](https://learn.adafruit.com/firewalker-led-sneakers/firewalker-code)
4. [Filtering and presenting gyro and accelerometer data](https://www.instructables.com/id/Guide-to-gyro-and-accelerometer-with-Arduino-inclu/)
5. [DIY smart insoles with pressure sensors](https://www.hackster.io/Juliette/a-diy-smart-insole-to-check-your-pressure-distribution-a5ceae)
6. [simple pedometer with accelerometer](https://www.instructables.com/id/Simple-Easy-and-Cheap-DIY-Pedometer-with-Arduino/)
7. [linux bluetooth connection to microbit](http://bluetooth-mdw.blogspot.com/2017/02/pairing-bbc-microbit-with-raspberry-pi.html)
8. [accelerometer datalogging example](https://microbit.org/en/2018-09-03-python-mu-datalogging/)
9. [uart bluetooth example microbit-python](https://github.com/alcir/microbit-ble)
