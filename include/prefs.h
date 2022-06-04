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

#ifndef _PREFS_H
#define _PREFS_H

#include <Arduino.h>
#include <Preferences.h>  // use NVS instead of EEPROM (depreciated on ESP32)

#define NUM_RELAY 4
#define NUM_MOISTURE_SENSORS 4

typedef struct  {
    uint16_t wifiAPT;
    char wifiApPassword[33];
    char wifiStaSSID[33];
    char wifiStaPassword[33];
    bool enableMQTT;
    char mqttBroker[65];
    char mqttTopicCmd[65];
    char mqttTopicState[65];
    uint16_t mqttPushInterval;
    char mqttUsername[33];
    char mqttPassword[33];
    bool mqttEnableAuth;
    bool clearNVSFwUpdate; // no switch in web ui
} generalPrefs_t;

typedef struct {
    int8_t pinRelay[NUM_RELAY];
    char labelRelay[NUM_RELAY][25];
    int8_t pinMoisture[NUM_MOISTURE_SENSORS];
    char labelMoisture[NUM_MOISTURE_SENSORS][25];
    uint8_t pinPump;
    uint16_t pumpAutoStopSecs;
    uint32_t relaysBlockMins;
    bool enableAutoIrrigation;
    char autoIrrigationTime[8];
    uint16_t autoIrrigationSecs[4];
    uint8_t autoIrrigationPauseHours;
    bool enableLogging;
    uint8_t minWaterLevel;
    bool ignoreWaterLevel;
    uint8_t waterReservoirHeight;
    uint16_t moistureMin;
    uint16_t moistureMax;
    bool moistureRaw;
    bool moistureMovingAvg;
} switchesPrefs_t;

extern Preferences nvs;
extern generalPrefs_t generalPrefs;
extern switchesPrefs_t switchesPrefs;

void initPrefs();
void restorePrefs();

#endif

