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

#ifndef _BUTTON_H_
#define _BUTTON_H_

#include "window.h"
#include <string>

enum ButtonFlags {
  BUTTON_BACKGROUND = 1,
  BUTTON_CHECKED = 2,
  BUTTON_NOFOCUS = 4,
  BUTTON_CHECKED_ON_FOCUS = 8,
  BUTTON_DISABLED = 16,
  BUTTON_RESULT = 128
};

class Button : public Window {
  public:

    Button(Window * parent, const rect_t & rect, std::function<uint8_t(void)> onPress=nullptr, uint8_t flags=0):
      Window(parent, rect),
      onPress(onPress),
      flags(flags),
      dialogResult(0)
    {
    }

#if defined(DEBUG_WINDOWS)
    std::string getName() override
    {
      return "Button";
    }
#endif

    void enable(bool enabled=true)
    {
      if (!enabled != bool(flags & BUTTON_DISABLED)) {
        flags ^= BUTTON_DISABLED;
        invalidate();
      }
    }

    void disable()
    {
      enable(false);
    }

    void check(bool checked=true)
    {
      if (checked != bool(flags & BUTTON_CHECKED)) {
        flags ^= BUTTON_CHECKED;
        invalidate();
      }
    }

    bool enabled()
    {
      return !(flags & BUTTON_DISABLED);
    }

    bool checked()
    {
      if (flags & BUTTON_CHECKED_ON_FOCUS)
        return hasFocus();
      else
        return flags & BUTTON_CHECKED;
    }

    void setPressHandler(std::function<uint8_t(void)> handler)
    {
      onPress = std::move(handler);
    }

    void setCheckHandler(std::function<void(void)> handler)
    {
      onCheck = std::move(handler);
    }

    bool onTouchEnd(coord_t x, coord_t y) override;

    void checkEvents() override;

    event_t getResult();

  protected:
    std::function<uint8_t(void)> onPress;
    std::function<void(void)> onCheck;
    uint8_t flags;
    event_t dialogResult;
};

class TextButton : public Button {
  public:
    TextButton(Window * parent, const rect_t & rect, std::string text, event_t dialogResult) :
      TextButton(parent, rect, text) {
        this->dialogResult = dialogResult;
    }
    TextButton(Window * parent, const rect_t & rect, std::string text, std::function<uint8_t(void)> onPress=nullptr, uint8_t flags=BUTTON_BACKGROUND):
      Button(parent, rect, onPress, flags),
      text(std::move(text))
    {
      windowFlags = OPAQUE;
    }
#if defined(DEBUG_WINDOWS)
    std::string getName() override
    {
      return "TextButton \"" + text + "\"";
    }
#endif

    void setText(std::string value)
    {
      if (value != text) {
        text = std::move(value);
        invalidate();
      }
    }


    void paint(BitmapBuffer * dc) override;

  protected:
    std::string text;
};

class IconButton: public Button {
  public:
    IconButton(Window * parent, const rect_t & rect, uint8_t icon, std::function<uint8_t(void)> onPress, uint8_t flags=0):
      Button(parent, rect, onPress, flags),
      icon(icon)
    {
    }

#if defined(DEBUG_WINDOWS)
    std::string getName() override
    {
      return "IconButton(" + std::to_string(icon) + ")";
    }
#endif

    void paint(BitmapBuffer * dc) override;

  protected:
    uint8_t icon;
};
enum FileType {
  Audio,
  Gfx,
  Bin,
  Text,
  Other
};

class FileButton: public Button {
  public:
    FileButton(Window * parent, const rect_t & rect, std::string text, bool folder, std::function<uint8_t(void)> onPress, uint8_t flags=0):
      Button(parent, rect, onPress, flags), text(std::move(text)), folder(folder)
    {

    }

#if defined(DEBUG_WINDOWS)
    std::string getName() override
    {
      return "FileButton(" + std::to_string(icon) + ")";
    }
#endif

    void paint(BitmapBuffer * dc) override;

  protected:
    static const std::list<std::string> fileTypeBin;
    static const std::list<std::string> fileTypeAudio;
    static const std::list<std::string> fileTypeGfx;
    static const std::list<std::string> fileTypeTxt;
    void setColorByExtension();

    uint8_t icon;
    std::string text;
    bool folder;
};

class FabIconButton: public Button {
  public:
    FabIconButton(Window * parent, coord_t x, coord_t y, uint8_t icon, std::function<uint8_t(void)> onPress, uint8_t flags=0);

#if defined(DEBUG_WINDOWS)
    std::string getName() override
    {
      return "FabIconButton(" + std::to_string(icon) + ")";
    }
#endif

    void paint(BitmapBuffer * dc) override;

  protected:
    uint8_t icon;
};

#endif
