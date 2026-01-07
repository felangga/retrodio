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
  radioWindow.visible = true;
  drawWindow(lcd, radioWindow);
}

void onStationWindowClose() {
  stationWindow.visible = false;
  radioWindow.visible = true;
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

  addStationWindow.visible = false;
  stationWindow.visible = true;
  drawWindow(lcd, stationWindow);
}

void onAddStationWindowClose() {
  if (globalKeyboard) {
    MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
    keyboard->visible = false;
  }

  addStationWindow.visible = false;
  stationWindow.visible = true;
  drawWindow(lcd, stationWindow);
}

void onAddStationWindowContentClick(int relativeX, int relativeY) {
  MacComponent* nameInputComp = nullptr;
  MacComponent* urlInputComp = nullptr;

  for (int i = 0; i < addStationWindow.childComponentCount; i++) {
    MacComponent* comp = addStationWindow.childComponents[i];
    if (comp->id == 401) {
      nameInputComp = comp;
    } else if (comp->id == 403) {
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
      keyboard->targetInputId = 401;
      keyboard->visible = true;

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
      keyboard->targetInputId = 403;
      keyboard->visible = true;

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

  if (componentId == CMP_NOW_PLAYING_LABEL) {
    displayStatus(lcd, "Label clicked", 160);
  } else if (componentId == CMP_VOLUME_SLIDER) {
    displayStatus(lcd, "Volume slider clicked", 160);
  } else if (componentId == CMP_BUFFER_PROGRESS) {
    displayStatus(lcd, "Progress bar clicked", 160);
  } else if (componentId == CMP_AUTO_PLAY_CHECKBOX) {
    MacComponent* component = findComponentById(radioWindow, componentId);
    if (component && component->customData) {
      MacCheckBox* checkbox = (MacCheckBox*)component->customData;
      checkbox->checked = !checkbox->checked;
      drawComponent(lcd, *component, radioWindow.x, radioWindow.y);
      displayStatus(lcd, checkbox->checked ? "Auto Play ON" : "Auto Play OFF", 160);
    }
  } else if (componentId == CMP_VISUALS_CHECKBOX) {
    MacComponent* component = findComponentById(radioWindow, componentId);
    if (component && component->customData) {
      MacCheckBox* checkbox = (MacCheckBox*)component->customData;
      checkbox->checked = !checkbox->checked;
      drawComponent(lcd, *component, radioWindow.x, radioWindow.y);
      displayStatus(lcd, checkbox->checked ? "Visuals ON" : "Visuals OFF", 160);
    }
  } else if (componentId == BTN_PLAY) {
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
    showWindowOnTop(lcd, stationWindow);
  } else if (componentId == BTN_ADD_STATION) {
    stationWindow.visible = false;

    {
      int tx, ty;
      delay(200);
      while (lcd.getTouch(&tx, &ty)) {
        delay(10);
      }
      delay(50);
    }

    addStationWindow.visible = true;
    addStationWindow.minimized = false;

    drawWindow(lcd, addStationWindow);
  } else if (componentId == BTN_SAVE_STATION) {
    {
      MacComponent* nameInputComp = findComponentById(addStationWindow, 401);
      MacComponent* urlInputComp = findComponentById(addStationWindow, 403);

      if (nameInputComp && urlInputComp) {
        MacInputField* nameInput = (MacInputField*)nameInputComp->customData;
        MacInputField* urlInput = (MacInputField*)urlInputComp->customData;

        String stationName = nameInput->text;
        String stationURL = urlInput->text;

        if (stationName.length() > 0 && stationURL.length() > 0) {
          if (ConfigManager::addStation(stationName, stationURL)) {
            displayStatus(lcd, "Station Saved: " + stationName, 160);

            reloadStationList();
            initializeStationWindow();

            nameInput->text = "";
            nameInput->cursorPos = 0;
            urlInput->text = "";
            urlInput->cursorPos = 0;
          } else {
            displayStatus(lcd, "Failed to save station", 160);
            return;
          }
        } else {
          displayStatus(lcd, "Please fill all fields", 160);
          return;
        }
      }
    }

    if (globalKeyboard) {
      MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
      keyboard->visible = false;
    }

    addStationWindow.visible = false;
    stationWindow.visible = true;

    drawWindow(lcd, stationWindow);
  } else if (componentId == BTN_CANCEL_ADD_STATION) {
    if (globalKeyboard) {
      MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
      keyboard->visible = false;
    }

    addStationWindow.visible = false;
    stationWindow.visible = true;

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
    displayStatus(lcd, "Paused", 160);
  } else {
    displayStatus(lcd, "Connecting...", 160);
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
  displayStatus(lcd, "Stopped", 160);
}

void onVolUp() {
  extern Audio audio;

  int newVol = min(21, audio.getVolume() + 1);
  audio.setVolume(newVol);
  ConfigManager::setVolume(newVol);
  displayStatus(lcd, "Volume: " + String(newVol), 160);

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
    displayStatus(lcd, "No stations available", 160);
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
    displayStatus(lcd, "No stations available", 160);
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
  displayStatus(lcd, "Volume: " + String(newVol), 160);

  if (!radioWindow.minimized && radioWindow.visible) {
    draw3DFrame(lcd, radioWindow.x + 310, radioWindow.y + 35, 90, 25, true);
    lcd.setTextColor(MAC_BLACK, MAC_WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(radioWindow.x + 315, radioWindow.y + 43);
    lcd.printf("Volume: %d", newVol);
  }
}
