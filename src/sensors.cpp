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

#include "sensors.h"
#include "prefs.h"
#include "config.h"


#ifdef HAS_HTU21D
static HTU21D  htu21(HTU21D_RES_RH12_TEMP14);
static bool htu21Ready = false;
#endif
#if defined(US_TRIGGER_PIN) && defined(US_ECHO_PIN)
static UltraSonicDistanceSensor hcrs04(US_TRIGGER_PIN, US_ECHO_PIN);
#endif
sensorReadings_t sensors;


void initSensors() {
#ifdef HAS_HTU21D
    Serial.print(millis());
    if (!htu21.begin()) {
        Serial.println(F(": Sensor HTU21D not found!"));
    } else {
        Serial.printf(": Sensor HTU21D v%d found\n", htu21.readFirmwareVersion());
        htu21Ready = true;
    }
#endif
#if defined(US_TRIGGER_PIN) && defined(US_ECHO_PIN)
    sensors.waterLevel = -1;
#endif
}


void readTemp(bool verbose) {
#ifdef HAS_HTU21D
    if (htu21Ready) {
        sensors.temperature = htu21.readTemperature();
        sensors.humidity = (uint8_t)htu21.readCompensatedHumidity();
        Serial.print(millis());
        Serial.print(F(": Temperature: "));
        Serial.print(sensors.temperature, 2);
        Serial.printf(" °C, relative humidity %d %%", sensors.humidity);  
    }
#endif
}

void readWaterLevel(bool verbose) {
#if defined(US_TRIGGER_PIN) && defined(US_ECHO_PIN)
    static int16_t prevLevel = 0;
    static uint8_t errors = 0;
    int16_t level = int(hcrs04.measureDistanceCm());

    // first run at system startup
    if (!prevLevel) {
        delay(100);
        level = int(hcrs04.measureDistanceCm());
        prevLevel = level;
    }

    // try to avoid jumpy water level values due to invalid readings
    if (level <= 0 || (abs(prevLevel-level)*100/level) > 25 || level > (WATER_RESERVOIR_HEIGHT*1.1)) {
        if (errors++ > 3) {
            sensors.waterLevel = -1;
        }
    } else {
        sensors.waterLevel = switchesPrefs.waterReservoirHeight - level;
        errors = 0;       
    }
    prevLevel = level;

    if (verbose) {
        Serial.print(millis());
        if (sensors.waterLevel > 0)
            Serial.printf(": Water level: %d cm\n", sensors.waterLevel);
        else
            Serial.println(": WARNING: water level unknown!");
    }
#endif
}

