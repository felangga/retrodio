/*
 * StationManager.cpp - Station Management Implementation
 *
 * Copyright (c) 2025 felangga
 *
 * This file implements station management functions
 */

#include "ConfirmDeleteWindow.h"
#include "StationManager.h"
#include "GlobalState.h"
#include "UIHelpers.h"
#include "ConfigManager.h"
#include "WindowCallbacks.h"
#include <WiFi.h>

void initializeConfirmDeleteWindow();

MacListViewItem* stationItems = nullptr;
int stationItemCount = 0;

void reloadStationList() {
  MacListViewItem* oldItems = stationItems;
  int oldCount = stationItemCount;

  stationItemCount = ConfigManager::getStationCount();

  if (stationItemCount == 0) {
    stationItems = nullptr;
    if (oldItems != nullptr) {
      delete[] oldItems;
    }
    return;
  }

  MacListViewItem* newItems = new MacListViewItem[stationItemCount];

  for (int i = 0; i < stationItemCount; i++) {
    Station station = ConfigManager::getStation(i);
    newItems[i].text = station.name;
    newItems[i].data = nullptr;
  }

  stationItems = newItems;

  vTaskDelay(pdMS_TO_TICKS(50));

  if (oldItems != nullptr) {
    delete[] oldItems;
  }
}

void switchToStation(int index) {
  extern String currentStationName;
  extern int currentStationIndex;
  extern SemaphoreHandle_t metadataMutex;
  extern StreamMetadata streamMetadata;
  extern String lastTrackInfo;
  extern String lastDisplayedBitRate;
  extern String lastDisplayedID3;
  extern String lastDisplayedInfo;
  extern String lastDisplayedDescription;
  extern String lastDisplayedLyrics;
  extern String lastDisplayedLog;
  extern QueueHandle_t audioCommandQueue;
  extern Audio audio;
  extern volatile bool isPlaying;

  if (index < 0 || index >= ConfigManager::getStationCount()) {
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    updateStationMetadata("Error", "WiFi not connected");
    return;
  }

  Station station = ConfigManager::getStation(index);

  if (station.url.length() == 0) {
    return;
  }

  currentStationName = station.name;
  currentStationIndex = index;

  if (metadataMutex) {
    if (xSemaphoreTake(metadataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      streamMetadata.stationName[0] = '\0';
      streamMetadata.trackInfo[0] = '\0';
      streamMetadata.received = false;
      xSemaphoreGive(metadataMutex);
    }
  }

  lastTrackInfo = "";
  lastDisplayedBitRate = "";
  lastDisplayedID3 = "";
  lastDisplayedInfo = "";
  lastDisplayedDescription = "";
  lastDisplayedLyrics = "";
  lastDisplayedLog = "";

  extern const int TXT_RADIO_NAME;
  MacComponent* txtRadioName = findComponentById(radioWindow, TXT_RADIO_NAME);
  if (txtRadioName && txtRadioName->customData) {
    MacRunningText* runningText = (MacRunningText*)txtRadioName->customData;
    if (runningText) {
      runningText->text = currentStationName;
      runningText->scrollOffset = 0;
    }
  }

  MacComponent* txtDescription = findComponentById(radioWindow, TXT_DESCRIPTION);
  if (txtDescription && txtDescription->customData) {
    MacRunningText* runningText = (MacRunningText*)txtDescription->customData;
    runningText->text = "";
    runningText->scrollOffset = 0;
    lastDisplayedDescription = "";
  }

  updateStationMetadata(currentStationName, "Connecting...");

  if (isPlaying && audio.isRunning()) {
    AudioCommandMsg stopMsg = {CMD_STOP, ""};
    xQueueSend(audioCommandQueue, &stopMsg, portMAX_DELAY);
    isPlaying = false;
    vTaskDelay(pdMS_TO_TICKS(200));
  } else {
    isPlaying = false;
  }

  MacComponent* playButton = findComponentById(radioWindow, 1);
  if (playButton && playButton->customData) {
    MacButton* btnData = (MacButton*)playButton->customData;
    if (btnData) {
      AudioCommandMsg msg = {CMD_CONNECT, ""};
      strncpy(msg.url, station.url.c_str(), sizeof(msg.url) - 1);
      msg.url[sizeof(msg.url) - 1] = '\0';

      if (xQueueSend(audioCommandQueue, &msg, pdMS_TO_TICKS(1000)) == pdTRUE) {
        isPlaying = true;
        btnData->symbol = SYMBOL_PAUSE;
        LastStation lastStation = {station.name, station.url};
        ConfigManager::setLastStation(lastStation);
      } else {
        updateStationMetadata(currentStationName, "Error: Command queue full");
      }
    }
  }
}

void onStationItemClick(int index, void* itemData) {
  switchToStation(index);
}

void initializeStationWindow() {
  extern const int BTN_ADD_STATION;
  extern const int BTN_DELETE_STATION;

  clearChildComponents(stationWindow);

  MacComponent* btnAddStation = createButtonComponent(310, 42, 90, 30, BTN_ADD_STATION, "Add");
  btnAddStation->onClick = [](int componentId) { onAddStationButtonClick(); };
  addChildComponent(stationWindow, btnAddStation);

  MacComponent* btnDeleteStation = createButtonComponent(310, 77, 90, 30, BTN_DELETE_STATION, "Delete");
  btnDeleteStation->onClick = [](int componentId) { onDeleteStationButtonClick(); };
  addChildComponent(stationWindow, btnDeleteStation);

  MacComponent* stationList = createListViewComponent(10, 42, 290, 188, 300,
                                                      stationItems, stationItemCount, 30);

  if (stationList && stationList->customData) {
    MacListView* listViewData = (MacListView*)stationList->customData;
    listViewData->onItemClick = onStationItemClick;
    listViewData->font = FONT_CHICAGO_9PT;  // Set Chicago font
  }

  addChildComponent(stationWindow, stationList);
}


void onAddStationButtonClick() {
  stationWindow.visible = false;
  stationWindow.active = false;
  addStationWindow.visible = true;
  addStationWindow.active = true;
  addStationWindow.minimized = false;

  drawCheckeredPatternArea(lcd, stationWindow.x, stationWindow.y, stationWindow.w + 5, stationWindow.h + 5);
  drawWindow(lcd, addStationWindow);
}

void onSaveStationButtonClick() {
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

void onCancelAddStationButtonClick() {
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

void onDeleteStationButtonClick() {
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
}
