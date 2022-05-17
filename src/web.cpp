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


#include "web.h"
#include "config.h"
#include "logging.h"
#include "utils.h"
#include "rtc.h"
#include "wlan.h"
#include "relais.h"
#include "mqtt.h"
#include "sensors.h"
#include "prefs.h"

#ifdef LANG_DE
#include "html_DE.h"
#else
#include "html_EN.h"
#endif

static uint32_t webserverRequestMillis = 0;
static uint16_t webserverTimeout = 0;

WebServer webserver(80);


// pass sensor readings, system status to web ui as JSON
static void updateUI() {
    static char buf[128];
    static StaticJsonDocument<256> JSON;

    memset(buf, 0, sizeof(buf));
    JSON.clear();
    JSON["logging"] = generalPrefs.enableLogging ? 1 : 0;
    if (getLocalTime() > 1609455600) {  // RTC already set?
        JSON["date"] = getDateString();
        JSON["time"] = getTimeString(false);
        JSON["tz"] = getTimeZone();
    }
    JSON["runtime"] = getRuntime(busyTime);
    JSON["wifi"] = wifi_uplink(false) ? 1 : 0;
#ifdef HAS_HTU21D
    char temp[8];
    dtostrf(sensors.temperature, 4, 1, temp);
    JSON["temp"] = temp;
    JSON["hum"] = sensors.humidity;
#endif
#if defined(US_TRIGGER_PIN) && defined(US_ECHO_PIN)
    JSON["level"] = sensors.waterLevel;
#endif
#ifdef DEBUG_MEMORY
    JSON["heap"] = ESP.getFreeHeap();
#endif

    if (serializeJson(JSON, buf) > 16)
        webserver.send(200, F("application/json"), buf);
    else
        webserver.send(500, "text/plan", "ERR");
}


