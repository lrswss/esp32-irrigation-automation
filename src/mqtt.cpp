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
#include "mqtt.h"
#include "logging.h"
#include "prefs.h"
#include "wlan.h"
#include "sensors.h"
#include "relais.h"
#include "utils.h"


WiFiClient wifi;
PubSubClient mqtt(wifi);
static char clientname[64];

// called if mqtt messages arrive on topics we've subscribed
static void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    if (strstr(topic, generalPrefs.mqttTopicCmd) != NULL) {
        for (uint8_t i = 1; i < (sizeof(pinmap)/sizeof(pinmap[0])); i++) {
            if (strstr(topic, pinnames[i])) {
                setRelais(i, strncmp((char*) payload, "on", length) == 0);
                mqtt_send(MQTT_TIMEOUT_MS);
            }
        }
    }
}


// set mqtt client name and callback function for subscribed topics
static void mqtt_init() {
    String name;
    static bool mqttInited = false;

    if (!mqttInited) {
        mqtt.setServer(generalPrefs.mqttBroker, MQTT_PORT);
        name = String(MQTT_CLIENT_NAME).substring(0,48) + "-" + systemID() + "-" + String(random(0xffff), HEX);
        sprintf(clientname, name.c_str(), name.length());
        mqtt.setCallback(mqtt_callback);
        mqttInited = true;
    }
}


// connect to mqtt broker and subscribe to valve cmd topics
bool mqtt_connect(uint16_t timeoutMillis) {
    static char buf[96], logmsg[96];
    uint8_t retries = 0;

    mqtt_init();
    if (mqtt.connected())
        return true;

    if (!wifi_uplink(false)) {
        Serial.print(millis());
        Serial.println(F(": MQTT: cannot connect, no WiFi uplink."));
        return false;
    }

    Serial.print(millis());
    Serial.printf(": MQTT: connecting to broker %s", generalPrefs.mqttBroker);

    while (retries++ < int(timeoutMillis/500)) {
        Serial.print(".");
        if (generalPrefs.mqttEnableAuth)
            mqtt.connect(clientname, generalPrefs.mqttUsername, generalPrefs.mqttPassword);
        else
            mqtt.connect(clientname);
        delay(500);
    }

    if (mqtt.connected()) {
        Serial.println(F("success!"));
        // subscribe to cmd topics for remote valve switching
        for (uint8_t i = 1; i < (sizeof(pinmap)/sizeof(pinmap[0])); i++) {
            snprintf(buf, sizeof(buf)-1, "%s/%s", generalPrefs.mqttTopicCmd, pinnames[i]);
            if (!mqtt.subscribe(buf)) {
                Serial.print(millis());
                Serial.printf(": MQTT: subscribe %s failed!\n", buf);
                sprintf(logmsg, "mqtt subscribe %s failed", buf);
                logMsg(logmsg);
            }
        }
        return true;
    } else {
        Serial.println(F("failed!"));
        sprintf(logmsg, "mqtt connection to %s failed", generalPrefs.mqttBroker);
        logMsg(logmsg);
        return false;
    }
}


// try to publish sensor reedings with given timeout 
// will implicitly call mqtt_init()
bool mqtt_send(uint16_t timeoutMillis) {
    StaticJsonDocument<256> JSON;
    static char status[64], topic[64], buf[128];

    if (!wifi_uplink(false)) {
        Serial.print(millis());
        Serial.println(F(": MQTT: cannot send, no WiFi uplink."));
        return false;
    }

    // create JSON with relais status and sensor readings
    relaisStatus(status, sizeof(status));
    deserializeJson(JSON, status);
    readTemp(false);
    readWaterLevel(false);
    JSON["temp"] = sensors.temperature;
    JSON["hum"] = sensors.humidity;
    JSON["level"] = sensors.waterLevel;
    size_t s = serializeJson(JSON, buf);

    if (mqtt_connect(timeoutMillis)) {
        snprintf(topic, sizeof(topic)-1, "%s", generalPrefs.mqttTopicState);
        Serial.print(millis());
        if (mqtt.publish(topic, buf, s)) {
            Serial.printf(": MQTT: published %d bytes to %s on %s\n", s, 
                generalPrefs.mqttTopicState, generalPrefs.mqttBroker);
            return true;
        } else {
            Serial.println(F(": MQTT: publish failed!"));
            logMsg("mqtt publish failed");
        }
    }

    return false;
}