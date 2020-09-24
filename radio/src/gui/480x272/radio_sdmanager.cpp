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

#include "radio_sdmanager.h"
#include "opentx.h"
#include "libwindows.h"
#include "page.h"
// #include "io/frsky_firmware_update.h"
#include "io/multi_firmware_update.h"

class FileNameEditWindow : public Page {
  public:
    FileNameEditWindow(const std::string name) :
      Page(),
      name(std::move(name))
    {
      buildBody(&body);
      buildHeader(&header);
    };

  protected:
    const std::string name;

    void buildHeader(Window * window)
    {
      new StaticText(window, {70, 4, 180, 30}, STR_RENAME_FILE, MENU_TITLE_COLOR);
    }

    void buildBody(Window * window)
    {
      GridLayout grid;
      grid.spacer(8);
	  
      const char * ext = getFileExtension(name.c_str());
	  std::string extStr(ext);
	  int nameLength = name.length() - extStr.length();
	  if(nameLength > SD_SCREEN_FILE_LENGTH)
		  nameLength = SD_SCREEN_FILE_LENGTH;

	  strncpy(reusableBuffer.sdmanager.originalName, name.c_str(), nameLength);
	  reusableBuffer.sdmanager.originalName[nameLength] = 0;

	  new TextEdit(window, grid.getLineSlot(), reusableBuffer.sdmanager.originalName, SD_SCREEN_FILE_LENGTH, 0, [=](char* newValue) {
		    std::string newName(newValue);
			newName += extStr;
			f_rename(name.c_str(), newName.c_str() );
		}, 
		false);	  
	}
};

RadioSdManagerPage::RadioSdManagerPage():
  PageTab(SD_IS_HC() ? STR_SDHC_CARD : STR_SD_CARD, ICON_RADIO_SD_BROWSER)
{
}

void RadioSdManagerPage::rebuild(Window * window)
{
  coord_t scrollPosition = window->getScrollPositionY();
  window->clear();
  build(window);
  window->setScrollPositionY(scrollPosition);
}

// comparison, not case sensitive.
bool compare_nocase(const std::string &first, const std::string &second)
{
  return strcasecmp(first.c_str(), second.c_str()) < 0;
}

char * getFullPath(const std::string & filename)
{
  static char full_path[_MAX_LFN+1]; // TODO optimize that!
  f_getcwd(full_path, _MAX_LFN);
  strcat(full_path, PSTR("/"));
  strcat(full_path, filename.c_str());
  return full_path;
}

const char * getBasename(const char * path)
{
  for (int8_t i = strlen(path) - 1; i >= 0; i--) {
    if (path[i] == '/') {
      return &path[i + 1];
    }
  }
  return path;
}

