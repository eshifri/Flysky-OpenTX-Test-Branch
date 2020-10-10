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

#ifndef _GRIDLAYOUT_H_
#define _GRIDLAYOUT_H_

#include "window.h"

static constexpr uint8_t lineSpacing = 6;
static constexpr uint8_t indentWidth = 10;
#define lineHeight ((g_eeGeneral.displayLargeLines) ? 39 : 26)

class GridLayout {
  public:
    GridLayout()
    {
    }

    void setLabelWidth(coord_t value)
    {
      labelWidth = value;
    }

    void setMarginLeft(coord_t value)
    {
      lineMarginLeft = value;
    }

    void setMarginRight(coord_t value)
    {
      lineMarginRight = value;
    }

    rect_t getLineSlot()
    {
      return { lineMarginLeft, currentY, LCD_W - lineMarginRight - lineMarginLeft, lineHeight };
    }

    rect_t getLabelSlot(bool indent = false) const
    {
      coord_t left = indent ? lineMarginLeft + indentWidth : lineMarginLeft;
      return { left, currentY, labelWidth - left, lineHeight };
    }

    rect_t getFieldSlot(uint8_t count = 1, uint8_t index = 0) const
    {
      coord_t width = (LCD_W - labelWidth - lineMarginRight - (count - 1) * lineSpacing) / count;
      coord_t left = labelWidth + (width + lineSpacing) * index;
      return {left, currentY, width, lineHeight};
    }

    void addWindow(Window * window)
    {
      window->adjustHeight();
      currentY += window->rect.h + lineSpacing;
    }

    void spacer(coord_t height=lineSpacing)
    {
      currentY += height;
    }

    void nextLine(coord_t height=lineHeight)
    {
      spacer(height + lineSpacing);
    }

    coord_t getWindowHeight() const
    {
      return currentY;
    }

  protected:
    coord_t currentY = 0;
    coord_t labelWidth = 140;
    coord_t lineMarginLeft = 6;
    coord_t lineMarginRight = 10;
};


class GridNxMLayout {
  public:
    GridNxMLayout(coord_t col, coord_t row) : columns(col), rows(row), index(0)
    {
      currentY += topMargin;
    }

    rect_t getNextFieldSlot()
    {
      coord_t width = (LCD_W - (lineMarginRight + lineMarginRight + (columns - 1) * lineSpacing)) / columns;
      coord_t height = (LCD_H - (topMargin + bottomMargin + (rows - 1) * lineSpacing)) / rows;
      coord_t left = lineMarginLeft + ((width + lineSpacing) * (index % columns));
      if (index && !(index % columns)) {
        if (columns == rows) currentY += width + lineSpacing;
        else currentY += height + lineSpacing;
      }
      index++;
      return {left, currentY, width, width};
    }

    void spacer(coord_t height=lineSpacing)
    {
      currentY += height;
    }

    coord_t getWindowHeight() const
    {
      return currentY;
    }

  protected:
    coord_t currentY = 0;
    coord_t topMargin = 8;
    coord_t bottomMargin = 8;
    coord_t lineMarginLeft = 6;
    coord_t lineMarginRight = 10;
    coord_t columns;
    coord_t rows;
    coord_t index;
};

#endif
