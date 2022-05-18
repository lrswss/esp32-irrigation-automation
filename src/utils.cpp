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
#include "utils.h"
#include "logging.h"
#include "prefs.h"
#include "wlan.h"
#include "mqtt.h"
#include "rtc.h"

rstcodes runmode;
char runmodes[7][10] = {
    "powerup",
    "restart",
    "softreset",
    "exception",
    "watchdog",
    "brownout",
    "other"
};


// short deep sleep triggers a soft 
// reboot and thus keeps RTC data
void restartSystem() {
    Serial.print(millis());
    Serial.println(F(": Restarting system..."));
    delay(1000);
    nvs.end();
    Serial.flush();
    esp_sleep_enable_timer_wakeup(1000000);
    esp_deep_sleep_start();
}


// reset system (RTC data will be lost)
void resetSystem() {
    Serial.print(millis());
    Serial.println(F(": System reset..."));
    Serial.flush();
    mqtt.disconnect();
    nvs.end();
    delay(500);
    ESP.restart();
}


// returns hardware system id (last 3 bytes of mac address)
String systemID() {
    uint8_t mac[6];
    char sysid[7];
    
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    sprintf(sysid, "%02X%02X%02X", mac[3], mac[4], mac[5]);
    return String(sysid);
}


// check if a new firmware has just been flashed
char* checkFirmwareUpdate() {
    uint8_t sha256[32], sha256Prev[32];
    static char logmsg[32];

    memset(logmsg, 0, sizeof(logmsg));
    if (esp_partition_get_sha256(esp_ota_get_running_partition(), sha256) == ESP_OK) {
        nvs.getBytes("sha256", sha256Prev, sizeof(sha256Prev));
        Serial.print(F("SHA256: "));
        printHEX8bit(sha256Prev, sizeof(sha256Prev), true, false);
        if (memcmp(sha256Prev, sha256, sizeof(sha256Prev))) {
            Serial.print(F("New firmware detected"));
            if (generalPrefs.clearNVSFwUpdate) {
                Serial.print(F(", resetting preferences in NVS!"));
                sprintf(logmsg, ",firmware-update,clearnvs");
                nvs.clear();
            } else {
                sprintf(logmsg, ",firmware-update");
            }
            nvs.putBytes("sha256", sha256, sizeof(sha256));
            Serial.println();
        }
    }
    return logmsg;
}


// https://esphome.io/api/debug__component_8cpp_source.html
rstcodes bootMsg() {
    Serial.print(F("ESP SDK Version: "));
    Serial.println(ESP.getSdkVersion());
    Serial.print(F("CPU Frequency (MHz): "));
    Serial.println(ESP.getCpuFreqMHz());
    
    switch (esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println(F("Wake up by reset button"));
            return RESTART;
        default:
            break;
    }

    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:
            Serial.println(F("System powerup"));
            return POWERUP;
        case ESP_RST_SW:
            Serial.println(F("Soft system reset"));
            return SOFTRESET;
        case ESP_RST_PANIC:
            Serial.println(F("WARNING: Exception cause system reset"));
            return EXCEPTION;
        case ESP_RST_TASK_WDT:
        case ESP_RST_WDT:
            Serial.println(F("WARNING: Watchdog cause system reset"));
            return WATCHDOG;
        case ESP_RST_BROWNOUT:
            Serial.println(F("WARNING: Brownout caused system reset"));
            return BROWNOUT;
        default:
            return OTHER;
    }
}


// turn hex byte array of given length into a null-terminated hex string
void array2string(const byte *arr, int len, char *buf, bool reverse) {
    for (int i = 0; i < len; i++)
        sprintf(buf + i * 2, "%02X", reverse ? arr[len-1-i]: arr[i]);
}


// print byte array as hex string, optionally masking last 4 bytes
void printHEX8bit(uint8_t *arr, uint8_t len, bool ln, bool reverse) {
    char hex[len * 2 + 1];
    array2string(arr, len, hex, reverse);
    Serial.print(hex);
    if (ln)
        Serial.println();
}


