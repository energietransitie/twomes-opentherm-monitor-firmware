# Twomes OpenTherm Monitor firmware
This repository contains the firmware and pointers to binary releases for the  Twomes OpenTherm Monitor. 

For the associated hardware design files for the OpenTherm Monitor Shield and enclosure and tips and instructions how to produce and assemble the hardware, please see the [twomes-opentherm-monitor-hardware](https://github.com/energietransitie/twomes-opentherm-monitor-hardware) repository. 

## Table of contents
* [General info](#general-info)
* [Deploying](#deploying)
* [Developing](#developing) 
* [Features](#features)
* [Status](#status)
* [License](#license)
* [Credits](#credits)

## General info
The OpenTherm Monitor should be connected via one wire pair to a [boiler that supports OpenTherm](https://www.otgw.tclcode.com/matrix.cgi#boilers) and via another wire pair to a [thermostat that supports OpenTherm](https://www.otgw.tclcode.com/matrix.cgi#thermostats). It sends the following properties to the Twomes server:

| OpenTherm ID | Part | Property                   | Unit | [Printf format](https://en.wikipedia.org/wiki/Printf_format_string) | Measurement interval \[h:mm:ss\] | Description               |
| ------------ | ---- | -------------------------- | ---- | ------------- | -------------------------------- | ------------------------- |
| 0            | LB/3 | `isBoilerFlameOn`          |      | 0/1           | 0:00:30                          | STATUS /Flame status      |
| 0            | LB/1 | `isCentralHeatingModeOn`   |      | 0/1           | 0:00:30                          | STATUS/CH mode            |
| 0            | LB/2 | `isDomesticHotWaterModeOn` |      | 0/1           | 0:00:30                          | STATUS/DHW mode           |
| 14           |      | `maxModulationLevel`       | %    | %d            | 0:00:30                          | CAPACITY SETTING          |
| 15           | HB   | `maxBoilerCap`             | kW   | %d            | 0:00:30                          | MAX CAPACITY              |
| 15           | LB   | `minModulationLevel`       | %    | %d            | 0:00:30                          | MIN-MOD-LEVEL             |
| 16           |      | `roomSetpointTemp`         | °C   | %.2f          | 0:05:00                          | ROOM SETPOINT             |
| 17           |      | `relativeModulationLevel`  | %    | %d            | 0:00:30                          | RELATIVE MODULATION LEVEL |
| 24           |      | `roomTemp`                 | °C   | %.2f          | 0:05:00                          | ROOM TEMPERATURE          |
| 57           |      | `boilerMaxSupplyTemp`      | °C   | %.2f          | 0:05:00                          | MAX CH WATER SETPOINT     |
| 25           |      | `boilerSupplyTemp`         | °C   | %.2f          | 0:00:10                          | BOILER WATER TEMP.        |
| 28           |      | `boilerReturnTemp`         | °C   | %.2f          | 0:00:10                          | RETURN WATER TEMPERATURE  |

This is device  is NOT an OpenTherm gateway; it only monitors OpenTherm traffic and it cannot insert OpenTherm commands to the boiler or thermostat.

## Deploying
This section describes how you can deploy binary releases of the firmware, i.e. without changing the source code, without a development environment and without needing to compile the source code.

### Prerequisites
In addition to the [prerequisites described in the generic firmware for Twomes measurement devices](https://github.com/energietransitie/twomes-generic-esp-firmware#prerequisites), you need:
* a [Twomes OpenTherm Monitor Shield](https://github.com/energietransitie/twomes-opentherm-monitor-hardware)

### Erasing all persistenly stored data
See [Deploying section of the generic firmware for Twomes measurement devices](https://github.com/energietransitie/twomes-generic-esp-firmware#deploying).

### Device preparation
See [Deploying section of the generic firmware for Twomes measurement devices](https://github.com/energietransitie/twomes-generic-esp-firmware#deploying).

### Erasing only Wi-Fi provisioning data
See [Deploying section of the generic firmware for Twomes measurement devices](https://github.com/energietransitie/twomes-generic-esp-firmware#deploying).

Specific for te OpenTherm Monitor::
* hold down the button mounted counterclockwise (viewed from above) compared to the connectors for more than 10 seconds;
* power down and power up the OpenTherm Monitor;
* start the Wi-Fi provisioning process again. 

## Developing
This section describes how you can change the source code using a development environment and compile the source code into a binary release of the firmware that can be depoyed, either via the development environment, or via the method described in the section Deploying.

## Features
List of features ready and TODOs for future development (other than the [features of the generic Twomes firmware](https://github.com/energietransitie/twomes-generic-esp-firmware#features)). 

Ready:
* Read and timestamp OpenTherm messages as indicated in the table above.
* Send the data collected to a Twomes server.
* Reset Wi-Fi provisioning by a long press (>10s) on the recessed button accessible through a pinhole in the encosure, mounted counterclockwise (viewed from above) from the connectors. 

To-do:
* Align indication of status and error via LEDs with other measurement devices

## Status
Project is: _in progress_

## License
This software is available under the [Apache 2.0 license](./LICENSE), Copyright 2021 [Research group Energy Transition, Windesheim University of Applied Sciences](https://windesheim.nl/energietransitie) 

## Credits
This software is a collaborative effort of :
* Kevin Jansen ·  [@KevinJan18](https://github.com/KevinJan18)
* Henri ter Hofte · [@henriterhofte](https://github.com/henriterhofte) · Twitter [@HeNRGi](https://twitter.com/HeNRGi)
* Marco Winkelman · [@MarcoW71](https://github.com/MarcoW71)

Thanks also go to:
* Arjen Korevaar ·  [@GArjenKorevaar](https://github.com/ArjenKorevaar)
* Sjors Smit ·  [@Shorts1999](https://github.com/Shorts1999)

We use and gratefully acknowlegde the efforts of the makers of the following source code and libraries:
* [OpenTherm Arduino/ESP8266 Library](https://github.com/ihormelnyk/opentherm_library/), by Ihor Melnyk, licensed under [MIT License](https://github.com/ihormelnyk/opentherm_library/blob/master/LICENSE)
