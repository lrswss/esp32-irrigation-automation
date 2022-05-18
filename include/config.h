/***************************************************************************
  Copyright (c) 2021-2022 Lars Wessels

  This file a part of the "ESP32-Irrigation-Automation" source code.
  https://github.com/lrswss/esp32-irrigation-automation

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
   
  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

***************************************************************************/

#ifndef _CONFIG_H
#define _CONFIG_H

// choose language for web interface
#define LANG_DE
//#define LANG_EN // XXX TODO

//
// All settings except DEBUG options can
// also be changed in the web interface
//

// timeout for local access point
#define AP_TIMEOUT_SECS 120

// base name and credentials for local access point (min. 8 characters!)
#define WIFI_AP_SSID "Irrigation-System"
#define WIFI_AP_PASSWORD "__secret__"

// start local AP if wifi is not available for given number of seconds
#define WIFI_AP_FALLBACK_TIMEOUT 120

// credentials for internet connection (wifi uplink)
#define WIFI_STA_SSID "***REMOVED***"
#define WIFI_STA_PASSWORD "***REMOVED***"
#define WIFI_STA_CONNECT_TIMEOUT 5
#define WIFI_STA_RECONNECT_TIMEOUT 60

// peridocally publish readings with MQTT
// if a WiFi uplink is preset/available
#define MQTT_BROKER "192.168.10.66"
#define MQTT_TOPIC_CMD "irrigation/cmd"
#define MQTT_TOPIC_STATE "irrigation/state"
#define MQTT_PUSH_INTERVAL_SECS 60
//#define MQTT_USERNAME "admin"
//#define MQTT_PASSWORD "secret"

// relay pin for water pump
#define PUMP_PIN 4

// pump will stop if active for more then given
// number of settings; meant as a upper limit
// to avoid accidental overwatering
#define PUMP_AUTOSTOP_SECS 30

// if water level in reservoir falls below this
// level, the pump is switched of and blocked to
// avoid damaging the submersible pump
#define PUMP_MIN_WATERLEVEL_CM 4

// distance from ultra-sonice sensor to bottom of reservoir
// use to calculate the water level
#define WATER_RESERVOIR_HEIGHT 37

// relais name and pin assigment
// set *_PIN to -1 to disable
#define RELAIS_PINS "13,16,17,18,19,23,25,26,27"
#define RELAIS1_PIN 19
#define RELAIS1_LABEL "valve1"
#define RELAIS2_PIN 18
#define RELAIS2_LABEL "valve2"
#define RELAIS3_PIN 17
#define RELAIS3_LABEL "valve3"
#define RELAIS4_PIN 16
#define RELAIS4_LABEL "valve4"

// pins for moisture sensor (ADC1) // XXX TODO
#define MOISTURE_PIN "32,33,34,36,39"

// wait a least given number of secs before triggering relais again
// meant to prevent accidental overwatering
#define RELAIS_BLOCK_SECS 900

// I2C temperature and humidity sensor (optional)
#define HAS_HTU21D

// pins for HC-SR04 ultrasonic sensor (water level)
#define US_TRIGGER_PIN 12
#define US_ECHO_PIN 14

// log sensor reading to flash
#define ENABLE_LOGGING

// time server
#define NTP_ADDRESS "de.pool.ntp.org"

// clear all preferences in NVS if firmware was updated
// must be set if structs for preferences were changed
#define CLEAR_NVS_FWUPDATE

#endif
