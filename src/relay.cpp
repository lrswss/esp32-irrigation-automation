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
#include "relay.h"

uint16_t pinstate = 0;

// relay pin and relay state value
// pin_number, state on, state blocked
uint16_t pinmap[][3] = { 
    { PUMP_PIN, 0x0001, 0x0002 },
    { RELAY1_PIN, 0x0004, 0x0008 },
    { RELAY2_PIN, 0x0010, 0x0020 },
    { RELAY3_PIN, 0x0040, 0x0080 },
    { RELAY4_PIN, 0x0100, 0x0200 }
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


// define pins configure as relay control port as
// output set them high since relay are active low
void initRelays() {
    pinMode(switchesPrefs.pinPump, OUTPUT);
    digitalWrite(switchesPrefs.pinPump, 0); // active high
    for (uint8_t i = 0; i <= 4; i++) {
        pinMode(switchesPrefs.pinRelay[i-1], OUTPUT);
        digitalWrite(switchesPrefs.pinRelay[i-1], 1);
    }
}


// check if any relay can be unblocked
void unblockRelays() {
    uint32_t blockTimeSecs;

    // don't unblock relay if pump currently blocked (e.g. due to low water level)
    if ((pinstate & pinmap[0][2]) != 0)
        return;

    // don't unblock other relay if one valve is currently open
    for (uint8_t i = 1; i < (sizeof(pinmap) / sizeof(pinmap[0])); i++) {
        if ((pinstate & pinmap[i][1]) != 0)
            return;
    }

    for (uint8_t i = 1; i < (sizeof(pinmap) / sizeof(pinmap[0])); i++) {
        blockTimeSecs = getLocalTime() - pintime[i];
        if ((pinstate & pinmap[i][2]) != 0 && blockTimeSecs > (switchesPrefs.relaysBlockMins * 60)) {
            pinstate &= ~pinmap[i][2];
        }
    } 
}


// switch relay on/off; also takes care of 
// blocking relay for certain time after last 
// being turned on and switching pump on/off 
void setRelay(uint8_t num, bool on) {
    bool valveOpen = false;
    char logmsg[32];

    if (num != 0) { // valves only
        if (on) {
            if ((pinstate & pinmap[num][2]) == 0) {
                if ((pinstate & pinmap[num][1]) == 0) {
                    digitalWrite(pinmap[num][0], 0); // active low
                    pinstate |= pinmap[num][1];
                    pinstate &= ~pinmap[num][2];
                    Serial.print(millis());
                    Serial.printf(": Opened %s\n", pinnames[num]);
                    sprintf(logmsg, "%s on", pinnames[num]);
                    logMsg(logmsg);

                    // block other relay if one is open to keep up pressure
                    for (uint8_t i = 1; i < (sizeof(pinmap) / sizeof(pinmap[0])); i++) {
                        if (i != num)
                            pinstate |= pinmap[i][2];
                    }
                }
            } else {
                Serial.print(millis());
                Serial.printf(": Relay %s blocked!\n", pinnames[num]);
                return;
            }
        } else {
            if ((pinstate & pinmap[num][1]) != 0) {
                digitalWrite(pinmap[num][0], 1);
                pintime[num] = getLocalTime(); // remember open valve time
                pinstate &= ~pinmap[num][1];
                pinstate |= pinmap[num][2]; // blocks this relay for a while
                Serial.print(millis());
                Serial.printf(": Closed %s\n", pinnames[num]);
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
            sprintf(logmsg, "pump on, water %dcm", sensors.waterLevel);
            logMsg(logmsg);
        }
    } else if (!valveOpen || (!num && !on)) {
        if ((pinstate & pinmap[0][1]) != 0) {
            digitalWrite(switchesPrefs.pinPump, 0);
            pinstate &= ~pinmap[0][1];
            Serial.print(millis());
            Serial.println(F(": Pump off"));
            sprintf(logmsg, "pump off, water %dcm", sensors.waterLevel);
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
    // update level reading regularly if pump is running
    if ((pinstate & pinmap[0][1]) != 0)
        readWaterLevel(false, false);

    // unblock pump if water level is known or deliberately ignored
    if ((sensors.waterLevel > switchesPrefs.minWaterLevel || 
            switchesPrefs.ignoreWaterLevel) && (pinstate & pinmap[0][2]) != 0) {
        pinstate &= ~pinmap[0][2];
        Serial.print(millis());
        Serial.printf(": Pump unblocked (%swater level %d cm)\n", 
            switchesPrefs.ignoreWaterLevel ? "ignoring " : "", sensors.waterLevel);
        sprintf(logmsg, "pump unblocked, %swater %dcm", 
            switchesPrefs.ignoreWaterLevel ? "ignoring " : "", sensors.waterLevel);
        logMsg(logmsg);

    // turn off and block pump and valves if water level is unknown due to sensor error
    } else if (sensors.waterLevel <= switchesPrefs.minWaterLevel &&
            !switchesPrefs.ignoreWaterLevel && (pinstate & pinmap[0][2]) == 0) {
        Serial.print(millis());
        if (sensors.waterLevel <= 0) {
            Serial.println(F(": WARNING: System blocked (unknown water level)"));
            logMsg("system blocked, unknown water level");
        } else {
            Serial.printf(": WARNING: Low water level %d cm\n", sensors.waterLevel);
            sprintf(logmsg, "low water, %dcm", sensors.waterLevel);
            logMsg(logmsg);
        }
        pumpoff = true;
        for (uint8_t i = 0; i < (sizeof(pinmap) / sizeof(pinmap[0])); i++)
            pinstate |= pinmap[i][2]; // block all relay
    }
#endif

    // turn off pump if auto-stop time has been reached 
    // time limit is checked to avoid accidental overwatering
    if (pinstate & pinmap[0][1] != 0) {
        if ((getLocalTime() - pintime[0]) > switchesPrefs.pumpAutoStopSecs) {
            pumpoff = true;
            Serial.print(millis());
            Serial.printf(": Pump autostop, %d secs\n", switchesPrefs.pumpAutoStopSecs);
            sprintf(logmsg, "pump autostop, %d secs", switchesPrefs.pumpAutoStopSecs);
            logMsg(logmsg);
        }

        // block all valves and then turn off pump
        if (pumpoff) {
            for (int8_t i = ((sizeof(pinmap) / sizeof(pinmap[0])) - 1); i >= 0; i--)
                setRelay(i, false);
            readMoisture(true, true, false);
            mqtt_send(MQTT_TIMEOUT_MS);
        }
    }
}


// return current relay/pump status as json string
uint16_t relayStatus(char *buf, size_t s) {
    static StaticJsonDocument<128> JSON;
    for (uint8_t i = 0; i < (sizeof(pinmap)/sizeof(pinmap[0])); i++) {
        if (i > 0 && switchesPrefs.pinRelay[i-1] < 0)
            JSON[pinnames[i]] = -1;  // disabled
        else if ((pinstate & pinmap[i][2]) != 0)
            JSON[pinnames[i]] = 2;  // blocked
        else if ((pinstate & pinmap[i][1]) != 0)
            JSON[pinnames[i]] = 1;  // on
        else
            JSON[pinnames[i]] = 0;  // off
    }
    return serializeJson(JSON, buf, s); 
}
