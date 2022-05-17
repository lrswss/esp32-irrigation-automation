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
#include "logging.h"
#include "wlan.h"
#include "utils.h"
#include "rtc.h"
#include "mqtt.h"
#include "prefs.h"
#include "sensors.h"
#include "relais.h"
#include "web.h"

void setup() {
    char logmsg[96];

    // init watchdog with 90 sec. timeout
    // log rotation (rename file) sometimes take about a minute
    esp_task_wdt_init(90, true); 
    esp_task_wdt_add(NULL);

    btStop();
    initPrefs();
    initRelais();
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.printf("%s (v%d)\n", "ESP32-Irrigation-Automation", FIRMWARE_VERSION);
    runmode = bootMsg();

    // normal power or full system reset
    if (runmode >= POWERUP) {
        sprintf(logmsg, "start,%s,v%d", runmodes[runmode], FIRMWARE_VERSION);
        strcat(logmsg, checkFirmwareUpdate());
        restorePrefs();

    // soft restart triggered by external reset button
    } else if (runmode == RESTART) {
        sprintf(logmsg, "restart,%s,v%d", runmodes[runmode], FIRMWARE_VERSION);
    }

    initLogging();    
    logMsg(logmsg);
    
    initSensors();
    #ifdef HAS_HTU21D
    readTemp(true); 
    #endif
    #if defined(US_TRIGGER_PIN) && defined(US_ECHO_PIN)
    readWaterLevel(true);
    #endif

    // check wifi uplink
    // if not available start AP
    if (!wifi_uplink(true))
        wifi_hotspot(true);
    else
        startNTPSync();

    webserver_start();

#ifdef DEBUG_MEMORY
    free_heap();
#endif
}


void loop() {
    static uint32_t prevLoopTimer = 0;
    static uint32_t prevMqttPublish = 0;
    static uint32_t wifiRetry = WIFI_STA_RECONNECT_TIMEOUT;
    static uint16_t wifiOffline = 0;
#ifdef DEBUG_MEMORY
    static char logmsg[32];
#endif

    // run tasks once every second
    if (millis() - prevLoopTimer >= 1000) { 
        prevLoopTimer = millis();
        busyTime += 1;

        // check for wifi uplink, try to reconnect every 60 secs.
        if (!wifi_uplink(false)) {
            if (!wifiRetry || wifiRetry <= busyTime) {
                wifiRetry = busyTime + WIFI_STA_RECONNECT_TIMEOUT;
                if (wifi_uplink(true))
                    startNTPSync();

            } else if (!(busyTime % 10)) {
                wifiOffline += 10;
                Serial.print(millis());
                Serial.print(F(": WiFi: Uplink down, next retry in "));
                Serial.print(wifiRetry - busyTime);
                Serial.println(F(" seconds."));

                // Switch on local AP as fallback if wifi 
                // is down for a longer period of time
                if (wifiOffline >= WIFI_AP_FALLBACK_TIMEOUT)
                    wifi_hotspot(true);    
            }

        } else {
            wifiRetry = 0;
            wifiOffline = 0;

            // publish current sensor readings
            if (millis() - prevMqttPublish >= (generalPrefs.mqttPushInterval * 1000)) {
                prevMqttPublish = millis();
                mqtt_send(MQTT_TIMEOUT_MS);
            }

            // check for remote commands
            if (mqtt_connect(MQTT_TIMEOUT_MS))
                mqtt.loop();

            // sync RTC, check for log rotation
            if (!strcmp("04:30:00", getTimeString(true))) {
                startNTPSync();
                rotateLogs();  // blocking...
            }
        }

        // stop local AP after timeout if connection to wifi is available
        if (!(busyTime % 10) && busyTime >= AP_TIMEOUT_SECS && wifi_uplink(false))
            wifi_hotspot(false);

        // read sensors
        if (!(busyTime % 20)) {
            readTemp(false);
            readWaterLevel(true);
        }

        unblockRelais();
        pumpAutoStop();

#ifdef DEBUG_MEMORY
        if (!(busyTime % 300))
            free_heap();
#endif
    }

    webserver.handleClient(); // handle webserver requests
    esp_task_wdt_reset(); // feed the dog...
}
