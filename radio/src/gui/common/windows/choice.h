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

#ifndef _CHOICE_H_
#define _CHOICE_H_

#include "window.h"
#include <string>


class CustomCurveChoice : public Window {
  public:
    CustomCurveChoice(Window * parent, const rect_t & rect, int16_t vmin, int16_t vmax, std::function<int16_t()> getValue, std::function<void(int16_t)> setValue, LcdFlags flags = 0);

#if defined(DEBUG_WINDOWS)
    std::string getName() override
    {
      return "CustomCurveChoice";
    }
#endif

    void paint(BitmapBuffer * dc) override;

    bool onTouchEnd(coord_t x, coord_t y) override;

  protected:
    int16_t vmin;
    int16_t vmax;
    std::function<int16_t()> getValue;
    std::function<void(int16_t)> setValue;
    LcdFlags flags;
};

class ChoiceBase {
public:
  ChoiceBase(const char * values, int16_t vmin, int16_t vmax, std::function<int16_t()> getValue, std::function<void(int16_t)> setValue, LcdFlags flags = 0);
  void setAvailableHandler(std::function<bool(int)> handler)
  {
    isValueAvailable = std::move(handler);
  }

  void setTextHandler(std::function<std::string(int32_t)> handler)
  {
    textHandler = std::move(handler);
  }
  void setReadOnly(bool readOnly) {
    this->readOnly = readOnly;
  }
  int16_t getMax() {return vmax;}
  void setMax(int16_t value) {vmax = value;}
protected:
  void paintChoice(BitmapBuffer * dc, bool hasFocus, const rect_t rect);
  bool handleTouchEnd(coord_t x, coord_t y);

  const char * values;
  int16_t vmin;
  int16_t vmax;
  std::function<int16_t()> getValue;
  std::function<void(int16_t)> setValue;
  std::function<bool(int)> isValueAvailable;
  std::function<std::string(int32_t)> textHandler;
  LcdFlags flags;
  bool readOnly;
};

class Choice : public Window, public ChoiceBase {
  public:
    Choice(Window * parent, const rect_t & rect, const char * values, int16_t vmin, int16_t vmax, std::function<int16_t()> getValue, std::function<void(int16_t)> setValue, LcdFlags flags = 0);

#if defined(DEBUG_WINDOWS)
    std::string getName() override
    {
      return "Choice";
    }
#endif

    void paint(BitmapBuffer * dc) override;

    bool onTouchEnd(coord_t x, coord_t y) override;
};

#endif // _CHOICE_H_
