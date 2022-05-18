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

#ifndef _UTILS_H
#define _UTILS_H

#include <Arduino.h>
#include "esp_task_wdt.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_ota_ops.h"

enum rstcodes {
    POWERUP,
    RESTART,
    SOFTRESET,
    EXCEPTION,
    WATCHDOG,
    BROWNOUT,
    OTHER
};

extern rstcodes runmode;
extern char runmodes[7][10];

String systemID();
rstcodes bootMsg();
char* checkFirmwareUpdate();
void restartSystem();
void resetSystem();
void array2string(const byte *arr, int len, char *buf, bool reverse);
void printHEX8bit(uint8_t *arr, uint8_t len, bool ln, bool reverse);
#endif

