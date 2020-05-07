# Data Server VMC

## Introduction

Data Server VMC - Arduino application for VMC regulation. VMC is a french abbreviation of Continuous Mandatory Ventilation.

## Configuration File

It is necessary to create a `config.h` file with this content:
```c
/* Wifi */
const char * CONFIG_WIFI_SSID = "your-wifi-ssid";           
const char * CONFIG_WIFI_PASSWORD = "your-wifi-password";  

/* DIO identification number of 26 bits, you can choose a random number */
const uint32_t CONFIG_DIO_ID = 0x0000BEEF;

/* deep sleep in sec after each humidity regulation */ 
const int32_t CONFIG_SLEEP_EACH_ITER_S = 30; 

/* number of deep sleeps before connection to wifi to download / upload information */
const int32_t CONFIG_FORCE_UPDATE_ITER_NB = 60; 
```

## License

Refer to the LICENSE file.