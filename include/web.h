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

#ifndef _WEBSERVER_H
#define _WEBSERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <Update.h>
#include <ArduinoJson.h>

// guard againt invalid remote configuration values
#define WEBSERVER_TIMEOUT_MIN_SECS 60
#define WEBSERVER_TIMEOUT_MAX_SECS 300

extern WebServer webserver;

void webserver_start();
bool webserver_stop();

#endif  
