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

#ifndef _SWITCH_WARN_DIALOG_H_
#define _SWITCH_WARN_DIALOG_H_

#include "dialog.h"
#include "mainwindow.h"
#include "opentx.h"
#include "gridlayout.h"

class SwitchWarnDialog : public Dialog {
  public:
    SwitchWarnDialog()
      : Dialog(WARNING_TYPE_ALERT, STR_SWITCHWARN) {
      last_bad_switches = 0xff;
      states = g_model.switchWarningState;
      bad_pots = 0;
      last_bad_pots = 0xff;
      setCloseCondition(std::bind(&SwitchWarnDialog::warningInactive, this));
    }


    ~SwitchWarnDialog() override;


#if defined(DEBUG_WINDOWS)
    std::string getName() override
    {
      return "SwitchWarnDialog";
    }
#endif


    void paint(BitmapBuffer * dc) override;
  protected:
    bool warningInactive();
  protected:
    swarnstate_t last_bad_switches;
    swarnstate_t states;
    uint8_t bad_pots;
    uint8_t last_bad_pots;
};

#endif
