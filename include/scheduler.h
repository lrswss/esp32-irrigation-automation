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



#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include <Arduino.h>
#include <sys/time.h>

#define MAX_JOBS 16

typedef struct valvejob_t valvejob_t;
typedef void (*jobfn_t)(uint8_t, bool);  // setRelais()

extern valvejob_t valvejobs[MAX_JOBS];

struct valvejob_t {
    struct valvejob_t* next;
    time_t time;
    jobfn_t func;
    uint8_t relais;
    bool state;
};

void schedule_job (valvejob_t* job, time_t time, jobfn_t func, uint8_t relais, bool state);
bool jobs_scheduled();
void scheduler();

#endif
