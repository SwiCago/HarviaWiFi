# HarviaWiFi

Arduino code to control Harvia sauna heaters, with esp8266 enabled sonoff TH device

<img src="https://github.com/SwiCago/HarviaWiFi/blob/master/images/sauna.jpg"/>

WARNING: Modifing your Sauna heater with WiFi can cause death if you are not careful. You are dealing with 240V, so take precautions not to kill yourself. I am not responsible if you kill yourself or burn your sauna/house down. You have been warned.

## Contents
- sources
- schematics
- instructions

## Controlling the sauna
### Captive Portal
<img src="https://github.com/SwiCago/HarviaWiFi/blob/master/images/CaptivePortal1.png"/>

- First start is via Captive portal
    - Access Point Configuration
    - MQTT Configuration
    - Sauna Configuration
    - See more captive portal sample images in <a href="https://github.com/SwiCago/HarviaWiFi/tree/master/images">images folder</a>
    
### MQTT DASH APP
<img src="https://github.com/SwiCago/HarviaWiFi/blob/master/images/Sauna_App_Heating.png"/>

- MQTT DASH app
    - https://play.google.com/store/apps/details?id=net.routix.mqttdash&hl=en
    - Turn on/off sauna
    - Set point temperature in C
    - View Temperature in C/F, operating state and AutoOff status
    - See Resource files for faster config of APP
    - See more APP sample images in <a href="https://github.com/SwiCago/HarviaWiFi/tree/master/images">images folder</a>
- In order to use this APP or any other MQTT app, you will need to install an MQTT broker.
    - Mosquitto is such a broker and easy to install. Do google search for instructions
- Integrations
    - With MQTT you can easily integrate with openhab and homeassistant, which in turn can be linked with alexa or google assistant.

## Flashing firmware
### Firmware requirements
- You will need latest Arduino IDE
- Install ESP8266 board in the IDE
- Download <a href="https://github.com/knolleary/pubsubclient">pubsub client</a> library for arduino
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

### First time flashing
- !!! DO NOT have mains connected to device when flashing with a serial <-> usb adapter. It will destroy the device and quite possibly your computer and you might even kill yourself !!!
- Connect SONOFF TH16 via serial <-> usb adapterto computer (make sure yours has 3v3 option), do not plug into your computer yet
- Hold white button down and then connect usb side of adapter to your computer.
- Select the COM device under Port setting of the IDE and then Upload
- After flashing is complete, it should reboot and be in captive portal mode.
    - Connect your table or phone to open wifi "sauna" and it should ask you to sign in. If not open your browser and go to 192.168.1.1
    - Enter config details necessary to connect to your network and MQTT broker. The device will reboot and connect to your network. You should see data on MQTT broker.
    - If you mess up configuration at anytime, just hold button down for at least 30 seconds and this will reset back to captive portal mode.
- Now that you have flashed first time, the device should be listed in the OTA section of port selection. If you want to make changes, you can now OTA flash. You can OTA flash even when installed in Sauna. Your Sonoff TH16 may now be install into your sauna. Remeber turn off your breaker before doing so!!!

### Notes

- I repeat! Never ever have mains(240V or 120V) connected to device when flashing with a serial <-> usb adapter
- In order to Flash for the first time, you will need to solder wires or headers to the board. When ready to flash, hold push button down and apply power.
- How to backup SONOFF firmware, in case you want to put original firmware back on it.
    - esptool.py --port COMPORT read_flash 0x00000 0x100000 sonoff_TH16.bin
- Note: the sonoff TH16 has pads for UFL connector(bottom left of pic below). This allows you to use an external antenna if needed.
<img src="https://github.com/SwiCago/HarviaWiFi/blob/master/images/TH16_board.png"/>

## Hardware Installation

## Installation
- Turn OFF breaker, 240V will kill you.
- Review Circuit diagrams, so you know which spade connectors to move
- Put 6 20A fuses into fuse block and close
- Open bottom panel of Harvia heater
- Install Fuse block in a convinient spot, I chose to install where thermostat and timer switch were.
- Find the Thermostat
    - It has 6 terminals
    - Move the 3 from same side and connect to fuse block, same side
    - Move the other 3 from other side and connect to fuse block, on opposite side.
    - Be sure to keep order of wires, label T1 T2 T3
- Find timer
    - There are 3 wires, one goes to limit switch and the other 2 go to contactor
    - Two will be close together and one on the side kind of alone. Move the lone one to one side of fuse block. Label as R
    - Now two are left. Take the one that goes to limit switch and connect to fuse block. Label L2
    - Now there is one left and it goes to contactor, move it to fuse block and label L1
    - You should now have all 3 wires on one side of fuse block
- Remove or move the Timer and Thermostat(including its sensor). I moved mine to the side and not in my garage where they will get lost.
- Connecting Wifi SONOFF TH16
    - Make 3 long jumper wires with spade connectors on them
    - From fuse block opposite L1 connect to sonoff N
    - From fuse block opposite L2 connect to sonoff L(in)
    - From fuse block opposite R connect to sonoff L(out)
- Hardware installation should be complete and easily reversible, thanks to fuse block
    - Turn on breaker and see if Sauna is listed in your Wifi. Connect to it configure and enjoy
 <img src="https://github.com/SwiCago/HarviaWiFi/blob/master/images/wifi.png"/>

### New Circuit Diagram

<img src="https://github.com/SwiCago/HarviaWiFi/blob/master/images/schematic.png" style="max-width:300px"/>

### Original Circuit Diagram

<img src="https://github.com/SwiCago/HarviaWiFi/blob/master/images/schematic_original_alt.png" style="max-width:300px"/>

### Parts Required

- SONOFF TH16 (buy two as a spare)
    - https://www.amazon.com/dp/B06XTNSJ46/
- DS18B20 Waterproof Temperature Sensor
    - https://www.amazon.com/gp/product/B078NRBNM8/
- 2.5mm extension cable
    - https://www.amazon.com/gp/product/B00FJEH1PY/
- Fuse Block
    - https://www.amazon.com/gp/product/B0752QMXGC/
- 20 Amp ATM fuses
    - https://www.amazon.com/dp/B000CF7CRW/
  

## License

Licensed under the GNU Lesser General Public License.
https://www.gnu.org/licenses/lgpl.html
