| Supported Targets | ESP32-P4-ETH |
| ----------------- | ------------ |

# RigExpert ShackMaster Power 600 
Control Software

## Overview

This project is designed run on a [WaveShare ESP32-P4-ETH](https://docs.waveshare.com/ESP32-P4-ETH)  board which is equipped with 2 USB ports.

* The main USB-C port is used for programming and debugging the board.
* The seconday USB port is USB 2 and requires an additional cable from the board with a USB A style socket.
* The ShackMaster 600 is connected to the USB 2 interfaced and acts as a HID device.
* The USB 2 port on the board is configured as a HID host.

## IDE
This project is develped in Visual Studio Code with Expressif ESP-IDF expension installed.

### Project configuration settings
Many project configuration items are defined the a single location and do not use the common `#define` pre-processor directive.  
In VSC, select `ESP-IDF Explorer` in the left hand menu, then click on `SDK Configuration Editor (menuconfig)`

## Ethernet connection
The ethernet interface is set as a DHCP client.  
The terminal output print the IP address assigned to the board

## User interaction
Once the assigned IP address is known, the user can navigate to http://x.x.x.x/ to access the web interface.

## Hardware
#### Board power supply options
The USB ports of the ShackMaster 600 do not provide power while the device is off, so it can't be used to supply power to the P4 board.

* A POE power module is available to plug direclty onto the ESP32-P4-ETH board H7 pin header.
* The USB0 [UART] port can power the board.
* The VBUS pin (40) is connected directly to the USB 5V supply. **Note: do not connect external power via VBUS, it can damage the device connected to USB0.**
* VSYS is designed for external supply and protects USB0 by a Schottky diode.
* The 5V pin of the USB1 port (OTG 2.0 - 4pin connector J3) is connected to VSYS and can also power the board.
* External voltage on VSYS must not exceed 5.5V (thermal limit of of the onboard regulator).