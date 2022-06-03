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

#ifndef _SENSORS_H
#define _SENSORS_H

#include <Arduino.h>
#include <HTU21D.h>
#include <HCSR04.h>
#include <driver/adc.h>

#define MOISTURE_MA_WINDOW_SIZE 5

typedef struct  {
    float temperature;
    uint8_t humidity;
    int16_t waterLevel;
    int16_t moisture[4];
} sensorReadings_t;

extern sensorReadings_t sensors;

void initSensors();
void readTemp(bool verbose, bool log);
void readWaterLevel(bool verbose, bool log);
void readMoisture(bool verbose, bool log, bool reset);

#endif
