# Twomes OpenTherm Monitor firmware
This repository contains the firmware and pointerr to binary releases for the  Twomes OpenTherm Monitor. 

For the associated hardware design files for the OpenTherm Monitor Shield and enclosure and tips and instructions how to produce and assemble the hardware, please see the [twomes-opentherm-monitor-hardware](https://github.com/energietransitie/twomes-opentherm-monitor-hardware) repository. 

## Table of contents
* [General info](#general-info)
* [Prerequisites](#prerequisites)
* [Deploying](#deploying)
* [Developing](#developing) 
* [Features](#features)
* [Status](#status)
* [License](#license)
* [Credits](#credits)

## General info
The OpenTherm Monitor should be connected via one wire pair to a [boiler that supports OpenTherm](https://www.otgw.tclcode.com/matrix.cgi#boilers) and via another wire pair to a [thermostat that supports OpenTherm](https://www.otgw.tclcode.com/matrix.cgi#thermostats). 

This is device  is NOT an OpenTherm gateway; it only monitors OpenTherm traffic and it cannot insert OpenTherm commands to the boiler or thermostat.


## Prerequisites
In addition to the [prerequisites described in the generic firmware for Twomes measurement devices](https://github.com/energietransitie/twomes-generic-esp-firmware#prerequisites), you need:
* a [Twomes OpenTherm Monitor Shield](https://github.com/energietransitie/twomes-opentherm-monitor-hardware)

## Deploying
See [Deploying section of the generic firmware for Twomes measurement devices](https://github.com/energietransitie/twomes-generic-esp-firmware#deploying).


## Developing

### Prerequisites for developing
NOTE: This section is still under (re)construction...
Prerequisites for deploying, plus:
*	[Visual Studio Code](https://code.visualstudio.com/download) installed
*	[PlatformIO for Visual Studio Code](https://platformio.org/install/ide?install=vscode) installed
*	This GitHub repository cloned
*	[ESP-IDF plugin for Visual Studio Code](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension) installed; make sure you satisfy all prerequisites, which may include
  * the GitHub reposotory [ESP-IDF](https://github.com/espressif/esp-idf.git) cloned 
  * In Visual Studio Code, this opens the ESP-IDF setup window
  * For `Select ESP-IDF version:` choose the oprion `Find ESP-IDF in your system`;
  * For `Enter ESP-IDF directory (IDF_PATH)` navitate to `%userprofile%\.platformio\platforms\espressif32`,(where %userprofile% is your home directory, which may not have spaces in its path)
  * Leave the `Enter ESP-IDF Tools directory (IDF_TOOLS_PATH)` as is.

Old text in this section was: "A good tip is also to use the ESP-IDF VSCode plugin for building the main branch. Not needed for OpenTherm-Port branch: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/vscode-setup.html."

## Features
List of features ready and TODOs for future development. Ready:

* awesome feature 1;
* awesome feature 2;
* awesome feature 3.

To-do:

* wow improvement to be done 1;
* wow improvement to be done 2.

## Status
Project is: _in progress_

## License
This software is available under the [Apache 2.0 license](./LICENSE), Copyright 2021 [Research group Energy Transition, Windesheim University of Applied Sciences](https://windesheim.nl/energietransitie) 

## Credits
This software is a collaborative effort the following students and researchers:
* <contributor name 1> ·  [@Github_handle_1](https://github.com/<github_handle_1>) ·  Twitter [@Twitter_handle_1](https://twitter.com/<twitter_handle_1>)
* <contributor name 2> ·  [@Github_handle_2](https://github.com/<github_handle_2>) ·  Twitter [@Twitter_handle_2](https://twitter.com/<twitter_handle_2>)
* <contributor name 3> ·  [@Github_handle_3](https://github.com/<github_handle_3>) ·  Twitter [@Twitter_handle_3](https://twitter.com/<twitter_handle_3>)
* etc. 


We use and gratefully aknowlegde the efforts of the makers of the following source code and libraries:

* [Arduino](https://github.com/arduino/Arduino), by the Arduino team, licensed under [GNU LGPL v2.1](https://github.com/arduino/Arduino/blob/master/license.txt)
* [OpenTherm Arduino/ESP8266 Library](https://github.com/ihormelnyk/opentherm_library/), by Ihor Melnyk, licensed under [MIT License](https://github.com/ihormelnyk/opentherm_library/blob/master/LICENSE)
