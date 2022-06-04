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

#include "config.h"
#include "web.h"
#include "prefs.h"
#include "utils.h"
#include "sensors.h"

// instantiate general settings and set default values
RTC_DATA_ATTR generalPrefs_t generalPrefs = {
    AP_TIMEOUT_SECS,
    WIFI_AP_PASSWORD,
    WIFI_STA_SSID,
    WIFI_STA_PASSWORD,
#ifdef MQTT_BROKER
    true,
#else
    false,
#endif
    MQTT_BROKER,
    MQTT_TOPIC_CMD,
    MQTT_TOPIC_STATE,
    MQTT_PUSH_INTERVAL_SECS,
#if defined(MQTT_USERNAME) && defined(MQTT_PASSWORD)
    MQTT_USERNAME,
    MQTT_PASSWORD,
    true,
#else
    "none",
    "none",
    false,
#endif
#ifdef CLEAR_NVS_FWUPDATE
    true
#else
    false
#endif
};

RTC_DATA_ATTR switchesPrefs_t switchesPrefs = {
    { RELAY1_PIN, RELAY2_PIN, RELAY3_PIN, RELAY4_PIN },
    { RELAY1_LABEL, RELAY2_LABEL, RELAY3_LABEL, RELAY4_LABEL},
    { MOIST1_PIN, MOIST2_PIN, MOIST3_PIN, MOIST4_PIN },
    { MOIST1_LABEL, MOIST2_LABEL, MOIST3_LABEL, MOIST4_LABEL},
    PUMP_PIN,
    PUMP_AUTOSTOP_SECS,
    RELAY_BLOCK_MINS,
#ifdef ENABLE_AUTO_IRRIGRATION
    true,
    AUTO_IRRIGRATION_TIME,
    { AUTO_IRRIGATION_SECS, AUTO_IRRIGATION_SECS, AUTO_IRRIGATION_SECS, AUTO_IRRIGATION_SECS },
    AUTO_IRRIGATION_PAUSE_HOURS,
#else
    false,
    "none",
    0,
    0,
#endif
#ifdef ENABLE_LOGGING
    true,
#else
    false,
#endif
    PUMP_MIN_WATERLEVEL_CM,
    false,
    WATER_RESERVOIR_HEIGHT,
    MOISTURE_VALUE_AIR,
    MOISTURE_VALUE_WATER,
    false,
    true
};

// use NVS to store settings to survive
// a system reset (cold start) or reflash
Preferences nvs;


// initialize NVS to (permanently) store system settings
void initPrefs() {
    nvs.begin("prefs", false);
}


// load system settings from NVS
void restorePrefs() {
    size_t prefSize;

    if (nvs.getBool("general")) {
        prefSize = nvs.getBytesLength("generalPrefs");
        byte bufGeneralPrefs[prefSize];
        nvs.getBytes("generalPrefs", bufGeneralPrefs, prefSize);
        memcpy(&generalPrefs, bufGeneralPrefs, prefSize);
        Serial.print("Restored general preferences (");
        Serial.print(prefSize);
        Serial.println(" bytes).");
    }
    
	if (nvs.getBool("switches")) {
        prefSize = nvs.getBytesLength("switchesPrefs");
        byte bufSwitchesPrefs[prefSize];
        nvs.getBytes("switchesPrefs", bufSwitchesPrefs, prefSize);
        memcpy(&switchesPrefs, bufSwitchesPrefs, prefSize);
        Serial.print("Restored switches preferences (");
        Serial.print(prefSize);
        Serial.println(" bytes).");
    }
}
