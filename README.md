# HarviaWiFi

Arduino code to control Harvia sauna heaters 

## Quick start

### Controlling the sauna

## Contents
- sources
- schematic
- my installation images

## Firmware requirements
- You will need latest Arduino IDE
- Install ESP8266 board in the IDE
- Download this code
- Connect SONOFF TH16 to serial <--> usb adapter
- Flash Settings
    - Generic ESP8266 Module
    - CPU Frequency 80Mhz
    - Flash Size 1M 64k SPIFFS
    - Flash Mode DIO
    - Debug None
    - Port COM (select from serial list)
    - All other settings default (unless over air after first flash)
- Flash OTA Settings
  - Debug Level OTA
  - Port sauna at xxx.xxx.xxx.xxx

## First time flashing
- !!! DO NOT have mains connected to device when flashing with a serial <-> usb adapter. It will destroy the device and quite possible your computer and you might even kill yourself !!!
- Connect SONOFF TH16 via serial <-> usb adapterto computer (make sure you yours has 3v3 option), do not plug into your computer yet
- Hold white button down and then connect usb side of adapter to your computer.
- Select the COM device under Port setting of the IDE and then Upload
- After flashing is complete, it should reboot and be in captive portal mode.
    - Connect your table or phone to open wifi "sauna" and it should ask you to sign in. If not opern your browser and go to 192.168.1.1
    - Enter config details necessary to connect to your network and MQTT broker. The device will reboot and connect to your network. You should see data on MQTT broker.
    - If you mess up configuration at anytime, just hold button down for at least 30 seconds and this will reset back to captive portal mode.
- Now that you have flashed first time, the device should be listed in the OTA sectin of port selection. If you want to make changes, you can now OTA flash. You can OTA flash even when installed in Sauna. Your Sonoff TH16 may now be install into your sauna. Remeber turn off your breaker before doing so!!!

## Notes
- Never ever have mains connected to device when flashing with a serial <-> usb adapter
- In order to Flash for the first time, you will need to solder wires or headers to the board. When ready to flash, hold push button down and apply power.
<img src="https://github.com/SwiCago/HarviaWiFi/blob/master/images/board.png"/>
- High voltage 240V !!! Always have the breaker off, when working on your Sauana's electrical.
- How to backup SONOFF firmware, in case you want to put original firmware back on it.
    - esptool.py --port COMPORT read_flash 0x00000 0x100000 sonoff_TH16.bin

## New Circuit Diagram

<img src="https://github.com/SwiCago/HarviaWiFi/blob/master/images/schematic.png"/>

## Original Circuit Diagram

<img src="https://github.com/SwiCago/HarviaWiFi/blob/master/images/schematic_original.png"/>

## Parts

- SONOFF TH16
    - https://www.amazon.com/Sonoff-TH16-Temperature-Monitoring-Compatible/dp/B06XTNSJ46/ref=sr_1_2?ie=UTF8&qid=1549127783&sr=8-2&keywords=sonoff+th16
- DS18B20 Waterproof Temperature Sensor
    - https://www.amazon.com/Sonoff-DS18B20-Waterproof-Temperature-Automation/dp/B07F34DF2T/ref=sr_1_5?ie=UTF8&qid=1549127783&sr=8-5&keywords=sonoff+th16
- 2.5mm extension cable
    - https://www.amazon.com/YCS-Basics-Conductor-Headphone-Extension/dp/B00FJEH1PY/ref=sr_1_2_sspa?ie=UTF8&qid=1549127906&sr=8-2-spons&keywords=3m+2.5mm+4+pin+extension+cable&psc=1

## Licence

Licensed under the GNU Lesser General Public License.
https://www.gnu.org/licenses/lgpl.html