void RadioSdManagerPage::build(Window * window)
{
  GridLayout grid;
  grid.spacer(8);

  FILINFO fno;
  DIR dir;
  std::list<std::string> files;
  std::list<std::string> directories;

  FRESULT res = f_opendir(&dir, "."); // Open the directory
  if (res == FR_OK) {
    // read all entries
    bool firstTime = true;
    for (;;) {
      res = sdReadDir(&dir, &fno, firstTime);

      if (res != FR_OK || fno.fname[0] == 0)
        break; // Break on error or end of dir
      if (strlen(fno.fname) > SD_SCREEN_FILE_LENGTH)
        continue;
      if (fno.fname[0] == '.' && fno.fname[1] != '.')
        continue; // Ignore hidden files under UNIX, but not ..

      if (fno.fattrib & AM_DIR) {
        directories.push_back(fno.fname);
      }
      else {
        files.push_back(fno.fname);
      }
    }

    // sort directories and files
    directories.sort(compare_nocase);
    files.sort(compare_nocase);

    for (auto name: directories) {
      new TextButton(window, grid.getLineSlot(), name, [=]() -> uint8_t {
        f_chdir(name.data());
        window->clear();
        build(window);
        return 0;
      });
      grid.nextLine();
    }
    
    for (auto name: files) {
      new TextButton(window, grid.getLineSlot(), name, [=]() -> uint8_t {
        auto menu = new Menu();
        const char * ext = getFileExtension(name.data());
        if (ext) {
          if (!strcasecmp(ext, SOUNDS_EXT)) {
            menu->addLine(STR_PLAY_FILE, [=]() {
              audioQueue.stopAll();
              audioQueue.playFile(getFullPath(name), 0, ID_PLAY_FROM_SD_MANAGER);
            });
          }
          else if (isExtensionMatching(ext, BITMAPS_EXT)) {
            // TODO
          }
          else if (!strcasecmp(ext, TEXT_EXT)) {
            menu->addLine(STR_VIEW_TEXT, [=]() {
              // TODO
            });
          }
          else if (!READ_ONLY() && !strcasecmp(ext, SPORT_FIRMWARE_EXT)) {
            menu->addLine(STR_FLASH_EXTERNAL_DEVICE, [=]() {
              sportFlashDevice(EXTERNAL_MODULE, getFullPath(name));
            });
          }
#if defined(LUA)
          else if (isExtensionMatching(ext, SCRIPTS_EXT)) {
            menu->addLine(STR_EXECUTE_FILE, [=]() {
              luaExec(getFullPath(name));
            });
          }
#endif
#if defined(MULTIMODULE)
          else if (!READ_ONLY() && !strcasecmp(ext, MULTI_FIRMWARE_EXT)) {
            char* fullPath = getFullPath(name);
            MultiFirmwareInformation information;
            if (information.readMultiFirmwareInformation(fullPath) == nullptr) {
#if defined(INTERNAL_MODULE_MULTI)
              menu->addLine(STR_FLASH_INTERNAL_MULTI, [=]() {
                multiFlashFirmware(INTERNAL_MODULE, fullPath);
                runProgressScreen();
              });
#endif        
              menu->addLine(STR_FLASH_EXTERNAL_MULTI, [=]() {
                multiFlashFirmware(EXTERNAL_MODULE, fullPath);
                runProgressScreen();
              });
            }
          }
#endif
        }
        if (!READ_ONLY()) {
          menu->addLine(STR_COPY_FILE, [=]() {
            clipboard.type = CLIPBOARD_TYPE_SD_FILE;
            f_getcwd(clipboard.data.sd.directory, CLIPBOARD_PATH_LEN);
            strncpy(clipboard.data.sd.filename, name.c_str(), CLIPBOARD_PATH_LEN-1);
          });
          if (clipboard.type == CLIPBOARD_TYPE_SD_FILE) {
            menu->addLine(STR_PASTE, [=]() {
              TCHAR lfn[_MAX_LFN+1];
			        f_getcwd(lfn, _MAX_LFN);
              
			        if (strcmp(clipboard.data.sd.directory, lfn)) {  // prevent copying to the same directory
				        POPUP_WARNING(sdCopyFile(clipboard.data.sd.filename, clipboard.data.sd.directory, clipboard.data.sd.filename, lfn));
				        clipboard.type = CLIPBOARD_TYPE_NONE;
			        }
			        rebuild(window);
            });
          }
          menu->addLine(STR_RENAME_FILE, [=]() {
            new FileNameEditWindow(name);
			      rebuild(window);
          });
          menu->addLine(STR_DELETE_FILE, [=]() {
            f_unlink(getFullPath(name));
            rebuild(window);
          });
        }
        return 0;
      });
      grid.nextLine();
    }
  }

  window->setInnerHeight(grid.getWindowHeight());
}

#if 0
bool menuRadioSdManagerInfo(event_t event)
{
  SIMPLE_SUBMENU(STR_SD_INFO_TITLE, ICON_RADIO_SD_BROWSER, 1);

  lcdDrawText(MENUS_MARGIN_LEFT, 2*FH, STR_SD_TYPE);
  lcdDrawText(100, 2*FH, SD_IS_HC() ? STR_SDHC_CARD : STR_SD_CARD);

  lcdDrawText(MENUS_MARGIN_LEFT, 3*FH, STR_SD_SIZE);
  lcdDrawNumber(100, 3*FH, sdGetSize(), LEFT, 0, NULL, "M");

  lcdDrawText(MENUS_MARGIN_LEFT, 4*FH, STR_SD_SECTORS);
#if defined(SD_GET_FREE_BLOCKNR)
  lcdDrawNumber(100, 4*FH, SD_GET_FREE_BLOCKNR()/1000, LEFT, 0, NULL, "/");
  lcdDrawNumber(150, 4*FH, sdGetNoSectors()/1000, LEFT);
#else
  lcdDrawNumber(100, 4*FH, sdGetNoSectors()/1000, LEFT, 0, NULL, "k");
#endif

  lcdDrawText(MENUS_MARGIN_LEFT, 5*FH, STR_SD_SPEED);
  lcdDrawNumber(100, 5*FH, SD_GET_SPEED()/1000, LEFT, 0, NULL, "kb/s");

  return true;
}
#endif

