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

  MacComponent* txtRadioName = findComponentById(radioWindow, 200);
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

void initializeRadioWindow() {
  extern String currentStationName;
  extern const int BTN_STATION;

  clearChildComponents(radioWindow);

  MacComponent* btnPrev = createButtonComponent(30, 165, 50, 50, 4, "", SYMBOL_PREV);
  btnPrev->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(radioWindow, btnPrev);

  MacComponent* btnPlay = createButtonComponent(80, 160, 60, 60, 1, "", SYMBOL_PLAY);
  btnPlay->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(radioWindow, btnPlay);

  MacComponent* btnStation = createButtonComponent(350, 165, 50, 50, BTN_STATION, "", SYMBOL_LIST);
  btnStation->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(radioWindow, btnStation);

  MacComponent* btnNext = createButtonComponent(140, 165, 50, 50, 5, "", SYMBOL_NEXT);
  btnNext->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(radioWindow, btnNext);

  MacComponent* txtRadioName = createRunningTextComponent(20, 45, 380, 25, 200,
                                                          currentStationName, 2, MAC_BLACK, 3);
  txtRadioName->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(radioWindow, txtRadioName);

  MacComponent* txtRadioDetails = createRunningTextComponent(20, 75, 200, 20, 201,
                                                             "Standby waiting for metadata ...", 2, MAC_BLACK, 1);
  txtRadioDetails->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(radioWindow, txtRadioDetails);

  MacComponent* txtBitRate = createRunningTextComponent(20, 92, 200, 20, 203,
                                                        "Bitrate: N/A", 2, MAC_BLACK, 1);
  addChildComponent(radioWindow, txtBitRate);

  MacComponent* txtID3 = createRunningTextComponent(20, 109, 200, 20, 204,
                                                    "ID3: N/A", 2, MAC_BLACK, 1);
  addChildComponent(radioWindow, txtID3);

  MacComponent* txtInfo = createRunningTextComponent(20, 126, 200, 20, 205,
                                                     "", 2, MAC_BLACK, 1);
  addChildComponent(radioWindow, txtInfo);

  MacComponent* txtDescription = createRunningTextComponent(20, 143, 200, 20, 206,
                                                            "", 2, MAC_BLACK, 1);
  addChildComponent(radioWindow, txtDescription);

  #if ENABLE_DEBUG
  MacComponent* cpuLabel = createLabelComponent(0, 6, 200, 15, 202, "CPU0: 0% CPU1: 0%", MAC_BLACK);
  cpuLabel->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(radioWindow, cpuLabel);
  #endif

  MacComponent* btnVolUp = createButtonComponent(200, 165, 50, 50, 3, "", SYMBOL_VOL_UP);
  btnVolUp->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(radioWindow, btnVolUp);

  MacComponent* btnVolDn = createButtonComponent(250, 165, 50, 50, 6, "", SYMBOL_VOL_DOWN);
  btnVolDn->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(radioWindow, btnVolDn);
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

void initializeAddStationWindow() {
  extern const int BTN_SAVE_STATION;
  extern const int BTN_CANCEL_ADD_STATION;

  clearChildComponents(addStationWindow);

  MacComponent* lblStationName = createLabelComponent(20, 45, 120, 20, 400, "Station Name:");
  addChildComponent(addStationWindow, lblStationName);

  MacComponent* txtStationName = createInputFieldComponent(150, 45, 190, 25, 401, "Enter station name", 50);
  txtStationName->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(addStationWindow, txtStationName);

  MacComponent* lblStationURL = createLabelComponent(20, 80, 120, 20, 402, "Station URL:");
  addChildComponent(addStationWindow, lblStationURL);

  MacComponent* txtStationURL = createInputFieldComponent(150, 80, 190, 25, 403, "https://...", 200);
  txtStationURL->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(addStationWindow, txtStationURL);

  MacComponent* btnSave = createButtonComponent(80, 120, 80, 30, BTN_SAVE_STATION, "Save");
  btnSave->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(addStationWindow, btnSave);

  MacComponent* btnCancel = createButtonComponent(180, 120, 80, 30, BTN_CANCEL_ADD_STATION, "Cancel");
  btnCancel->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(addStationWindow, btnCancel);
}

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
  btnYes->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(confirmDeleteWindow, btnYes);

  MacComponent* btnNo = createButtonComponent(150, 75, 80, 30, BTN_CONFIRM_NO, "No");
  btnNo->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(confirmDeleteWindow, btnNo);
}
