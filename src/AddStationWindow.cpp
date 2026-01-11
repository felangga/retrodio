/*
 * AddStationWindow.cpp - Add Station Window Implementation
 *
 * Copyright (c) 2025 felangga
 *
 * This file implements add station window initialization functions
 */

#include "AddStationWindow.h"
#include "GlobalState.h"
#include "WindowCallbacks.h"

void initializeAddStationWindow() {
  extern const int BTN_SAVE_STATION;
  extern const int BTN_CANCEL_ADD_STATION;
  extern const int LBL_STATION_NAME;
  extern const int INPUT_STATION_NAME;
  extern const int LBL_STATION_URL;
  extern const int INPUT_STATION_URL;

  clearChildComponents(addStationWindow);

  MacComponent* lblStationName = createLabelComponent(20, 50, 120, 20, LBL_STATION_NAME, "Station Name:");
  MacLabel* labelData = (MacLabel*)lblStationName->customData;
  labelData->font = FONT_CHICAGO_9PT;

  addChildComponent(addStationWindow, lblStationName);

  MacComponent* txtStationName = createInputFieldComponent(140, 45, 200, 25, INPUT_STATION_NAME, "Enter station name", 50);
  addChildComponent(addStationWindow, txtStationName);

  MacComponent* lblStationURL = createLabelComponent(20, 90, 120, 20, LBL_STATION_URL, "Station URL:");
  labelData = (MacLabel*)lblStationURL->customData;
  labelData->font = FONT_CHICAGO_9PT;
  addChildComponent(addStationWindow, lblStationURL);

  MacComponent* txtStationURL = createInputFieldComponent(140, 80, 200, 25, INPUT_STATION_URL, "https://...", 200, "https://");
  addChildComponent(addStationWindow, txtStationURL);

  MacComponent* btnSave = createButtonComponent(170, 115, 80, 30, BTN_SAVE_STATION, "Save");
  btnSave->onClick = [](int componentId) { onSaveStationButtonClick(); };
  addChildComponent(addStationWindow, btnSave);

  MacComponent* btnCancel = createButtonComponent(260, 115, 80, 30, BTN_CANCEL_ADD_STATION, "Cancel");
  btnCancel->onClick = [](int componentId) { onCancelAddStationButtonClick(); };
  addChildComponent(addStationWindow, btnCancel);
}
