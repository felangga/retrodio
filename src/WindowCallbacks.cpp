/*
 * WindowCallbacks.cpp - Window Event Handlers Implementation
 *
 * Copyright (c) 2025 felangga
 *
 * This file implements window callback functions
 */

#include "WindowCallbacks.h"
#include "GlobalState.h"
#include "StationManager.h"
#include "AudioHandlers.h"
#include "wt32_sc01_plus.h"
#include "ConfigManager.h"
#include "UIHelpers.h"

void onWindowMinimize() {
  handleWindowMinimize(lcd, radioWindow, &radioIcon);
}

void onWindowClose() {
  handleWindowClose(lcd, radioWindow, &radioIcon);
}

void onRadioIconClick() {
  handleIconClick(lcd, radioWindow);
}

void onWindowContentClick(int relativeX, int relativeY) {
  handleWindowContentClick(lcd, radioWindow, relativeX, relativeY);
}

void onWindowMoved() {
  handleWindowMoved(lcd, radioWindow);
}

void onStationWindowMinimize() {
  stationWindow.visible = false;
  stationWindow.active = false;
  radioWindow.visible = true;
  radioWindow.active = true;
  drawCheckeredPatternArea(lcd, stationWindow.x, stationWindow.y, stationWindow.w + 5, stationWindow.h + 5);
  drawWindow(lcd, radioWindow);
}

void onStationWindowClose() {
  stationWindow.visible = false;
  stationWindow.active = false;
  radioWindow.visible = true;
  radioWindow.active = true;
  drawCheckeredPatternArea(lcd, stationWindow.x, stationWindow.y, stationWindow.w + 5, stationWindow.h + 5);
  drawWindow(lcd, radioWindow);
}

void onStationWindowContentClick(int relativeX, int relativeY) {
  handleWindowContentClick(lcd, stationWindow, relativeX, relativeY);
}

void onStationWindowMoved() {
  handleWindowMoved(lcd, stationWindow);
}

void onAddStationWindowMinimize() {
  if (globalKeyboard) {
    MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
    keyboard->visible = false;
  }

  // Restore window position before hiding
  adjustWindowForKeyboard(addStationWindow, nullptr, false);

  addStationWindow.visible = false;
  addStationWindow.active = false;
  stationWindow.visible = true;
  stationWindow.active = true;
  drawCheckeredPatternArea(lcd, addStationWindow.x, addStationWindow.y, addStationWindow.w + 5, addStationWindow.h + 5);
  drawWindow(lcd, stationWindow);
}

void onAddStationWindowClose() {
  if (globalKeyboard) {
    MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
    keyboard->visible = false;
  }

  // Restore window position before hiding
  adjustWindowForKeyboard(addStationWindow, nullptr, false);

  addStationWindow.visible = false;
  addStationWindow.active = false;
  stationWindow.visible = true;
  stationWindow.active = true;
  drawCheckeredPatternArea(lcd, addStationWindow.x, addStationWindow.y, addStationWindow.w + 5, addStationWindow.h + 5);
  drawWindow(lcd, stationWindow);
}

