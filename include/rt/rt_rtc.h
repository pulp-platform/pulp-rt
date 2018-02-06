
/*
 * Copyright (C) 2018 GreenWaves Technologies
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __RT_RTC_H__
#define __RT_RTC_H__

/// @cond IMPLEM

typedef enum{
  MODE_CALENDAR,
  MODE_ALARM,
  MODE_CNTDOWN,
  MODE_CALIBR
}rt_rtc_mode_e;

typedef enum{
  INT_CALIBRATION = 0,
  INT_CNTDOWN,
  INT_ALARM
}rt_irq_rtc_e;

typedef enum{
  RTC_START,
  RTC_STOP,
  RTC_RESET,
  RTC_CLK_DIV_SET,
  RTC_ALARM_SET,
  RTC_ALARM_START,
  RTC_ALARM_STOP,
  RTC_CALENDAR_SET,
  RTC_CALENDAR_START,
  RTC_CALENDAR_STOP,
  RTC_CNTDOWN_SET,
  RTC_CNTDOWN_START,
  RTC_CNTDOWN_STOP,
  RTC_CNTDOWN_VALUE,
  RTC_CALIBRATION,
  RTC_GET_TIME
}rt_rtc_cmd_e;

typedef enum{
  PER_SEC = 3,
  PER_MIN = 4,
  PER_HOUR = 5,
  PER_DAY = 6,
  PER_MON = 7,
  PER_YERA = 8
}rt_alarm_mode_e;

typedef struct{
  unsigned int      time;
  unsigned int      date;
}__time_date_t;

typedef __time_date_t rt_rtc_calendar_t;

typedef struct{
  __time_date_t     time_date;
  unsigned int      repeat_mode;
}rt_rtc_alarm_t;

typedef struct{
  unsigned int      value;
  unsigned int      repeat_en;
}rt_rtc_cntDwn_t;

typedef struct{
  unsigned int      mode;
  unsigned int      clkDivider;
  rt_rtc_calendar_t calendar;
  rt_rtc_alarm_t    alarm;
  rt_rtc_cntDwn_t   cntDwn;
}rt_rtc_conf_t;

typedef struct{
  rt_dev_t          *dev;
  rt_event_t        *event;
  rt_rtc_conf_t     conf;
}rt_rtc_t;


rt_rtc_t* rt_rtc_open(rt_rtc_conf_t *rtc_conf, rt_event_t *event);
void rt_rtc_close(rt_rtc_t *rtc);
void rt_rtc_control( rt_rtc_t *rtc, rt_rtc_cmd_e rtc_cmd, void *value, rt_event_t *event );

/// @endcond

#endif
