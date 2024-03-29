*Last update on 22/05/2021*

# Continuous Mandatory Ventilation

--------------------------------------

## Introduction

The idea of this project is to automize the speed regulation of Continuous Mandatory Ventilation (VMC) regarding a relative humidity. 

The built module makes these things:

- gets measurements from a humidity sensor, 

- receives commands from a simple HTTP server via wifi, 

- sends measurements to a database via wifi,

- controls the VMC relays regarding a measured humidity.

--------------------------------------

## Hardware

### Modules

```plantuml
@startuml

rectangle "7-pin Connector\n<img:https://tuppi.ovh/data_server_vmc/sources/img_connector_7p.png{scale=0.5}>" as M_CONN

rectangle "USB 5V" as M_USB

rectangle "BOX" as M_BOX {

    rectangle "433 MHz Transmitter\n<img:https://tuppi.ovh/data_server_vmc/sources/img_433mhz_tx.png{scale=0.5}>" as M_433MHZ

    rectangle "Power Supply" as M_POW

    rectangle "LED" as M_LED

    rectangle "ESP8266\n<img:https://tuppi.ovh/data_server_vmc/sources/img_esp8266.png{scale=0.5}>" as M_ESP8266

    rectangle "DHT22 Sensor\n<img:https://tuppi.ovh/data_server_vmc/sources/img_dht22.png{scale=0.5}>" as M_DHT22
}

M_ESP8266 --> M_DHT22
M_ESP8266 --> M_433MHZ
M_ESP8266 --> M_LED
M_CONN --> M_ESP8266
M_USB --> M_POW

@enduml
```

### Power Supply

A component used to make a 3V3 stable voltage is AMS1117-3.3:

```plantuml
@startuml
    rectangle "AMS1117-3.3 " as M_AMS
    note left
    1: Ground 
    2: Vout (with 0.1uF to GND)
    3: Vin (with 22uF to GND)
    end note
@enduml
```

If it's necessary to power the module from battery, you should see the MCP1702-3302 with a low quiescent current:

```plantuml
@startuml
    rectangle "MCP1702-3302 " as M_AMS
    note left
    1: Ground 
    2: Vin (with 22uF to GND)
    3: Vout (with 0.1uF to GND)
    end note
@enduml
```

A 3-pin connector is used to connect the input voltage with the board:

```plantuml
@startuml
    rectangle "3-pin Input" as M_5V
    note left
    1: Vin 
    2: GND
    3: Key
    end note
@enduml
```

### DHT22

To be completed.

### ESP8266

To be completed.

### 7-Pin Connector

Pin  &nbsp; &nbsp; &nbsp; | Name  &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; | Wire Color
------|------|------
1 | GND | Black
2 | RX | White
3 | Reserved\* | - 
4 | TX | Yellow
5 | Boot by GPIO0\*\* | Green 
6 | N.C. | -
7 | N.C. | -

*\* Reserved: Compatibility another project*  
*\*\* Boot by GPIO0: N.C. – Flash boot, GND – UART boot*  

### Boards Changelog

#### v2.0 - WIFI Connection & Debug Connector

Below you can find how looks like a pcb design of this version:

<img src="../sources/img_pcb_v2.png" width="400"/>

Changelog vs v1.0:

- Added a 7-pin connector. 

This board version is used for CMV speed regulation. It is supplied directly from a 220V USB adapter to not bother about battery connection and charging. A consumption power is about 0.3W.

#### v1.1 - Low Consumption Temperature Sensor

Changelog vs v1.0:

- Changed the supply component from AMS1117 to MCP1702.

- Added a 7-pin connector. 

- Integrated 9V battery directly in the box.

- Removed LED and 433 MHz transmitter.

This board version is used for an autonomous temperature / humidity measurements where a power consumption becomes critical. 

#### v1.0 - Initial Version without WIFI Connection

Initial board version.

### Extra Information

Some more information about hardware design can be found in [this document (PDF)](../sources/doc-hardware-design.pdf).

--------------------------------------

## Firmware

### Configuration File

It is necessary to create a `config.h` file with this content:
```c
#ifndef CONFIG_H
#define CONFIG_H

/* Wifi */
const char * CONFIG_WIFI_SSID = <your-wifi-ssid>;           
const char * CONFIG_WIFI_PASSWORD = <your-wifi-password>;  

/* DIO identification number of 26 bits, you can choose a random number */
const uint32_t CONFIG_DIO_ID = <int>;

/* deep sleep in sec after each humidity regulation */ 
const int32_t CONFIG_SLEEP_EACH_ITER_S = 30; 

/* number of deep sleeps before connection to wifi to download / upload information */
const int32_t CONFIG_FORCE_UPDATE_ITER_NB = 60; 

/* mysensors node id (see MySensors documentation for more information) */
const int32_t CONFIG_MYSENSORS_NODE_ID = <int>;

/* mysensors IP address */
const char * CONFIG_MYSENSORS_IP = "<int>.<int>.<int>.<int>";  

#endif
```

### Source Code 

Source code of this project: 

- [https://github.com/tuppi-ovh/data-server-vmc](https://github.com/tuppi-ovh/data-server-vmc)

- [https://github.com/tuppi-ovh/data-server-pi](https://github.com/tuppi-ovh/data-server-pi)

--------------------------------------

## Conclusion

The board is working already for a while. Below you can find how the board looks like in the real life:

<img src="../sources/img_result.jpg" width="400"/>

--------------------------------------

## Links

- [MCP1702 Datasheet (PDF)](http://ww1.microchip.com/downloads/en/devicedoc/22008e.pdf)

- [Arduino IDE Configuration (FRENCH)](https://www.fais-le-toi-meme.fr/fr/electronique/tutoriel/programmes-arduino-executes-sur-esp8266-arduino-ide)

- [Control DiO by Arduino](http://charleslabs.fr/fr/project-Contr%C3%B4le+de+prises+DiO+avec+Arduino)

- [ESP8266 Deep Sleep Mode](https://projetsdiy.fr/esp8266-test-mode-deep-sleep-reveil-wakeup-detecteur-mouvement-pir)

- [ExpressPCB (software used for PCB design)](https://www.expresspcb.com)
