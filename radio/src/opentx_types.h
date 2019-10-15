/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _OTXTYPES_H_
#define _OTXTYPES_H_

#include <cstring>

typedef int16_t gvar_t;

#if defined(CPUARM)
  typedef uint32_t tmr10ms_t;
  typedef int32_t rotenc_t;
  typedef int32_t getvalue_t;
  typedef uint32_t mixsrc_t;
  typedef int32_t swsrc_t;
  typedef int16_t safetych_t;
  typedef uint16_t bar_threshold_t;
#else
  typedef uint16_t tmr10ms_t;
  typedef int8_t rotenc_t;
  typedef int16_t getvalue_t;
  typedef uint8_t mixsrc_t;
  typedef int8_t swsrc_t;
  typedef int8_t safetych_t;
  typedef uint8_t bar_threshold_t;
#endif

#if defined(CPUARM)
typedef uint16_t event_t;
#else
typedef uint8_t event_t;
#endif
#define EVENT_PARAMS_COUNT 2
typedef int32_t event_param_t;

struct event_ext_t {
  event_t evt;
  //for now int32 but can be strut or union
  //describing type to allow easier handling in LUA
  event_param_t params[EVENT_PARAMS_COUNT];

  event_ext_t() {
    clear();
  }
  int paramsCount(){
    return sizeof(params)/sizeof(params[0]);
  }

  void clear() {
    evt = 0;
    for(int i = 0; i < paramsCount(); i++){
      params[i] = 0;
    }
  }
  void set(event_t event, event_param_t* p = nullptr, int count = 0) {
    clear();
    evt = event;
    if(p && count > 0) {
      if(count > paramsCount()) {
        count = paramsCount();
      }
      memcpy(static_cast<void*>(params), static_cast<void*>(p), count * sizeof(params[0]));
    }
  }
  void set(event_ext_t* event) {
    set(event->evt, event->params, paramsCount());
  }
  event_param_t x(){
    return params[0];
  }
  event_param_t y(){
    return params[1];
  }
  void setX(event_param_t x) {
    params[0] = x;
  }
  void setY(event_param_t y) {
    params[1] = y;
  }
};


typedef struct event_ext_t event_ext_t;



typedef int32_t putstime_t;
typedef int32_t coord_t;
typedef uint32_t LcdFlags;

typedef uint16_t FlightModesType;

#endif // _OTXTYPES_H_