#if 0
void onSdManagerMenu(const char * result)
{
  TCHAR lfn[_MAX_LFN+1];

  // TODO possible buffer overflows here!

  uint8_t index = menuVerticalPosition-menuVerticalOffset;
  char *line = reusableBuffer.sdmanager.lines[index];

  if (result == STR_SD_INFO) {
    pushMenu(menuRadioSdManagerInfo);
  }
  else if (result == STR_SD_FORMAT) {
    POPUP_CONFIRMATION(STR_CONFIRM_FORMAT);
  }
  else if (result == STR_PASTE) {
    f_getcwd(lfn, _MAX_LFN);
    // if destination is dir, copy into that dir
    if (IS_DIRECTORY(line)) {
      strcat(lfn, PSTR("/"));
      strcat(lfn, line);
    }
    if (strcmp(clipboard.data.sd.directory, lfn)) {  // prevent copying to the same directory
      POPUP_WARNING(sdCopyFile(clipboard.data.sd.filename, clipboard.data.sd.directory, clipboard.data.sd.filename, lfn));
      REFRESH_FILES();
    }
  }
  else if (result == STR_RENAME_FILE) {
    memcpy(reusableBuffer.sdmanager.originalName, line, sizeof(reusableBuffer.sdmanager.originalName));
    uint8_t fnlen = 0, extlen = 0;
    getFileExtension(line, 0, LEN_FILE_EXTENSION_MAX, &fnlen, &extlen);
    // write spaces to allow extending the length of a filename
    memset(line + fnlen - extlen, ' ', SD_SCREEN_FILE_LENGTH - fnlen + extlen);
    line[SD_SCREEN_FILE_LENGTH-extlen] = '\0';
    s_editMode = EDIT_MODIFY_STRING;
    editNameCursorPos = 0;
  }
  else if (result == STR_ASSIGN_BITMAP) {
    memcpy(g_model.header.bitmap, line, sizeof(g_model.header.bitmap));
    storageDirty(EE_MODEL);
  }
  else if (result == STR_ASSIGN_SPLASH) {
    f_getcwd(lfn, _MAX_LFN);
    sdCopyFile(line, lfn, SPLASH_FILE, BITMAPS_PATH);
  }
  else if (result == STR_VIEW_TEXT) {
    getSelectionFullPath(lfn);
    pushMenuTextView(lfn);
  }
}
#endif

#if 0
  // TODO bitmap preview
  const char * ext = getFileExtension(reusableBuffer.sdmanager.lines[index]);
  if (ext && isExtensionMatching(ext, BITMAPS_EXT)) {
    if (currentBitmapIndex != menuVerticalPosition) {
      currentBitmapIndex = menuVerticalPosition;
      delete currentBitmap;
      currentBitmap = BitmapBuffer::load(reusableBuffer.sdmanager.lines[index]);
    }
    if (currentBitmap) {
      uint16_t height = currentBitmap->getHeight();
      uint16_t width = currentBitmap->getWidth();
      if (height > MENU_BODY_HEIGHT-10) {
        height = MENU_BODY_HEIGHT - 10;
      }
      if (width > LCD_W/2) {
        width = LCD_W/2;
      }
      lcd->drawScaledBitmap(currentBitmap, LCD_W / 2 - 20 + LCD_W/4 - width/2, MENU_BODY_TOP + MENU_BODY_HEIGHT/2 - height/2, width, height);
    }
  }
#endif
