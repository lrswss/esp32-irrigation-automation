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
#include "rtc.h"
#include "logging.h"

RTC_DATA_ATTR uint32_t busyTime = 0;

// setup the ntp udp client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, 0);

// TimeZone settings details https://github.com/JChristensen/Timezone
TimeChangeRule CEST = { "CEST", Last, Sun, Mar, 2, 120 };
TimeChangeRule CET = { "CET", Last, Sun, Oct, 3, 60 };
Timezone TZ(CEST, CET);  // Frankfurt, Paris  // keep "TZ"


// start ntp client and update RTC
bool startNTPSync() {
    struct timeval tv;

    Serial.print(millis());
    timeClient.begin();
    timeClient.forceUpdate(); // takes a while
    if (timeClient.getEpochTime() > 1000) {
        // set ESP32 internal RTC
        tv.tv_sec = timeClient.getEpochTime(); // epoch
        tv.tv_usec = 0;
        settimeofday(&tv, NULL); // TZ UTC
        Serial.printf(": Local time: %s, %s\n", getDateString(), getTimeString(true));
        logMsg("ntp sync");
        return true;
    } else {
        Serial.println(F(": Syncing RTC with NTP-Server failed!"));
        return false;
    }
}


void stopNTPSync() {
    timeClient.end();
}


// returns system's total runtime 
char* getRuntime(uint32_t runtimeSecs) {
    static char str[16];

    memset(str, 0, sizeof(str));
    int days = runtimeSecs / 86400 ;
    int hours = (runtimeSecs % 86400) / 3600;
    int minutes = ((runtimeSecs % 86400) % 3600) / 60;
    sprintf(str, "%dd %dh %dm", days, hours, minutes);
    return str;
}


// returns local time (seconds since epoch)
time_t getLocalTime() {
    time_t time_utc;
    time(&time_utc);
    return TZ.toLocal(time_utc);
}


// returns current system time (UTC) as string
char* getSystemTime() {
    static char str[32];
    time_t now;

    now = getLocalTime();
    strftime(str, sizeof(str), "%Y/%m/%d %H:%M:%S (", localtime(&now));
    strcat(str, getTimeZone());  // add timezone abbreviation
    strcat(str, ")");
    return str;
}


// returns ptr to array with current time
char* getTimeString(bool showsecs) {
    static char strTime[9]; // HH:MM:SS
    struct tm tm;
    time_t now;

    now = getLocalTime();
    localtime_r(&now, &tm);
    memset(strTime, 0, sizeof(strTime));
    if (showsecs)
        sprintf(strTime, "%.2d:%.2d:%.2d", 
            tm.tm_hour, tm.tm_min, tm.tm_sec);
    else
        sprintf(strTime, "%.2d:%.2d", tm.tm_hour, tm.tm_min);
    return strTime;
}


// returns ptr to array with current date
char* getDateString() {
    static char strDate[11]; // DD.MM.YYYY
    struct tm tm;
    time_t now;

    now = getLocalTime();
    localtime_r(&now, &tm);
    memset(strDate, 0, sizeof(strDate));
    sprintf(strDate, "%.2d.%.2d.%4d", tm.tm_mday, tm.tm_mon+1, tm.tm_year+1900);
    return strDate;
}


// return abbreviation for current timezone
char* getTimeZone() {
    static char tz[6];
    TimeChangeRule *tcr; 
    time_t now;

    time(&now);
    TZ.toLocal(now, &tcr);
    strncpy(tz, tcr->abbrev, 5);
    return tz;
}