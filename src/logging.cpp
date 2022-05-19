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

#include "logging.h"
#include "web.h"
#include "rtc.h"
#include "config.h"
#include "utils.h"
#include "prefs.h"

#ifdef LANG_EN
#include "html_EN.h"
#else
#include "html_DE.h"
#endif

static bool fsInited = true;

void initLogging() {
    uint32_t freeBytes;
    
    if (LITTLEFS.begin(true)) {
        freeBytes = LITTLEFS.totalBytes() * 0.95 - LITTLEFS.usedBytes();
        Serial.print(F("LittleFS mounted: "));
        Serial.print(freeBytes/1024);
        Serial.println(F(" kb free"));
        listDirectory("/");
        fsInited = true;
    } else {
        Serial.println(F("Failed to mount LittleFS!"));
    }
}


void logMsg(const char *msg) {
    File logfile;
    char timeStr[20];
    time_t now;
    struct tm tm;
    
    if (!switchesPrefs.enableLogging || !fsInited)
        return;

    now = getLocalTime();
    localtime_r(&now, &tm);
    logfile = LITTLEFS.open(LOGFILE_NAME, "a");
    if (logfile) {
        sprintf(timeStr, "%4d-%.2d-%.2dT%.2d:%.2d:%.2d", 
            tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, 
            tm.tm_hour, tm.tm_min, tm.tm_sec);
        logfile.print(timeStr);
        logfile.print(",");
        logfile.println(msg);
        logfile.close();
    }
}


bool handleSendFile(String path) {
    if (!switchesPrefs.enableLogging || !fsInited)
        return false;

    if (LITTLEFS.exists(path)) {
        File file = LITTLEFS.open(path, "r");
        if (file) {
            Serial.print(millis());
            Serial.printf(": Sending file %s (%d bytes)...\n", file.name(), file.size());
            webserver.streamFile(file, "text/plain");
            file.close();
            return true;
        }
    }
    return false;
}


void listDirectory(const char* dir) {
    File rootDir, file;

    if (!switchesPrefs.enableLogging || !fsInited)
        return;

    rootDir = LITTLEFS.open(dir);
    file = rootDir.openNextFile();
    Serial.println(F("Contents of root directory: "));
    while (file) {
        Serial.print(file.name());
        Serial.print(" (");
        Serial.print(file.size());
        Serial.println(" bytes)");
        file = rootDir.openNextFile();
    }
}


String listDirHTML(const char* path) {
    String listing;

    if (!switchesPrefs.enableLogging || !fsInited)
       return listing;

    File root = LITTLEFS.open("/");
    File file = root.openNextFile();
    while (file) {
        listing += "<a href=\"";
        listing += file.name();
        listing += "\">";
        listing += String(file.name());
        listing += "</a> (";
        listing += file.size();
        listing += " bytes)<br>\n";
        file = root.openNextFile();
    }
    return listing;
}


void removeLogs() {
    String filename;
    File rootDir, file;

    if (!switchesPrefs.enableLogging || !fsInited)
        return;

    rootDir = LITTLEFS.open("/");
    file = rootDir.openNextFile();
    while (file) {
        filename = "/" + String(file.name());
        file.close();
        Serial.print(millis());
        Serial.print(F(": Removing file "));
        Serial.print(filename);
        Serial.print(F("..."));
        if (LITTLEFS.remove(filename))
            Serial.println("ok.");
        else
            Serial.println("failed!");
        file = rootDir.openNextFile();
        delay(100);
    }
}


void rotateLogs() {
  String fOld, fNew;
  int maxFiles = 0;

  File file = LITTLEFS.open(LOGFILE_NAME, "r");
  if (file && file.size() > LOGFILE_MAX_SIZE) {
    file.close();
    maxFiles = int(LITTLEFS.totalBytes() * 0.95 / LOGFILE_MAX_SIZE) - 1;
    maxFiles = max(LOGFILE_MAX_FILES, maxFiles);
    LITTLEFS.remove(String(LOGFILE_NAME) + "." + LOGFILE_MAX_FILES); // remove oldest log file
    for (uint8_t i = maxFiles; i > 1; i--) {
      fOld = String(LOGFILE_NAME) + "." + String(i - 1);
      fNew = String(LOGFILE_NAME) + "." + i;
      if (LITTLEFS.exists(fOld)) {
        Serial.print(fOld); Serial.print(" -> "); Serial.println(fNew);
        LITTLEFS.rename(fOld, fNew);
        esp_task_wdt_reset();
      }
    }
    fOld = String(LOGFILE_NAME);
    fNew = String(LOGFILE_NAME) + "." + 1;
    Serial.print(fOld); Serial.print(" -> "); Serial.println(fNew);
    LITTLEFS.rename(fOld, fNew);
  }
}


// concate all log files into one http content stream
void sendAllLogs() {
  File file;
  String logFile, downloadFile;
  WiFiClient client = webserver.client();
  uint32_t totalSize = 0;

  if (!switchesPrefs.enableLogging || !fsInited)
    return;

    // determine total size of all files (for content-size header)
    file = LITTLEFS.open(LOGFILE_NAME, "r");
    if (file)
        totalSize = file.size();
    for (uint8_t i = LOGFILE_MAX_FILES; i >= 1; i--) {
        logFile = String(LOGFILE_NAME) + "." + i;
        file = LITTLEFS.open(logFile, "r");
        if (file)
        totalSize += file.size();
    }
    Serial.printf("Sending all logs in one file (%d bytes)...\n", totalSize);

    // send header for upcoming byte stream
    downloadFile = "irrigation_" + systemID() + ".log";
    webserver.sendHeader("Content-Type", "text/plain");
    webserver.sendHeader("Content-Disposition", "attachment; filename="+downloadFile);
    webserver.setContentLength(totalSize);
    webserver.sendHeader("Connection", "close");
    webserver.send(200, "application/octet-stream","");

    // finally stream all files to client in one chunck
    for (uint8_t i = LOGFILE_MAX_FILES; i >= 1; i--) {
        logFile = String(LOGFILE_NAME) + "." + i;
        file = LITTLEFS.open(logFile, "r");
        if (file)
            client.write(file);
    }
    file = LITTLEFS.open(LOGFILE_NAME, "r");
    if (file)
        client.write(file);
    client.flush();
}
