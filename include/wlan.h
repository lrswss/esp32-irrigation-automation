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

#ifndef _WIFI_H
#define _WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "esp_wifi.h"
#include "config.h"

//#define MDNS_NAME "irrigation"

bool wifi_hotspot(bool activate);
bool wifi_uplink(bool reconnect);

#endif