void onAddStationWindowContentClick(int relativeX, int relativeY) {
  extern const int INPUT_STATION_NAME;
  extern const int INPUT_STATION_URL;
  MacComponent* nameInputComp = nullptr;
  MacComponent* urlInputComp = nullptr;

  for (int i = 0; i < addStationWindow.childComponentCount; i++) {
    MacComponent* comp = addStationWindow.childComponents[i];
    if (comp->id == INPUT_STATION_NAME) {
      nameInputComp = comp;
    } else if (comp->id == INPUT_STATION_URL) {
      urlInputComp = comp;
    }
  }

  if (globalKeyboard && nameInputComp && urlInputComp) {
    MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;

    if (relativeX >= nameInputComp->x && relativeX <= nameInputComp->x + nameInputComp->w &&
        relativeY >= nameInputComp->y && relativeY <= nameInputComp->y + nameInputComp->h) {
      MacInputField* nameInput = (MacInputField*)nameInputComp->customData;
      MacInputField* urlInput = (MacInputField*)urlInputComp->customData;

      nameInput->focused = true;
      urlInput->focused = false;
      keyboard->targetInputId = INPUT_STATION_NAME;
      keyboard->visible = true;

      // Adjust window position to reveal input field
      adjustWindowForKeyboard(addStationWindow, nameInputComp, true);

      drawComponent(lcd, *nameInputComp, addStationWindow.x, addStationWindow.y);
      drawComponent(lcd, *urlInputComp, addStationWindow.x, addStationWindow.y);
      drawComponent(lcd, *globalKeyboard, 0, 0);

      int tx, ty;
      delay(150);
      while (lcd.getTouch(&tx, &ty)) {
        delay(10);
      }
      return;
    }

    if (relativeX >= urlInputComp->x && relativeX <= urlInputComp->x + urlInputComp->w &&
        relativeY >= urlInputComp->y && relativeY <= urlInputComp->y + urlInputComp->h) {
      MacInputField* nameInput = (MacInputField*)nameInputComp->customData;
      MacInputField* urlInput = (MacInputField*)urlInputComp->customData;

      nameInput->focused = false;
      urlInput->focused = true;
      keyboard->targetInputId = INPUT_STATION_URL;
      keyboard->visible = true;

      // Adjust window position to reveal input field
      adjustWindowForKeyboard(addStationWindow, urlInputComp, true);

      drawComponent(lcd, *nameInputComp, addStationWindow.x, addStationWindow.y);
      drawComponent(lcd, *urlInputComp, addStationWindow.x, addStationWindow.y);
      drawComponent(lcd, *globalKeyboard, 0, 0);

      int tx, ty;
      delay(150);
      while (lcd.getTouch(&tx, &ty)) {
        delay(10);
      }
      return;
    }

    if (keyboard->visible) {
      keyboard->visible = false;

      if (nameInputComp && nameInputComp->customData) {
        MacInputField* nameInput = (MacInputField*)nameInputComp->customData;
        nameInput->focused = false;
      }
      if (urlInputComp && urlInputComp->customData) {
        MacInputField* urlInput = (MacInputField*)urlInputComp->customData;
        urlInput->focused = false;
      }

      // Restore window to original position
      adjustWindowForKeyboard(addStationWindow, nullptr, false);

      int keyboardHeight = screenHeight / 2;
      int keyboardY = screenHeight - keyboardHeight;
      drawCheckeredPatternArea(lcd, 0, keyboardY, screenWidth, keyboardHeight);

      drawWindow(lcd, addStationWindow);
    }
  }

  handleWindowContentClick(lcd, addStationWindow, relativeX, relativeY);
}

void onAddStationWindowMoved() {
  handleWindowMoved(lcd, addStationWindow);
}

void onConfirmDeleteWindowMinimize() {
  confirmDeleteWindow.visible = false;
  confirmDeleteWindow.active = false;
  stationWindow.visible = true;
  stationWindow.active = true;
  drawCheckeredPatternArea(lcd, confirmDeleteWindow.x, confirmDeleteWindow.y, confirmDeleteWindow.w + 5, confirmDeleteWindow.h + 5);
  drawWindow(lcd, stationWindow);
}

void onConfirmDeleteWindowClose() {
  confirmDeleteWindow.visible = false;
  confirmDeleteWindow.active = false;
  stationWindow.visible = true;
  stationWindow.active = true;
  drawCheckeredPatternArea(lcd, confirmDeleteWindow.x, confirmDeleteWindow.y, confirmDeleteWindow.w + 5, confirmDeleteWindow.h + 5);
  drawWindow(lcd, stationWindow);
}

void onConfirmDeleteWindowContentClick(int relativeX, int relativeY) {
  handleWindowContentClick(lcd, confirmDeleteWindow, relativeX, relativeY);
}

void onConfirmDeleteWindowMoved() {
  handleWindowMoved(lcd, confirmDeleteWindow);
}

MacComponent* findComponentById(const MacWindow& window, int id);
void updateComponentSymbol(const MacWindow& window, int componentId, SymbolType newSymbol);