void webserver_start() {

    // send main page
    webserver.on("/", HTTP_GET, []() {
        String html = FPSTR(HEADER_html);
        html += FPSTR(ROOT_html);
        html.replace("__SYSTEMID__", systemID());
        html.replace("__WATER_RESERVOIR_HEIGHT__", String(WATER_RESERVOIR_HEIGHT));
        html.replace("__RELAIS1_LABEL__", String(switchesPrefs.labelRelais1));
        html.replace("__RELAIS2_LABEL__", String(switchesPrefs.labelRelais2));
        html.replace("__RELAIS3_LABEL__", String(switchesPrefs.labelRelais3));
        html.replace("__RELAIS4_LABEL__", String(switchesPrefs.labelRelais4));
        html += FPSTR(FOOTER_html);
        html.replace("__FIRMWARE__", String(FIRMWARE_VERSION));
        html.replace("__BUILD__", String(__DATE__)+" "+String(__TIME__));
        webserver.send(200, "text/html", html);
    });

    // AJAX request from main page to update readings
    webserver.on("/ui", HTTP_GET, updateUI);

    // set/check valves
    webserver.on("/valve", HTTP_GET, []() {
        char reply[64];
        if (webserver.arg("on").toInt() >= 1 && webserver.arg("on").toInt() <= 4) {
            setRelais(webserver.arg("on").toInt(), true);
        } else if (webserver.arg("off").toInt() >= 1 && webserver.arg("off").toInt() <= 4) {
            setRelais(webserver.arg("off").toInt(), false);
        }
        if (relaisStatus(reply, sizeof(reply)) > 0) {
            webserver.send(200, F("application/json"), reply);
        } else {
            webserver.send(500, "text/plain", "ERR");
        }
        if (webserver.arg("on").toInt() > 0 || webserver.arg("off").toInt() > 0)
            mqtt_send(MQTT_TIMEOUT_MS); // publish changed relais settings
    });

    // show page with log files
    if (generalPrefs.enableLogging) {
        webserver.on("/logs", HTTP_GET, []() {
            logMsg("show logs");
            uint32_t freeBytes = LITTLEFS.totalBytes() * 0.95 - LITTLEFS.usedBytes();
            String html = FPSTR(HEADER_html);
            html += FPSTR(LOGS_HEADER_html);
            html.replace("__BYTES_FREE__", String(freeBytes / 1024));
            html += listDirHTML("/");
            html += FPSTR(LOGS_FOOTER_html);
            html += FPSTR(FOOTER_html);
            html.replace("__FIRMWARE__", String(FIRMWARE_VERSION));
            html.replace("__BUILD__", String(__DATE__) + " " + String(__TIME__));
            webserver.send(200, "text/html", html);
            Serial.println(F("Show log files."));
        });

        // delete all log files
        webserver.on("/rmlogs", HTTP_GET, []() {
            logMsg("remove logs");
            removeLogs();
            webserver.send(200, "text/plain", "OK");
        });
    }

    // handle request to update firmware
    webserver.on("/update", HTTP_GET, []() {
        String html = FPSTR(HEADER_html);
        html += FPSTR(UPDATE_html);
        html += FPSTR(FOOTER_html);
        html.replace("__FIRMWARE__", String(FIRMWARE_VERSION));
        html.replace("__BUILD__", String(__DATE__) + " " + String(__TIME__));
        html.replace("__DISPLAY__", "display:none;");
        webserver.send(200, "text/html", html);
        Serial.println(F("Show update page."));
    });

    // handle firmware upload
    webserver.on("/update", HTTP_POST, []() {
        String html = FPSTR(HEADER_html);
        if (Update.hasError()) {
            html += FPSTR(UPDATE_ERR_html);
            logMsg("ota failed");
        } else {
            html += FPSTR(UPDATE_OK_html);
            logMsg("ota successful");
        }
        html += FPSTR(FOOTER_html);
        html.replace("__FIRMWARE__", String(FIRMWARE_VERSION));
        html.replace("__BUILD__", String(__DATE__) + " " + String(__TIME__));
        webserver.send(200, "text/html", html);
    }, []() {
        HTTPUpload& upload = webserver.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.println(F("Starting OTA update..."));
            uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
            if (!Update.begin(maxSketchSpace)) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            webserverRequestMillis = millis();
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.println(F("Update successful!"));
            } else {
                Update.printError(Serial);
            }
        }
        esp_task_wdt_reset();
    });

    // show LoRaWAN settings
    webserver.on("/config", HTTP_GET, []() {
        String html;
        html += HEADER_html;
        html += CONFIG_html;

        html.replace("__STA_SSID__", String(generalPrefs.wifiStaSSID));
        html.replace("__STA_PASSWORD__", String(generalPrefs.wifiStaPassword));
        html.replace("__AP_PASSWORD__", String(generalPrefs.wifiApPassword));

        html.replace("__MQTT_BROKER__", String(generalPrefs.mqttBroker));
        html.replace("__MQTT_TOPIC_CMD__", String(generalPrefs.mqttTopicCmd));
        html.replace("__MQTT_TOPIC_STATE__", String(generalPrefs.mqttTopicState));
        html.replace("__MQTT_TOPIC_STATE__", String(generalPrefs.mqttTopicState));
        html.replace("__MQTT_INTERVAL__", String(generalPrefs.mqttPushInterval));
        html.replace("__MQTT_USERNAME__", String(generalPrefs.mqttUsername));
        html.replace("__MQTT_PASSWORD__", String(generalPrefs.mqttPassword));

        if (generalPrefs.mqttEnableAuth)
            html.replace("__MQTT_AUTH__", "checked");
        else
            html.replace("__MQTT_AUTH__", "");

        html.replace("__PUMP_AUTOSTOP__", String(switchesPrefs.pumpAutoStopSecs));
        html.replace("__PUMP_BLOCKTIME__", String(switchesPrefs.relaisBlockSecs));
        html.replace("__RESERVOIR_HEIGHT__", String(switchesPrefs.waterReservoirHeight));
        html.replace("__MIN_WATER_LEVEL__", String(switchesPrefs.minWaterLevel));

        html.replace("__RELAIS_PINS__", RELAIS_PINS);
        html.replace("__RELAIS1_LABEL__", String(switchesPrefs.labelRelais1));
        html.replace("__RELAIS1_PIN__", String(switchesPrefs.pinRelais1));
        html.replace("__RELAIS2_LABEL__", String(switchesPrefs.labelRelais2));
        html.replace("__RELAIS2_PIN__", String(switchesPrefs.pinRelais2));
        html.replace("__RELAIS3_LABEL__", String(switchesPrefs.labelRelais3));
        html.replace("__RELAIS3_PIN__", String(switchesPrefs.pinRelais3));
        html.replace("__RELAIS4_LABEL__", String(switchesPrefs.labelRelais4));
        html.replace("__RELAIS4_PIN__", String(switchesPrefs.pinRelais4));

        if (generalPrefs.enableLogging)
            html.replace("__LOGGING__", "checked");
        else
            html.replace("__LOGGING__", "");

        html += FPSTR(FOOTER_html);
        html.replace("__FIRMWARE__", String(FIRMWARE_VERSION));
        html.replace("__BUILD__", String(__DATE__) + " " + String(__TIME__));

        webserver.send(200, "text/html", html);
        Serial.print(millis());
        Serial.println(F(": Show system settings"));
    });

    // changes to LoRaWAN configuration must be saved to lorawanKeys prefs
    // and working copies in RTC memory used in lmic_init() after soft restart
    webserver.on("/config", HTTP_POST, []() {

        logMsg("webui save prefs");
        if (webserver.arg("appassword").length() >= 8 && webserver.arg("appassword").length() <= 32)
            strncpy(generalPrefs.wifiApPassword, webserver.arg("appassword").c_str(), 32); 
        if (webserver.arg("stassid").length() >= 8 && webserver.arg("stassid").length() <= 32)
            strncpy(generalPrefs.wifiStaSSID, webserver.arg("stassid").c_str(), 32); 
        if (webserver.arg("stapassword").length() >= 8 && webserver.arg("stapassword").length() <= 32)
            strncpy(generalPrefs.wifiStaPassword, webserver.arg("stapassword").c_str(), 32); 

        if (webserver.arg("mqttbroker").length() >= 4 && webserver.arg("mqttbroker").length() <= 64)
            strncpy(generalPrefs.mqttBroker, webserver.arg("mqttbroker").c_str(), 64); 

        if (webserver.arg("mqtttopiccmd").length() >= 4 && webserver.arg("mqtttopiccmd").length() <= 64)
            strncpy(generalPrefs.mqttTopicCmd, webserver.arg("mqtttopiccmd").c_str(), 64); 
        if (webserver.arg("mqtttopicstate").length() >= 4 && webserver.arg("mqtttopicstate").length() <= 64)
            strncpy(generalPrefs.mqttTopicState, webserver.arg("mqtttopicstate").c_str(), 64); 
        if (webserver.arg("mqttinterval").toInt() >= 10 && webserver.arg("mqttinterval").toInt() <= 600)
            generalPrefs.mqttPushInterval = webserver.arg("mqttinterval").toInt();
        if (webserver.arg("mqttuser").length() >= 4 && webserver.arg("mqttuser").length() <= 32)
            strncpy(generalPrefs.mqttUsername, webserver.arg("mqttuser").c_str(), 32); 
        if (webserver.arg("mqttpassword").length() >= 4 && webserver.arg("mqttpassword").length() <= 32)
            strncpy(generalPrefs.mqttPassword, webserver.arg("mqttpassword").c_str(), 32); 

        if (webserver.arg("mqttauth") == "on")
            generalPrefs.mqttEnableAuth = true;
        else
            generalPrefs.mqttEnableAuth = false;   

        if (webserver.arg("pump_autostop").toInt() >= 5 && webserver.arg("pump_autostop").toInt() <= 180)
            switchesPrefs.pumpAutoStopSecs = webserver.arg("pump_autostop").toInt();
        if (webserver.arg("pump_blocktime").toInt() >= 60 && webserver.arg("pump_blocktime").toInt() <= 7200)
            switchesPrefs.relaisBlockSecs = webserver.arg("pump_blocktime").toInt();
        if (webserver.arg("min_waterlevel").toInt() >= 5 && webserver.arg("min_waterlevel").toInt() <= 200)
            switchesPrefs.minWaterLevel = webserver.arg("min_waterlevel").toInt();    
        if (webserver.arg("reservoir_height").toInt() >= 10 && webserver.arg("reservoir_height").toInt() <= 200)
            switchesPrefs.waterReservoirHeight = webserver.arg("reservoir_height").toInt();

        if (webserver.arg("relais1_name").length() >= 3 && webserver.arg("relais1_name").length() <= 24)
            strncpy(switchesPrefs.labelRelais1, webserver.arg("relais1_name").c_str(), 24); 
        if (webserver.arg("relais1_pin").toInt() >= 1 && webserver.arg("relais1_pin").toInt() <= 39)
            switchesPrefs.pinRelais1 = webserver.arg("relais1_pin").toInt();
        if (webserver.arg("relais2_name").length() >= 3 && webserver.arg("relais2_name").length() <= 24)
            strncpy(switchesPrefs.labelRelais2, webserver.arg("relais2_name").c_str(), 24); 
        if (webserver.arg("relais2_pin").toInt() >= 1 && webserver.arg("relais2_pin").toInt() <= 39)
            switchesPrefs.pinRelais2 = webserver.arg("relais2_pin").toInt();
        if (webserver.arg("relais3_name").length() >= 3 && webserver.arg("relais3_name").length() <= 24)
            strncpy(switchesPrefs.labelRelais3, webserver.arg("relais3_name").c_str(), 24); 
        if (webserver.arg("relais3_pin").toInt() >= 1 && webserver.arg("relais3_pin").toInt() <= 39)
            switchesPrefs.pinRelais3 = webserver.arg("relais3_pin").toInt();
        if (webserver.arg("relais4_name").length() >= 3 && webserver.arg("relais4_name").length() <= 24)
            strncpy(switchesPrefs.labelRelais4, webserver.arg("relais4_name").c_str(), 24); 
        if (webserver.arg("relais4_pin").toInt() >= 1 && webserver.arg("relais4_pin").toInt() <= 39)
            switchesPrefs.pinRelais4 = webserver.arg("relais4_pin").toInt();   

        if (webserver.arg("logging") == "on")
            generalPrefs.enableLogging = true;
        else
            generalPrefs.enableLogging = false;
        
        // store settings in NVS
        nvs.putBool("general", true);
        nvs.putBytes("generalPrefs", &generalPrefs, sizeof(generalPrefs));       
        nvs.putBool("switches", true);
        nvs.putBytes("switchesPrefs", &switchesPrefs, sizeof(switchesPrefs));       

        webserver.sendHeader("Location", "/config?saved=1", true);
        webserver.send(302, "text/plain", "");
        Serial.print(millis());
        Serial.println(F(": System settings saved"));
    });

    if (generalPrefs.enableLogging) {
        webserver.on("/sendlogs", HTTP_GET, []() {
        logMsg("send all logs");
        sendAllLogs();
        });
    }

    // soft reboot (short deep sleep, RTC memory is preserved)
    webserver.on("/restart", HTTP_GET, []() {
        webserver.send(200, "text/plain", "OK");
        logMsg("webui restart");
        restartSystem();
    });

    // triggers ESP.restart() thus RTC memory is lost
    webserver.on("/reset", HTTP_GET, []() {
        webserver.send(200, "text/plain", "OK");
        logMsg("webui reset");
        resetSystem();
    });

    webserver.on("/delnvs", HTTP_GET, []() {
        logMsg("webui delnvs");
        nvs.clear();
        Serial.print(millis());
        Serial.println(F(": All settings in NVS removed"));
        webserver.send(200, "text/plain", "OK");
    });

    webserver.onNotFound([]() {
        String html;

        // send main page
        if (webserver.uri().endsWith("/")) {  
            html += HEADER_html;
            html += ROOT_html;
            webserver.send(200, "text/html", html);

        // send log file(s)
        } else if (!handleSendFile(webserver.uri()) && generalPrefs.enableLogging) { 
            webserver.send(404, "text/plain", "Error 404: file not found");
        }
    });

    webserver.begin();
    Serial.print(millis());
    Serial.println(F(": Webserver started."));
}


// change webserver timeout and reset its timer
void webserver_settimeout(uint16_t timeoutSecs) {
    if (webserverTimeout != timeoutSecs) {
        Serial.printf("Set webserver timeout to %d secs.\n", timeoutSecs);
        webserverTimeout = timeoutSecs;
    }
}


// reset timeout countdown
void webserver_tickle() {
    webserverRequestMillis = millis();
}



bool webserver_stop(bool force) {
    if (!force && (millis() - webserverRequestMillis) < (webserverTimeout*1000))
        return false;

    if (webserverRequestMillis > 0) {
        webserver.stop();
        webserverRequestMillis = 0;
        Serial.print(millis());
        Serial.println(F(": Webserver stopped"));
        logMsg("webserver off");
    }
    return true;
}
