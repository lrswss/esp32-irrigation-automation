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

#include "scheduler.h"

valvejob_t* scheduledjobs = NULL;
valvejob_t valvejobs[MAX_JOBS];

// schedule a valve job
void schedule_job(valvejob_t* job, time_t time, jobfn_t func, uint8_t relay, bool state) {
    valvejob_t** pnext;

    if (time <= 0)
        time = 1;

    // create job...
    job->next = NULL;
    job->time = time;
    job->func = func;
    job->relay = relay;
    job->state = state;

    // ...and insert it into schedule
    for (pnext = &scheduledjobs; *pnext; pnext = &((*pnext)->next)) {
        if (((*pnext)->time - time) > 0) {
            job->next = *pnext;
            break;
        }
    }
    *pnext = job;
}


// execute scheduled jobs
void scheduler() {
    if (jobs_scheduled() && (scheduledjobs->time < millis())) {
        scheduledjobs->func(scheduledjobs->relay, scheduledjobs->state);
        scheduledjobs = scheduledjobs->next;
    }
}


// check for scheduled jobs
bool jobs_scheduled() {
    return scheduledjobs != NULL;
}
