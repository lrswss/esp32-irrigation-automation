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
#include "prefs.h"
#include "sensors.h"
#include "rtc.h"
#include "logging.h"
#include "mqtt.h"
#include "relais.h"

uint16_t pinstate = 0;

// relais pin and relais state value
// pin_number, state on, state blocked
uint16_t pinmap[][3] = { 
    { PUMP_PIN, 0x0001, 0x0002 },
    { RELAIS1_PIN, 0x0004, 0x0008 },
    { RELAIS2_PIN, 0x0010, 0x0020 },
    { RELAIS3_PIN, 0x0040, 0x0080 },
    { RELAIS4_PIN, 0x0100, 0x0200 }
};

// store time pin was last triggered
uint32_t pintime[] = { 0, 0, 0, 0, 0 };

char pinnames[5][7] = {
    "pump",
    "valve1",
    "valve2",
    "valve3",
    "valve4"
};


// define pins configure as relais control port as
// output set them high since relais are active low
void initRelais() {
    pinMode(switchesPrefs.pinPump, OUTPUT);
    digitalWrite(switchesPrefs.pinPump, 0); // active high
    pinMode(switchesPrefs.pinRelais1, OUTPUT);
    digitalWrite(switchesPrefs.pinRelais1, 1);
    pinMode(switchesPrefs.pinRelais2, OUTPUT);
    digitalWrite(switchesPrefs.pinRelais2, 1);
    pinMode(switchesPrefs.pinRelais3, OUTPUT);
    digitalWrite(switchesPrefs.pinRelais3, 1);
    pinMode(switchesPrefs.pinRelais4, OUTPUT);
    digitalWrite(switchesPrefs.pinRelais4, 1);
}


// check if any relais can be unblocked
void unblockRelais() {
    uint32_t blockTimeSecs;

    // don't unblock relais if pump currently blocked (due to low water level)
    if ((pinstate & pinmap[0][2]) != 0)
        return;

    // don't unblock other relais if one valve is currently open
    for (uint8_t i = 1; i < (sizeof(pinmap) / sizeof(pinmap[0])); i++) {
        if ((pinstate & pinmap[i][1]) != 0)
            return;
    }

    for (uint8_t i = 1; i < (sizeof(pinmap) / sizeof(pinmap[0])); i++) {
        blockTimeSecs = getLocalTime() - pintime[i];
        if ((pinstate & pinmap[i][2]) != 0 && blockTimeSecs > (switchesPrefs.relaisBlockSecs)) {
            pinstate &= ~pinmap[i][2];
        }
    } 
}


// switch relais on/off; also takes care of 
// blocking relais for certain time after last 
// being turned on and switching pump on/off 
void setRelais(uint8_t num, bool on) {
    uint32_t blockTimeSecs;
    bool valveOpen = false;
    char logmsg[32];

    if (num != 0) { // valves only
        if (on) {
            blockTimeSecs = getLocalTime() - pintime[num];
            if (blockTimeSecs > (switchesPrefs.relaisBlockSecs)) {
                if ((pinstate & pinmap[num][1]) == 0) {
                    digitalWrite(pinmap[num][0], 0); // active low
                    pinstate |= pinmap[num][1];
                    pinstate &= ~pinmap[num][2];
                    Serial.println(millis());
                    Serial.printf(": Opened %s", pinnames[num]);
                    sprintf(logmsg, "%s on", pinnames[num]);
                    logMsg(logmsg);

                    // block other relais if one is open to keep up pressure
                    for (uint8_t i = 1; i < (sizeof(pinmap) / sizeof(pinmap[0])); i++) {
                        if (i != num)
                            pinstate |= pinmap[i][2];
                    }
                }
            } else {
                Serial.println(millis());
                Serial.printf(": Relais %s blocked for %d secs!\n", pinnames[num],
                    ((switchesPrefs.relaisBlockSecs) - blockTimeSecs));
                return;
            }
        } else {
            if ((pinstate & pinmap[num][1]) != 0) {
                digitalWrite(pinmap[num][0], 1);
                pintime[num] = getLocalTime(); // remember open valve time
                pinstate &= ~pinmap[num][1];
                pinstate |= pinmap[num][2]; // blocks this relais for a while
                Serial.print(millis());
                Serial.printf(": Closed %s", pinnames[num]);
                sprintf(logmsg, "%s off", pinnames[num]);
                logMsg(logmsg);
            }
        }
    }

    // turn on pump if at least one valve is open
    for (uint8_t i = 1; i < (sizeof(pinmap) / sizeof(pinmap[0])); i++) {
        if ((pinstate & pinmap[i][1]) != 0)
            valveOpen = true;
    }

    // switch pump
    if (valveOpen || (!num && on)) {
        if ((pinstate & pinmap[0][1]) == 0) {
            pintime[0] = getLocalTime(); 
            digitalWrite(switchesPrefs.pinPump, 1);
            pinstate |= pinmap[0][1];
            Serial.print(millis());
            Serial.println(F(": Pump on"));
            sprintf(logmsg, "pump on, water %d cm", sensors.waterLevel);
            logMsg(logmsg);
        }
    } else if (!valveOpen || (!num && !on)) {
        if ((pinstate & pinmap[0][1]) != 0) {
            digitalWrite(switchesPrefs.pinPump, 0);
            pinstate &= ~pinmap[0][1];
            Serial.print(millis());
            Serial.println(F(": Pump off"));
            sprintf(logmsg, "pump off, water %d cm", sensors.waterLevel);
            logMsg(logmsg);
        }
    }
}


