/*
 * StationManager.cpp - Station Management Implementation
 *
 * Copyright (c) 2025 felangga
 *
 * This file implements station management functions
 */

#include "StationManager.h"
#include "GlobalState.h"
#include "UIHelpers.h"
#include "ConfigManager.h"
#include <WiFi.h>

void onComponentClick(int id, void* data);

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
  btnAddStation->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(stationWindow, btnAddStation);

  MacComponent* btnDeleteStation = createButtonComponent(310, 77, 90, 30, BTN_DELETE_STATION, "Delete");
  btnDeleteStation->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(stationWindow, btnDeleteStation);

  MacComponent* stationList = createListViewComponent(10, 42, 290, 188, 300,
                                                      stationItems, stationItemCount, 30);

  if (stationList && stationList->customData) {
    MacListView* listViewData = (MacListView*)stationList->customData;
    listViewData->onItemClick = onStationItemClick;
    listViewData->font = FONT_CHICAGO_9PT;  // Set Chicago font
  }

  stationList->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(stationWindow, stationList);
}