void onComponentClick(int componentId, void* data) {
  extern const int CMP_NOW_PLAYING_LABEL;
  extern const int CMP_VOLUME_SLIDER;
  extern const int CMP_BUFFER_PROGRESS;
  extern const int CMP_AUTO_PLAY_CHECKBOX;
  extern const int CMP_VISUALS_CHECKBOX;
  extern const int BTN_PLAY;
  extern const int BTN_STOP;
  extern const int BTN_VOL_UP;
  extern const int BTN_PREV;
  extern const int BTN_NEXT;
  extern const int BTN_VOL_DOWN;
  extern const int BTN_STATION;
  extern const int BTN_ADD_STATION;
  extern const int BTN_SAVE_STATION;
  extern const int BTN_CANCEL_ADD_STATION;
  extern const int BTN_DELETE_STATION;
  extern const int BTN_CONFIRM_YES;
  extern const int BTN_CONFIRM_NO;

  if (componentId == BTN_PLAY) {
    onPlay();
  } else if (componentId == BTN_STOP) {
    onStop();
  } else if (componentId == BTN_VOL_UP) {
    onVolUp();
  } else if (componentId == BTN_PREV) {
    onPrev();
  } else if (componentId == BTN_NEXT) {
    onNext();
  } else if (componentId == BTN_VOL_DOWN) {
    onVolDown();
  } else if (componentId == BTN_STATION) {
    radioWindow.visible = false;
    radioWindow.active = false;
    stationWindow.visible = true;
    stationWindow.active = true;
    stationWindow.minimized = false;
    drawCheckeredPatternArea(lcd, radioWindow.x, radioWindow.y, radioWindow.w + 5, radioWindow.h + 5);
    drawWindow(lcd, stationWindow);
  } else if (componentId == BTN_ADD_STATION) {
    stationWindow.visible = false;
    stationWindow.active = false;

    {
      int tx, ty;
      delay(200);
      while (lcd.getTouch(&tx, &ty)) {
        delay(10);
      }
      delay(50);
    }

    addStationWindow.visible = true;
    addStationWindow.active = true;
    addStationWindow.minimized = false;

    drawCheckeredPatternArea(lcd, stationWindow.x, stationWindow.y, stationWindow.w + 5, stationWindow.h + 5);
    drawWindow(lcd, addStationWindow);
  } else if (componentId == BTN_SAVE_STATION) {
    {
      extern const int INPUT_STATION_NAME;
      extern const int INPUT_STATION_URL;
      MacComponent* nameInputComp = findComponentById(addStationWindow, INPUT_STATION_NAME);
      MacComponent* urlInputComp = findComponentById(addStationWindow, INPUT_STATION_URL);

      if (nameInputComp && urlInputComp) {
        MacInputField* nameInput = (MacInputField*)nameInputComp->customData;
        MacInputField* urlInput = (MacInputField*)urlInputComp->customData;

        String stationName = nameInput->text;
        String stationURL = urlInput->text;

        if (stationName.length() > 0 && stationURL.length() > 0) {
          if (ConfigManager::addStation(stationName, stationURL)) {
            reloadStationList();
            initializeStationWindow();

            nameInput->text = "";
            nameInput->cursorPos = 0;
            urlInput->text = "";
            urlInput->cursorPos = 0;
          } else {
            return;
          }
        } 
      }
    }

    if (globalKeyboard) {
      MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
      keyboard->visible = false;
    }

    // Restore window position before hiding
    adjustWindowForKeyboard(addStationWindow, nullptr, false);

    addStationWindow.visible = false;
    addStationWindow.active = false;
    stationWindow.visible = true;
    stationWindow.active = true;

    drawCheckeredPatternArea(lcd, addStationWindow.x, addStationWindow.y, addStationWindow.w + 5, addStationWindow.h + 5);
    drawWindow(lcd, stationWindow);
  } else if (componentId == BTN_CANCEL_ADD_STATION) {
    if (globalKeyboard) {
      MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
      keyboard->visible = false;
    }

    // Restore window position before hiding
    adjustWindowForKeyboard(addStationWindow, nullptr, false);

    addStationWindow.visible = false;
    addStationWindow.active = false;
    stationWindow.visible = true;
    stationWindow.active = true;

    drawCheckeredPatternArea(lcd, addStationWindow.x, addStationWindow.y, addStationWindow.w + 5, addStationWindow.h + 5);
    drawWindow(lcd, stationWindow);
  } else if (componentId == BTN_DELETE_STATION) {
    // Find the station list component to get selected index
    MacComponent* stationListComp = findComponentById(stationWindow, 300);

    if (stationListComp && stationListComp->customData) {
      MacListView* listViewData = (MacListView*)stationListComp->customData;

      if (listViewData->selectedIndex >= 0 && listViewData->selectedIndex < ConfigManager::getStationCount()) {
        // Store the station index to delete
        stationToDeleteIndex = listViewData->selectedIndex;

        // Show confirmation dialog
        stationWindow.visible = false;
        stationWindow.active = false;

        initializeConfirmDeleteWindow();
        confirmDeleteWindow.visible = true;
        confirmDeleteWindow.active = true;

        drawCheckeredPatternArea(lcd, stationWindow.x, stationWindow.y, stationWindow.w + 5, stationWindow.h + 5);
        drawWindow(lcd, confirmDeleteWindow);
      }
    }
  } else if (componentId == BTN_CONFIRM_YES) {
    // User confirmed deletion
    if (stationToDeleteIndex >= 0 && stationToDeleteIndex < ConfigManager::getStationCount()) {
      // Delete the station
      if (ConfigManager::removeStation(stationToDeleteIndex)) {
        // Reload the station list
        reloadStationList();
        initializeStationWindow();
      }
    }

    // Reset the deletion index
    stationToDeleteIndex = -1;

    // Hide confirmation dialog and show station window
    confirmDeleteWindow.visible = false;
    confirmDeleteWindow.active = false;
    stationWindow.visible = true;
    stationWindow.active = true;

    drawCheckeredPatternArea(lcd, confirmDeleteWindow.x, confirmDeleteWindow.y, confirmDeleteWindow.w + 5, confirmDeleteWindow.h + 5);
    drawWindow(lcd, stationWindow);
  } else if (componentId == BTN_CONFIRM_NO) {
    // User cancelled deletion
    stationToDeleteIndex = -1;

    // Hide confirmation dialog and show station window
    confirmDeleteWindow.visible = false;
    confirmDeleteWindow.active = false;
    stationWindow.visible = true;
    stationWindow.active = true;

    drawCheckeredPatternArea(lcd, confirmDeleteWindow.x, confirmDeleteWindow.y, confirmDeleteWindow.w + 5, confirmDeleteWindow.h + 5);
    drawWindow(lcd, stationWindow);
  }
}

