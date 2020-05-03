/**
 * Data Server VMC - Arduino application for VMC regulation.
 * Copyright (C) 2020 Vadim MUKHTAROV
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 * For information on Data Server VMC: tuppi.ovh@gmail.com
 */

/* 
 * To add ESP board:
 * 1. File > Preferences > Additional Boards URL > http://arduino.esp8266.com/staging/package_esp8266com_index.json
 * 1.1. If the link doesn't work, try https://github.com/esp8266/Arduino/releases/download/2.3.0/package_esp8266com_index.json
 * 2. Tools > Board > Boards Manager > esp8266
 * 3. Tools > Board > Generic ESP8266 Module
 */
//#include <ESP.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

/* 
 * To install DiOremote library: Sketch > Include Library > Add ZIP library "DiOremote"
 */
#include "DiOremote.h"

/* 
 * To install DHT library: Sketch > Include Library > Manage Libraries…  
 * Enter “dht” in the search field and look through the list for “DHT sensor library by Adafruit".
 * 
 * To install Adafruit_Sensor library: Sketch > Include Library > Add ZIP library "Adafruit_Sensor-master"
 */
#include "DHT.h"

/*
 * Configuration file
 */ 
#include "config.h"

/*
 * Other libs.
 */
#include <stdio.h>
#include <string.h>

extern "C" {
  #include "user_interface.h"
}


/* GPIO pins */
const int32_t GPIO_LED = 5;
const int32_t GPIO_RF433_DATA = 14; 
const int32_t GPIO_DHT22_DATA = 12;
const int32_t GPIO_DHT22_FREE = 13;

/* DIO codes */
const uint32_t DIO_GRP = 0;
const uint32_t DIO_ON = 1;
const uint32_t DIO_OFF = 0;
const uint32_t DIO_SUBID_0 = 0;
const uint32_t DIO_SUBID_1 = 1;
const uint32_t DIO_SUBID_2 = 2;
const uint32_t DIO_RELAY_I_ON_CODE    = (CONFIG_DIO_ID << 6) | (DIO_GRP << 5) | (DIO_ON  << 4) | (DIO_SUBID_0 << 0);
const uint32_t DIO_RELAY_I_OFF_CODE   = (CONFIG_DIO_ID << 6) | (DIO_GRP << 5) | (DIO_OFF << 4) | (DIO_SUBID_0 << 0);
const uint32_t DIO_RELAY_II_ON_CODE   = (CONFIG_DIO_ID << 6) | (DIO_GRP << 5) | (DIO_ON  << 4) | (DIO_SUBID_1 << 0);
const uint32_t DIO_RELAY_II_OFF_CODE  = (CONFIG_DIO_ID << 6) | (DIO_GRP << 5) | (DIO_OFF << 4) | (DIO_SUBID_1 << 0);
const uint32_t DIO_RELAY_III_ON_CODE  = (CONFIG_DIO_ID << 6) | (DIO_GRP << 5) | (DIO_ON  << 4) | (DIO_SUBID_2 << 0);
const uint32_t DIO_RELAY_III_OFF_CODE = (CONFIG_DIO_ID << 6) | (DIO_GRP << 5) | (DIO_OFF << 4) | (DIO_SUBID_2 << 0);

/* Humidity Limits */
const int32_t HUM_SPEED_HIGH = 80;
const int32_t HUM_SPEED_LOW = 60;
const int32_t HUM_HYSTERESIS = 5;

/* classes */
DiOremote myRemote = DiOremote(GPIO_RF433_DATA);
DHT dht(GPIO_DHT22_DATA, DHT22);
ESP8266WiFiMulti WiFiMulti;

/* structure to save vmc data */
struct DATA_Struct
{
  uint32_t crc32;
  int32_t relay_speed_high;
  int32_t relay_speed_low;
  int32_t force_update_counter;
  int32_t hum_speed_high;
  int32_t hum_speed_low;
  int32_t hum_hysteresis;
};
static struct DATA_Struct rtcData;

/* buffers */
char buf_str[100];

/**
 * 
 */
static uint32_t calculateCRC32(const uint8_t * data, size_t length)
{
  uint32_t crc = 0xffffffff;
  while (length--) {
    uint8_t c = *data++;
    for (uint32_t i = 0x80; i > 0; i >>= 1) {
      bool bit = crc & 0x80000000;
      if (c & i) {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) {
        crc ^= 0x04c11db7;
      }
    }
  }

  return crc;
}


/**
 * 
 */
