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

#include "tabsgroup.h"
#include "mainwindow.h"
#include "keyboard_number.h"
#include "keyboard_text.h"
#include "keyboard_curve.h"
#include "opentx.h" // TODO for constants...

TabsGroupHeader::TabsGroupHeader(TabsGroup * parent):
  Window(parent, { 0, 0, LCD_W, MENU_BODY_TOP }, OPAQUE),
  back(this, { 0, 0, TOPBAR_BUTTON_WIDTH, TOPBAR_BUTTON_WIDTH }, ICON_BACK,
       [&]() -> uint8_t {
        bool switching = false;
        if (menu) {
          switching = menu->switching;
          if (!switching && menu->currentTab) {
            menu->switching = switching = menu->currentTab->leave(std::bind(&TabsGroupHeader::deleteMenu, this));
          }
          if (!switching) {
            deleteMenu();
          }
        }
        return !switching;
       }, BUTTON_NOFOCUS),
  carousel(this, parent)
{
  menu = parent;
}

void TabsGroupHeader::deleteMenu() {
  menu->switching = false;
  menu->deleteLater();
  menu = nullptr;
}

void TabsGroupHeader::paint(BitmapBuffer * dc)
{
  dc->drawSolidFilledRect(0, MENU_HEADER_HEIGHT, LCD_W, MENU_TITLE_TOP - MENU_HEADER_HEIGHT, TEXT_BGCOLOR); // the white separation line
  dc->drawSolidFilledRect(0, MENU_TITLE_TOP, LCD_W, MENU_TITLE_HEIGHT, TITLE_BGCOLOR); // the title line background
  if (title) {
    lcdDrawText(MENUS_MARGIN_LEFT, MENU_TITLE_TOP, title, MENU_TITLE_COLOR);
  }
}

TabsCarousel::TabsCarousel(Window * parent, TabsGroup * menu):
  Window(parent, { TOPBAR_BUTTON_WIDTH, 0, LCD_W - TOPBAR_BUTTON_WIDTH, MENU_HEADER_HEIGHT }, OPAQUE),
  menu(menu)
{
}

void TabsCarousel::updateInnerWidth()
{
  setInnerWidth(padding_left + TOPBAR_BUTTON_WIDTH * menu->tabs.size());
}

void TabsCarousel::paint(BitmapBuffer * dc)
{
  dc->drawSolidFilledRect(0, 0, padding_left, TOPBAR_BUTTON_WIDTH, HEADER_BGCOLOR);
  for (unsigned i=0; i<menu->tabs.size(); i++) {
    dc->drawBitmap(padding_left + i*TOPBAR_BUTTON_WIDTH, 0, theme->getIconBitmap(menu->tabs[i]->icon, currentPage == i));
  }
  coord_t x = padding_left + TOPBAR_BUTTON_WIDTH * menu->tabs.size();
  coord_t w = width() - x;
  if (w > 0) {
    dc->drawSolidFilledRect(x, 0, w, TOPBAR_BUTTON_WIDTH, HEADER_BGCOLOR);
  }
}

bool TabsCarousel::onTouchEnd(coord_t x, coord_t y)
{
  unsigned index = (x - padding_left) / TOPBAR_BUTTON_WIDTH;
  menu->setCurrentTab(index);
  currentPage = index;
  AUDIO_KEY_PRESS();
  return true;
}

TabsGroup::TabsGroup():
  Window(&mainWindow, { 0, 0, LCD_W, LCD_H }, OPAQUE),
  header(this),
  body(this, { 0, MENU_BODY_TOP, LCD_W, MENU_BODY_HEIGHT })
{
}

TabsGroup::~TabsGroup()
{
  clearFocus();
  currentTab = nullptr;
  for (auto tab: tabs) {
    delete tab;
  }

  body.deleteChildren();

  TextKeyboard::instance()->disable(false);
  NumberKeyboard::instance()->disable(false);
  CurveKeyboard::instance()->disable(false);
}

void TabsGroup::addTab(PageTab * page)
{
  tabs.push_back(page);
  if (!currentTab) {
    setCurrentTab(page);
  }
  header.carousel.updateInnerWidth();
}

void TabsGroup::removeTab(unsigned index)
{
  if (currentTab == tabs[index]) {
    setCurrentTab(max<unsigned>(0, index - 1));
  }

  tabs.erase(tabs.begin() + index);
  g_model.view  = 0;
}

void TabsGroup::setCurrentTab(PageTab * tab)
{
  if (tab != currentTab && !switching) {
    clearFocus();
    body.clear();
    TextKeyboard::instance()->disable(false);
    NumberKeyboard::instance()->disable(false);
    CurveKeyboard::instance()->disable(false);
    if (currentTab) {
      switching = currentTab->leave([=]() {
        currentTab = tab;
        switchTab();
      });
    }
    if (!switching) {
      currentTab = tab;
      switchTab();
    }
  }
}
void TabsGroup::switchTab() {
  if (currentTab) {
    currentTab->build(&body);
    header.setTitle(currentTab->title.c_str());
    invalidate();
  }
  switching = false;
}

void TabsGroup::checkEvents()
{
  Window::checkEvents();
  if (currentTab)
    currentTab->checkEvents();
}

void TabsGroup::paint(BitmapBuffer * dc)
{
  dc->clear(TEXT_BGCOLOR);
}

bool TabsGroup::onTouchEnd(coord_t x, coord_t y)
{
  if (Window::onTouchEnd(x, y))
    return true;

  TextKeyboard::instance()->disable(true);
  NumberKeyboard::instance()->disable(true);
  CurveKeyboard::instance()->disable(true);
  return true;
}
