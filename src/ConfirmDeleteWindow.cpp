/*
 * ConfirmDeleteWindow.cpp - Confirm Delete Window Implementation
 *
 * Copyright (c) 2025 felangga
 *
 */

#include "GlobalState.h"
#include "WindowCallbacks.h"
#include "ConfigManager.h"
#include "StationManager.h"

void initializeConfirmDeleteWindow() {
  extern const int BTN_CONFIRM_YES;
  extern const int BTN_CONFIRM_NO;

  clearChildComponents(confirmDeleteWindow);

  MacComponent* lblMessage = createLabelComponent(20, 45, 240, 20, 500, "Delete this station?");
  if (lblMessage && lblMessage->customData) {
    MacLabel* labelData = (MacLabel*)lblMessage->customData;
    labelData->font = FONT_CHICAGO_9PT;
  }
  addChildComponent(confirmDeleteWindow, lblMessage);

  MacComponent* btnYes = createButtonComponent(50, 75, 80, 30, BTN_CONFIRM_YES, "Yes");
  btnYes->onClick = [](int componentId) { onConfirmYesButtonClick(); };
  addChildComponent(confirmDeleteWindow, btnYes);

  MacComponent* btnNo = createButtonComponent(150, 75, 80, 30, BTN_CONFIRM_NO, "No");
  btnNo->onClick = [](int componentId) { onConfirmNoButtonClick(); };
  addChildComponent(confirmDeleteWindow, btnNo);
}

void onConfirmYesButtonClick() {
  // User confirmed deletion
  if (stationToDeleteIndex >= 0 && stationToDeleteIndex < ConfigManager::getStationCount()) {
    // Delete the station
    if (ConfigManager::removeStation(stationToDeleteIndex)) {
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
}

void onConfirmNoButtonClick() {
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