static void write_mem_and_deep_sleep(const int32_t delay_s)
{
  /* checksum  */
  rtcData.crc32 = calculateCRC32(((uint8_t*) &rtcData) + 4, sizeof(rtcData) - 4);

  /* write */
  (void)ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData));
  
  /* sleep before next measurement */
  ESP.deepSleep(delay_s * 1000000, RF_DISABLED);
}

/**
 * 
 */
static void led_blink(const int32_t delay_ms, const int32_t times)
{
  int32_t i;
  for (i = 0; i < times; i++)
  {
    /* switch on LED */
    digitalWrite(GPIO_LED, LOW); 
    /* delay */
    delay(delay_ms);
    /* switch off LED */
    digitalWrite(GPIO_LED, HIGH); 
    /* delay */
    if (times > 1)
    {
      delay(200);
    }
  }
}

/**
 * 
 */
void check_force_update(int32_t temper_x10, int32_t hum_x10)
{
  if (rtcData.force_update_counter >= CONFIG_FORCE_UPDATE_ITER_NB)
  {
    int32_t i;

    /* wifi turn on */
    WiFi.forceSleepWake(); 
    Serial.println("");
    
    /* try to connect to wifi during 60 sec */
    for (i = 0; i < 60; i++)
    {
      /* if connected */
      if ((WiFiMulti.run() == WL_CONNECTED)) 
      {
        HTTPClient http;
        int32_t http_begin;

        /* log IP address */
        Serial.print("wifi connected: ");
        Serial.println(WiFi.localIP());

        /* upload temperature */
        snprintf(buf_str, sizeof(buf_str), "http://192.168.8.100/cgi-bin/cgi_cmd.py?command=db.add.temper.104.%d&chat_id=-1", temper_x10);
        Serial.println(buf_str);
        http_begin = http.begin(buf_str);
        if (http_begin) 
        { 
          const int32_t http_get = http.GET();
          {
            Serial.println("wifi upload temper get error:");
            Serial.println(http_get);
          } 
        }
        else
        {
          Serial.println("wifi upload temper begin error:");
          Serial.println(http_begin);
        }

        /* upload humidity */
        snprintf(buf_str, sizeof(buf_str), "http://192.168.8.100/cgi-bin/cgi_cmd.py?command=db.add.hum.104.%d&chat_id=-1", hum_x10);
        Serial.println(buf_str);
        http_begin = http.begin(buf_str);
        if (http_begin) 
        { 
          const int32_t http_get = http.GET();
          {
            Serial.println("wifi upload hum get error:");
            Serial.println(http_get);
          } 
        }
        else
        {
          Serial.println("wifi upload hum begin error:");
          Serial.println(http_begin);
        }

        /* try to read http */
        if (http.begin("http://192.168.8.100/cmd_vmc.txt")) 
        { 
          if (http.GET() == 200)
          {
            int32_t value[4] = { -1, -1, -1, -1 };
            int32_t getSize = http.getSize();
            String payload = http.getString();

            /* copy to buffer for handling */
            strncpy(buf_str, payload.c_str(), sizeof(buf_str));

            /* find first int */
            char * str = strtok(buf_str, ",");
            for (int32_t i = 0; i < 4; i++)
            {
              /* exit if no delimited string */
              if (str == NULL)
              {
                break;
              }
 
              /* convert to int */
              value[i] = atoi(str);

              /* Find the next int input string */
              str = strtok(NULL, ",");
            }

            /* update structure is checksum ok */
            if (value[3] == (value[0] + value[1] + value[2]))
            {
              rtcData.hum_speed_high = value[1];
              rtcData.hum_speed_low = value[0];
              rtcData.hum_hysteresis = value[2];

              Serial.println("wifi http updated (high / low / hysteresis):");
              Serial.println(rtcData.hum_speed_high);
              Serial.println(rtcData.hum_speed_low);
              Serial.println(rtcData.hum_hysteresis);
              //Serial.println(res);
            }
            else
            {
              Serial.println("wifi http checksum error");
            }
          }
          else
          {
            Serial.println("wifi http get error");
          }

          /* stop http */
          http.end();
        }
        else
        {
          Serial.println("wifi http begin error");
        }

        /* disconnect */
        WiFi.disconnect();
        Serial.println("wifi disconnected");
        
        /* exit loop */
        break;
      }

      /* sleep before new iteration */
      Serial.print(".");
      delay(1000);
    }

    /* wifi turn off */
    WiFi.forceSleepBegin(); 
    Serial.println("");

    /* reset counter to force update */
    rtcData.force_update_counter = 0;
  }
  else
  {
    /* increment counter */
    rtcData.force_update_counter++;
  }
}

