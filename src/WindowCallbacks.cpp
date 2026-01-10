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
#include "ConfirmDeleteWindow.h"
#include "AudioHandlers.h"
#include "RadioWindow.h"
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

// Individual button click handlers
void onStationButtonClick() {
  radioWindow.visible = false;
  radioWindow.active = false;
  stationWindow.visible = true;
  stationWindow.active = true;
  stationWindow.minimized = false;
  drawCheckeredPatternArea(lcd, radioWindow.x, radioWindow.y, radioWindow.w + 5, radioWindow.h + 5);
  drawWindow(lcd, stationWindow);
}

