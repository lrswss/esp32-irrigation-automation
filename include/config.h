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
//#define LANG_DE
#define LANG_EN

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
#define WIFI_STA_SSID "__your_ssid__"
#define WIFI_STA_PASSWORD "__secret__"
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
#define PUMP_AUTOSTOP_SECS 90

// if water level in reservoir falls below this
// level, the pump is switched of and blocked to
// avoid damaging the submersible pump
#define PUMP_MIN_WATERLEVEL_CM 4

// distance from ultra-sonice sensor to bottom of reservoir
// use to calculate the water level
#define WATER_RESERVOIR_HEIGHT 37

// relay names (max. 24 chars) and pin assigment
// set *_PIN to -1 to disable
#define RELAY_PINS "13,16,17,18,19,23,25,26,27"
#define RELAY1_PIN 19
#define RELAY1_LABEL "valve1"
#define RELAY2_PIN 18
#define RELAY2_LABEL "valve2"
#define RELAY3_PIN 17
#define RELAY3_LABEL "valve3"
#define RELAY4_PIN 16
#define RELAY4_LABEL "valve4"

// optional capacitive moisture sensors
// pins and labels (max. 24 chars) for moisture sensor on ADC1
// set *_PIN to -1 to disable
#define MOISTURE_PINS "32,33,34,36,39"
#define MOIST1_PIN 33
#define MOIST1_LABEL "moisture1"
#define MOIST2_PIN 34
#define MOIST2_LABEL "moisture2"
#define MOIST3_PIN 32
#define MOIST3_LABEL "moisture3"
#define MOIST4_PIN -1
#define MOIST4_LABEL "moisture4"

// ADC readings from capacitave moisture sensor v1.2 which mark the
// upper bound (sensor in air) and lower bound (sensor placed in water)
#define MOISTURE_VALUE_AIR 840
#define MOISTURE_VALUE_WATER 445

// wait a least given number of secs before triggering 
// relay again; meant to prevent accidental overwatering
#define RELAY_BLOCK_MINS 180

// I2C temperature and humidity sensor (optional)
#define HAS_HTU21D

// pins for HC-SR04 ultrasonic sensor (water level)
#define US_TRIGGER_PIN 12
#define US_ECHO_PIN 14

// log sensor reading to flash
#define ENABLE_LOGGING

// if plants haven't been watered for AUTO_IRRIGATION_PAUSE_HOURS trigger 
// irrigation (all valves) for AUTO_IRRIGATION_SECS at AUTO_IRRIGRATION_TIME
// Note: AUTO_IRRIGATION_DURATION_SECS must be less than PUMP_AUTOSTOP_SECS
#define ENABLE_AUTO_IRRIGRATION
#define AUTO_IRRIGRATION_TIME "06:30"   // HH:MM, 24h
#define AUTO_IRRIGATION_SECS 20
#define AUTO_IRRIGATION_PAUSE_HOURS 18

// time server
#define NTP_ADDRESS "de.pool.ntp.org"

// clear all preferences in NVS if firmware was updated
// must be set if structs for preferences were changed
#define CLEAR_NVS_FWUPDATE

// prints free heap in web ui
//#define DEBUG_MEMORY

#endif
