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

#ifndef _LOGGING_H
#define _LOGGING_H

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

#define LOGFILE_MAX_SIZE 1024*50  // 50k
#define LOGFILE_MAX_FILES 24
#define LOGFILE_NAME "/irrigation.log"

void initLogging();
void logMsg(const char *msg);
void listDirectory(const char* dir);
void sendAllLogs();
void rotateLogs();
void removeLogs();
bool handleSendFile(String path);
String listDirHTML(const char* path);

#endif