/**
 * Regulates humidity and activates relays if required.
 * 
 * @param h input humidity.
 * 
 * @return void
 */
static void humidity_regulate(const int32_t h)
{
  int32_t update = 0;

  /* high speed status */ 
  if (rtcData.relay_speed_high == 1)
  {
    if (h < (rtcData.hum_speed_high - rtcData.hum_hysteresis))
    {
      rtcData.relay_speed_high = 0;
      rtcData.relay_speed_low = 1;
      update = 1;
    }
  }
  else if (rtcData.relay_speed_high == 0)
  {
    if (h > rtcData.hum_speed_high)
    {
      rtcData.relay_speed_high = 1;
      rtcData.relay_speed_low = 0;
      update = 1;
    }
  }
  else
  {
    rtcData.relay_speed_high = 0;
    rtcData.relay_speed_low = 0;
    update = 1;
  }
  

  /* low speed status */
  if (rtcData.relay_speed_high == 1)
  {
    /* do nothing */
  }
  else if (rtcData.relay_speed_low == 1)
  {
    if (h < (rtcData.hum_speed_low - rtcData.hum_hysteresis))
    {
      /* relays values */
      rtcData.relay_speed_high = 0;
      rtcData.relay_speed_low = 0;
      update = 1;
    }
  }
  else if (rtcData.relay_speed_low == 0)
  {
    if (h > rtcData.hum_speed_low)
    {
      /* relays values */
      rtcData.relay_speed_high = 0;
      rtcData.relay_speed_low = 1;
      update = 1;
    }
  }
  else
  {
    rtcData.relay_speed_high = 0;
    rtcData.relay_speed_low = 0;
    update = 1;
  }
  
  /* update relays */
  if ((update == 1) || (rtcData.force_update_counter == 0))
  {
    /* relays */
    myRemote.send((rtcData.relay_speed_low  == 1) ? DIO_RELAY_II_ON_CODE : DIO_RELAY_II_OFF_CODE);
    myRemote.send((rtcData.relay_speed_high == 1) ? DIO_RELAY_I_ON_CODE  : DIO_RELAY_I_OFF_CODE);

    /* log */
    Serial.println("Relays (low / high):"); 
    Serial.println(rtcData.relay_speed_low);
    Serial.println(rtcData.relay_speed_high);
    Serial.println("");

    /* led */
    led_blink(10, 3);
  }
}

/**
 * Initialization.
 */ 
void setup(void)
{
  /* serial init */
  Serial.begin(115200);

  const bool memread = ESP.rtcUserMemoryRead(0, (uint32_t*) &rtcData, sizeof(rtcData));
  const uint32_t crcOfData = calculateCRC32(((uint8_t*) &rtcData) + 4, sizeof(rtcData) - 4);

  Serial.println("");

  Serial.println("RTC memory (result / crc-calc / crc-read):"); 
  Serial.println(memread);
  Serial.println(crcOfData);
  Serial.println(rtcData.crc32);
  Serial.println("");

  /* init RTC memory */
  if ( (memread != true) || (crcOfData != rtcData.crc32) )
  {
    /* structure init (to force update) */
    rtcData.relay_speed_high = -1;
    rtcData.relay_speed_low = -1;
    rtcData.force_update_counter = 0;
    rtcData.hum_speed_high = HUM_SPEED_HIGH;
    rtcData.hum_speed_low = HUM_SPEED_LOW;
    rtcData.hum_hysteresis = HUM_HYSTERESIS;
  }

  /* wifi init */
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);
  WiFi.forceSleepBegin(); 

  /* Enable light sleep */
  wifi_set_sleep_type(LIGHT_SLEEP_T);

  /* GPIO init */
  pinMode(GPIO_LED, OUTPUT);     
  
  /* DHT22 init */
  dht.begin();
}

/**
 * Loop.
 */  
void loop(void)
{
  /* led */
  led_blink(10, 1);

  /* read humidity & temperature */
  const int32_t h = (int32_t)(dht.readHumidity());
  const int32_t h_x10 = h * 10;
  const int32_t t_x10 = (int32_t)(dht.readTemperature() * 10.0f);

  /* log */
  Serial.println("Measurements (humidity / temperature) x10:"); 
  Serial.println(h_x10);
  Serial.println(t_x10);
  Serial.println("");

  /* check force update */
  check_force_update(t_x10, h_x10);

  /* humidity regulation */
  humidity_regulate(h);

  /* sleep */
  //delay(CONFIG_SLEEP_EACH_ITER_S * 1000); 
  write_mem_and_deep_sleep(CONFIG_SLEEP_EACH_ITER_S);
}