void onPlay() {
  extern Audio audio;
  extern QueueHandle_t audioCommandQueue;
  extern volatile bool isPlaying;
  extern String RadioURL;

  if (isPlaying) {
    AudioCommandMsg msg = {CMD_STOP, ""};
    xQueueSend(audioCommandQueue, &msg, portMAX_DELAY);
    isPlaying = false;
    updateComponentSymbol(radioWindow, 1, SYMBOL_PLAY);
  } else {
    AudioCommandMsg msg = {CMD_CONNECT, ""};
    strncpy(msg.url, RadioURL.c_str(), sizeof(msg.url) - 1);
    xQueueSend(audioCommandQueue, &msg, portMAX_DELAY);
  }
}

void onStop() {
  extern Audio audio;
  extern QueueHandle_t audioCommandQueue;
  extern volatile bool isPlaying;

  if (isPlaying && audio.isRunning()) {
    AudioCommandMsg msg = {CMD_STOP, ""};
    xQueueSend(audioCommandQueue, &msg, portMAX_DELAY);
  }

  isPlaying = false;
  updateComponentSymbol(radioWindow, 1, SYMBOL_PLAY);
}

void onVolUp() {
  extern Audio audio;

  int newVol = min(21, audio.getVolume() + 1);
  audio.setVolume(newVol);
  ConfigManager::setVolume(newVol);

  if (!radioWindow.minimized && radioWindow.visible) {
    draw3DFrame(lcd, radioWindow.x + 310, radioWindow.y + 35, 90, 25, true);
    lcd.setTextColor(MAC_BLACK, MAC_WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(radioWindow.x + 315, radioWindow.y + 43);
    lcd.printf("Volume: %d", newVol);
  }
}

void onPrev() {
  int stationCount = ConfigManager::getStationCount();

  if (stationCount == 0) {
    return;
  }

  extern int currentStationIndex;
  int prevIndex;
  if (currentStationIndex <= 0) {
    prevIndex = stationCount - 1;
  } else {
    prevIndex = currentStationIndex - 1;
  }

  switchToStation(prevIndex);
}

void onNext() {
  int stationCount = ConfigManager::getStationCount();

  if (stationCount == 0) {
    return;
  }

  extern int currentStationIndex;
  int nextIndex;
  if (currentStationIndex < 0 || currentStationIndex >= stationCount - 1) {
    nextIndex = 0;
  } else {
    nextIndex = currentStationIndex + 1;
  }

  switchToStation(nextIndex);
}

void onVolDown() {
  extern Audio audio;

  int newVol = max(0, audio.getVolume() - 1);
  audio.setVolume(newVol);
  ConfigManager::setVolume(newVol);

  if (!radioWindow.minimized && radioWindow.visible) {
    draw3DFrame(lcd, radioWindow.x + 310, radioWindow.y + 35, 90, 25, true);
    lcd.setTextColor(MAC_BLACK, MAC_WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(radioWindow.x + 315, radioWindow.y + 43);
    lcd.printf("Volume: %d", newVol);
  }
}