// prevent water pump from running dry
// turn off pump automatically after configured auto stop
// timeout or if water level reaches lower limit
void pumpAutoStop() {
    static char logmsg[48];
    bool pumpoff = false;

#if defined(US_TRIGGER_PIN) && defined(US_ECHO_PIN)
    if (sensors.waterLevel < 0 && (pinstate & pinmap[0][2]) == 0) {
        Serial.print(millis());
        Serial.println(F(": WARNING: Pump blocked (unknown water level)"));
        logMsg("pump blocked, unknown water level");
        for (uint8_t i = 0; i < (sizeof(pinmap) / sizeof(pinmap[0])); i++)
            pinstate |= pinmap[i][2]; // block all relais
    } else if (sensors.waterLevel > 0 && (pinstate & pinmap[0][2]) != 0) {
        pinstate &= ~pinmap[0][2];
        Serial.print(millis());
        Serial.printf(": Pump unblocked (water level %d cm)\n", sensors.waterLevel);
        sprintf(logmsg, "pump unblocked, water %d cm", sensors.waterLevel);
        logMsg(logmsg);
    }
#endif

    if ((pinstate & pinmap[0][1]) != 0) {
#if defined(US_TRIGGER_PIN) && defined(US_ECHO_PIN)
        readWaterLevel(false);
        if (sensors.waterLevel < 0)
            return; // triggers pump blocking above on next call to pumpAutoStop()
        if (sensors.waterLevel <= switchesPrefs.minWaterLevel) {
            pumpoff = true;
            Serial.print(millis());
            Serial.printf(": WARNING: low water level %d cm\n", sensors.waterLevel);
            sprintf(logmsg, "low water, %d cm", sensors.waterLevel);
            for (uint8_t i = 1; i < (sizeof(pinmap) / sizeof(pinmap[0])); i++) {
                pinstate |= pinmap[i][2]; // block relais
            }
            logMsg(logmsg);
        } else
#endif
        if ((getLocalTime() - pintime[0]) > switchesPrefs.pumpAutoStopSecs) {
            pumpoff = true;
            Serial.print(millis());
            Serial.printf(": Pump autostop, %d secs\n", switchesPrefs.pumpAutoStopSecs);
            sprintf(logmsg, "pump autostop, %d secs", switchesPrefs.pumpAutoStopSecs);
            logMsg(logmsg);
        }

        if (pumpoff) {
            for (int8_t i = ((sizeof(pinmap) / sizeof(pinmap[0])) - 1); i >= 0; i--)
                setRelais(i, false);
            mqtt_send(MQTT_TIMEOUT_MS);
        }
    }
}


// return current relais/pump status as json string
uint16_t relaisStatus(char *buf, size_t s) {
    static StaticJsonDocument<128> JSON;
    for (uint8_t i = 0; i < (sizeof(pinmap)/sizeof(pinmap[0])); i++) {
        if ((pinstate & pinmap[i][2]) != 0)
            JSON[pinnames[i]] = 2;  // blocked
        else if ((pinstate & pinmap[i][1]) != 0)
            JSON[pinnames[i]] = 1;  // on
        else
            JSON[pinnames[i]] = 0;  // off
    }
    return serializeJson(JSON, buf, s); 
}
