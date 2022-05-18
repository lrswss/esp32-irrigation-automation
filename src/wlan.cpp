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

#include "wlan.h"
#include "logging.h"
#include "utils.h"
#include "rtc.h"
#include "prefs.h"

static bool wifiActive = false;
static bool wifiAP = false;
static bool wifiUplink = false;
static bool mdnsStarted = true;


static void wifi_mdns() {
#ifdef MDNS_NAME
    if (!mdnsStarted && !MDNS.begin(MDNS_NAME)) 
        Serial.println(F("WiFi: failed to setup MDNS responder!"));
    else
        mdnsStarted = true;  
#endif
}


static bool wifi_init() {
    if (wifiActive)
        return true;
    if (WiFi.mode(WIFI_STA)) {
        wifiActive = true;
    } else {
        Serial.print(millis());
        Serial.println(F(": WiFi failed!"));
        logMsg("wifi failed");
        delay(750);
    }
    delay(250);
    return wifiActive;
}


static bool wifi_start_ap(const char* ssid, const char* pass) {
    char buf[64], ap_ssid[32];

    if (wifiAP)
        return true;

    Serial.print(millis());
    if (!strlen(ssid)) {
        Serial.println(F(": WiFi: failed to start access point, no SSID set!"));
        return false;;
    } else if (strlen(pass) < 8) {
        Serial.println(F(": WiFi: failed to start access point, password too short!"));
        return false;
    } else if (!wifiActive && !wifi_init()) {
        return false;
    } else if (!WiFi.mode(WIFI_AP_STA)) {
        Serial.println(F(": WiFi: failed to set WIFI_AP_STA mode!"));
        return false;
    }

    sprintf(ap_ssid, "%s-%s", ssid, systemID().c_str());
    if (WiFi.softAP(ap_ssid, pass)) {
        Serial.printf(": WiFi: local AP with SSID %s, IP %s started\n",
            ap_ssid, WiFi.softAPIP().toString().c_str());
        sprintf(buf, "wifi ap %s", ap_ssid);
        logMsg(buf);
        wifi_mdns();
        wifiAP = true;
    } else {
        Serial.println(F(": WiFi: failed to start local AP!"));
        logMsg("wifi ap failed");
    }
    delay(1000);
    return wifiAP;
}


static bool wifi_start_sta(const char* ssid, const char* pass, uint8_t timeoutSecs) {
    uint8_t ticks = 0;
    char buf[64];

    Serial.print(millis());
    if (!strlen(ssid)) {
        Serial.println(F(": WiFi: failed to connect to station, no SSID set!"));
        return false;
    } else if (!wifiActive && !wifi_init())
        return false;

    if (wifiUplink && WiFi.status() == WL_CONNECTED)
        return true;

    Serial.printf(": WiFi: connecting to SSID %s...", ssid);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED && (ticks/2) <= timeoutSecs) {
        Serial.print(".");
        Serial.flush();
        delay(500);
        ticks++;
    }
    delay(500);
        
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("with IP %s.\n", WiFi.localIP().toString().c_str());
        sprintf(buf, "connect ssid %s, ip %s", ssid, WiFi.localIP().toString().c_str());
        logMsg(buf);
        wifi_mdns();
        wifiUplink = true;
    } else {
        Serial.println(F("failed!"));
        wifiUplink = false;
    }
    delay(1000);
    return wifiUplink;
}


// start or stop local access point
bool wifi_hotspot(bool activate) {
    if (activate) {
        return wifi_start_ap(WIFI_AP_SSID, generalPrefs.wifiApPassword);
    } else {
        if (wifiAP) {
            WiFi.softAPdisconnect(true);
            Serial.println(millis());
            Serial.println(F(": WiFi: Local AP stopped."));
            wifiAP = false;
        }
        return wifiAP;
    }
}


// check for WiFi uplink
// triggers new connection if reconnect is set
bool wifi_uplink(bool reconnect) {
    if (!reconnect)
        return (WiFi.status() == WL_CONNECTED);
    return wifi_start_sta(generalPrefs.wifiStaSSID, generalPrefs.wifiStaPassword, WIFI_STA_CONNECT_TIMEOUT);
}

void wifi_stop() {
    if (!wifiActive)
        return;

    stopNTPSync();
    if (mdnsStarted)
        MDNS.end();
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    delay(1);
    wifiActive = false;
    wifiUplink = false;
    wifiAP = false;
    Serial.println(millis());
    Serial.println(F(": WiFi stopped."));
    Serial.flush();
    logMsg("wifi stopped");
}