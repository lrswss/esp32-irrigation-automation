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
#include "relay.h"
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
    static char buf[192], label[16];
    static StaticJsonDocument<384> JSON;

    memset(buf, 0, sizeof(buf));
    JSON.clear();
    JSON["logging"] = switchesPrefs.enableLogging ? 1 : 0;
    if (getLocalTime() > 1609455600) {  // RTC already set?
        JSON["date"] = getDateString();
        JSON["time"] = getTimeString(false);
        JSON["tz"] = getTimeZone();
    }
    JSON["runtime"] = getRuntime(busyTime);
    JSON["wifi"] = wifi_uplink(false) ? 1 : 0;
#ifdef HAS_HTU21D
	char temp[8];
    if (sensors.humidity > 0) {
        dtostrf(sensors.temperature, 4, 1, temp);
        JSON["temp"] = temp;
        JSON["hum"] = sensors.humidity;
    }
#endif
#if defined(US_TRIGGER_PIN) && defined(US_ECHO_PIN)
    if (!switchesPrefs.ignoreWaterLevel)
        JSON["level"] = sensors.waterLevel;
    else
        JSON["level"] = -2;
#endif
    for (uint8_t i = 0; i < NUM_MOISTURE_SENSORS; i++) {
        if (switchesPrefs.pinMoisture[i] > 0) {
            sprintf(label, "moist%d", i+1);
            JSON[label] = sensors.moisture[i];
        }
    }
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
        char buf[32];

        html += FPSTR(ROOT_html);
        html.replace("__SYSTEMID__", systemID());
        html.replace("__WATER_RESERVOIR_HEIGHT__", String(WATER_RESERVOIR_HEIGHT));
        html.replace("__MIN_WATER_LEVEL__", String(switchesPrefs.minWaterLevel));
        for (uint8_t i = 1; i <= NUM_RELAY; i++) {
            sprintf(buf, "__RELAY%d_LABEL__", i);
            html.replace(buf, String(switchesPrefs.labelRelay[i-1]));

        }
        for (uint8_t i = 1; i <= NUM_MOISTURE_SENSORS; i++) {
            sprintf(buf, "__MOIST%d_LABEL__", i);
            html.replace(buf, String(switchesPrefs.labelMoisture[i-1]));
        }

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
            setRelay(webserver.arg("on").toInt(), true);
        } else if (webserver.arg("off").toInt() >= 1 && webserver.arg("off").toInt() <= 4) {
            setRelay(webserver.arg("off").toInt(), false);
        }
        if (relayStatus(reply, sizeof(reply)) > 0) {
            webserver.send(200, F("application/json"), reply);
        } else {
            webserver.send(500, "text/plain", "ERR");
        }
        if (webserver.arg("on").toInt() > 0 || webserver.arg("off").toInt() > 0)
            mqtt_send(MQTT_TIMEOUT_MS); // publish changed relay settings
    });

    // show page with log files
    if (switchesPrefs.enableLogging) {
        webserver.on("/logs", HTTP_GET, []() {
            logMsg("show logs");
            uint32_t freeBytes = LittleFS.totalBytes() * 0.95 - LittleFS.usedBytes();
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

    // show network settings
    webserver.on("/network", HTTP_GET, []() {
        String html;
        html += HEADER_html;
        html += NETWORK_html;

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

        html += FPSTR(FOOTER_html);
        html.replace("__FIRMWARE__", String(FIRMWARE_VERSION));
        html.replace("__BUILD__", String(__DATE__) + " " + String(__TIME__));

        webserver.send(200, "text/html", html);
        Serial.print(millis());
        Serial.println(F(": Show network settings"));
    });

    // save network settings to NVS
    webserver.on("/network", HTTP_POST, []() {
        logMsg("webui save network prefs");

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

        nvs.putBool("general", true);
        nvs.putBytes("generalPrefs", &generalPrefs, sizeof(generalPrefs));  

        webserver.sendHeader("Location", "/network?saved=1", true);
        webserver.send(302, "text/plain", "");
        Serial.print(millis());
        Serial.println(F(": Network settings saved"));
    });

    // show pin settings
    webserver.on("/pins", HTTP_GET, []() {
        String html;
        char buf[32];
        html += HEADER_html;
        html += PINS_html;

        html.replace("__RELAY_PINS__", RELAY_PINS);
        for (uint8_t i = 1; i <= NUM_RELAY; i++) {
            sprintf(buf, "__RELAY%d_LABEL__", i);
            html.replace(buf, String(switchesPrefs.labelRelay[i-1]));
            sprintf(buf, "__RELAY%d_PIN__", i);
            html.replace(buf, String(switchesPrefs.pinRelay[i-1]));
        }

        html.replace("__MOISTURE_PINS__", MOISTURE_PINS);
        for (uint8_t i = 1; i <= NUM_MOISTURE_SENSORS; i++) {
            sprintf(buf, "__MOIST%d_LABEL__", i);
            html.replace(buf, String(switchesPrefs.labelMoisture[i-1]));
            sprintf(buf, "__MOIST%d_PIN__", i);
            html.replace(buf, String(switchesPrefs.pinMoisture[i-1]));
        }

        html.replace("__MOISTURE_MIN__", String(switchesPrefs.moistureMin));
        html.replace("__MOISTURE_MAX__", String(switchesPrefs.moistureMax));
        if (switchesPrefs.moistureRaw)
            html.replace("__MOISTURE_RAW__", "checked");
        else
            html.replace("__MOISTURE_RAW__", "");
        if (switchesPrefs.moistureMovingAvg)
            html.replace("__MOISTURE_AVG__", "checked");
        else
            html.replace("__MOISTURE_AVG__", "");

        html += FPSTR(FOOTER_html);
        html.replace("__FIRMWARE__", String(FIRMWARE_VERSION));
        html.replace("__BUILD__", String(__DATE__) + " " + String(__TIME__));

        webserver.send(200, "text/html", html);
        Serial.print(millis());
        Serial.println(F(": Show pin settings"));
    });

    // save pin settings to NVS
    webserver.on("/pins", HTTP_POST, []() {
        char buf[32];

        logMsg("webui save pin prefs");
        for (uint8_t i = 1; i <= NUM_RELAY; i++) {
            sprintf(buf, "relay%d_name", i);
            if (webserver.arg(buf).length() >= 3 && webserver.arg(buf).length() <= 24) {
                strncpy(switchesPrefs.labelRelay[i-1], webserver.arg(buf).c_str(), 24);
            }
            sprintf(buf, "relay%d_pin", i);
            if (webserver.arg(buf).toInt() >= -1 && webserver.arg(buf).toInt() <= 39) {
                switchesPrefs.pinRelay[i-1] = webserver.arg(buf).toInt();
            }
        }

       for (uint8_t i = 1; i <= NUM_MOISTURE_SENSORS; i++) {
            sprintf(buf, "moist%d_name", i);
            if (webserver.arg(buf).length() >= 3 && webserver.arg(buf).length() <= 24)
                strncpy(switchesPrefs.labelMoisture[i-1], webserver.arg(buf).c_str(), 24);
            sprintf(buf, "moist%d_pin", i);
            if (webserver.arg(buf).toInt() >= -1 && webserver.arg(buf).toInt() <= 39)
                switchesPrefs.pinMoisture[i-1] = webserver.arg(buf).toInt();
        }

        if (webserver.arg("moisture_min").toInt() >= 100 && webserver.arg("moisture_min").toInt() <= 1000)
            switchesPrefs.moistureMin = webserver.arg("moisture_min").toInt();
        if (webserver.arg("moisture_max").toInt() >= 100 && webserver.arg("moisture_max").toInt() <= 1000)
            switchesPrefs.moistureMax = webserver.arg("moisture_max").toInt();
        if (webserver.arg("moisture_raw") == "on")
            switchesPrefs.moistureRaw = true;
        else
            switchesPrefs.moistureRaw = false;
        if (webserver.arg("moisture_avg") == "on")
            switchesPrefs.moistureMovingAvg = true;
        else
            switchesPrefs.moistureMovingAvg = false;           

        // store settings in NVS     
        nvs.putBool("switches", true);
        nvs.putBytes("switchesPrefs", &switchesPrefs, sizeof(switchesPrefs));

        // reset moving average values
        if (switchesPrefs.moistureMovingAvg)
            readMoisture(true, false, true);     

        webserver.sendHeader("Location", "/pins?saved=1", true);
        webserver.send(302, "text/plain", "");
        Serial.print(millis());
        Serial.println(F(": Pin settings saved"));
    });

    // show irrgation settings
    webserver.on("/config", HTTP_GET, []() {
        String html;
        char buf[32];
        html += HEADER_html;
        html += CONFIG_html;

        if (switchesPrefs.enableAutoIrrigation) 
            html.replace("__AUTO_IRRIGATION__", "checked");
        else
            html.replace("__AUTO_IRRIGATION__", "");
        html.replace("__IRRIGATION_TIME__", String(switchesPrefs.autoIrrigationTime));
        html.replace("__IRRIGATION_PAUSE__", String(switchesPrefs.autoIrrigationPauseHours));

        for (uint8_t i = 1; i <= NUM_RELAY; i++) {
            sprintf(buf, "__RELAY%d_LABEL__", i);
            html.replace(buf, String(switchesPrefs.labelRelay[i-1]));
            sprintf(buf, "__IRRIGATION_RELAY%d_SECS__", i);
            html.replace(buf, String(switchesPrefs.autoIrrigationSecs[i-1]));
        }

        html.replace("__PUMP_AUTOSTOP__", String(switchesPrefs.pumpAutoStopSecs));
        html.replace("__PUMP_BLOCKTIME__", String(switchesPrefs.relaysBlockMins));
        html.replace("__RESERVOIR_HEIGHT__", String(switchesPrefs.waterReservoirHeight));
        html.replace("__MIN_WATER_LEVEL__", String(switchesPrefs.minWaterLevel));

        if (switchesPrefs.ignoreWaterLevel)
            html.replace("__IGNORE_WATER_LEVEL__", "checked");
        else
            html.replace("__IGNORE_WATER_LEVEL__", "");

        if (switchesPrefs.enableLogging)
            html.replace("__LOGGING__", "checked");
        else
            html.replace("__LOGGING__", "");

        html += FPSTR(FOOTER_html);
        html.replace("__FIRMWARE__", String(FIRMWARE_VERSION));
        html.replace("__BUILD__", String(__DATE__) + " " + String(__TIME__));

        webserver.send(200, "text/html", html);
        Serial.print(millis());
        Serial.println(F(": Show main settings"));
    });

    // save main settings to NVS
    webserver.on("/config", HTTP_POST, []() {
        char buf[32];

        logMsg("webui save main prefs");
        if (webserver.arg("auto_irrigation") == "on")
            switchesPrefs.enableAutoIrrigation = true;
        else
            switchesPrefs.enableAutoIrrigation = false;

        if (webserver.arg("irrigation_time").length() == 5)
            strncpy(switchesPrefs.autoIrrigationTime, webserver.arg("irrigation_time").c_str(), 5); 
        if (webserver.arg("irrigation_pause").toInt() >= 1 && webserver.arg("irrigation_pause").toInt() <= 24)
            switchesPrefs.autoIrrigationPauseHours = webserver.arg("irrigation_pause").toInt();
        for (uint8_t i = 1; i <= NUM_RELAY; i++) {
            sprintf(buf, "irrigation_relay%d_secs", i);
            if (webserver.arg(buf).toInt() >= 10 && webserver.arg(buf).toInt() <= switchesPrefs.pumpAutoStopSecs)
                switchesPrefs.autoIrrigationSecs[i-1] = webserver.arg(buf).toInt();
        }

        if (webserver.arg("pump_autostop").toInt() >= 10 && webserver.arg("pump_autostop").toInt() <= 300)
            switchesPrefs.pumpAutoStopSecs = webserver.arg("pump_autostop").toInt();
        if (webserver.arg("pump_blocktime").toInt() >= 60 && webserver.arg("pump_blocktime").toInt() <= 2880)
            switchesPrefs.relaysBlockMins = webserver.arg("pump_blocktime").toInt();
        if (webserver.arg("min_water_level").toInt() >= 4 && webserver.arg("min_water_level").toInt() <= 200)
            switchesPrefs.minWaterLevel = webserver.arg("min_water_level").toInt();    
        if (webserver.arg("reservoir_height").toInt() >= 10 && webserver.arg("reservoir_height").toInt() <= 200)
            switchesPrefs.waterReservoirHeight = webserver.arg("reservoir_height").toInt();
        
        if (webserver.arg("ignore_water_level") == "on")
            switchesPrefs.ignoreWaterLevel = true;
        else
            switchesPrefs.ignoreWaterLevel = false;

        if (webserver.arg("logging") == "on")
            switchesPrefs.enableLogging = true;
        else
            switchesPrefs.enableLogging = false;

        // store settings in NVS     
        nvs.putBool("switches", true);
        nvs.putBytes("switchesPrefs", &switchesPrefs, sizeof(switchesPrefs));       

        webserver.sendHeader("Location", "/config?saved=1", true);
        webserver.send(302, "text/plain", "");
        Serial.print(millis());
        Serial.println(F(": Main settings saved"));
    });

    if (switchesPrefs.enableLogging) {
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
        } else if (!handleSendFile(webserver.uri()) && switchesPrefs.enableLogging) { 
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
