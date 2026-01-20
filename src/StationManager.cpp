/*
 * StationManager.cpp - Station Management Implementation
 *
 * Copyright (c) 2025 felangga
 *
 */

#include "StationManager.h"
#include <WiFi.h>
#include "ConfigManager.h"
#include "ConfirmDeleteWindow.h"
#include "GlobalState.h"
#include "NetworkHandlers.h"
#include "UIHelpers.h"
#include "WindowCallbacks.h"

void initializeConfirmDeleteWindow();

UIListViewItem* stationItems = nullptr;
int stationItemCount = 0;
SemaphoreHandle_t stationListMutex = nullptr;

void reloadStationList() {
  // Create mutex if it doesn't exist
  if (stationListMutex == nullptr) {
    stationListMutex = xSemaphoreCreateMutex();
  }

  // Take mutex before modifying station list
  if (xSemaphoreTake(stationListMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    return;  // Couldn't get mutex, skip reload
  }

  UIListViewItem* oldItems = stationItems;
  stationItemCount = ConfigManager::getStationCount();

  if (stationItemCount == 0) {
    stationItems = nullptr;
    xSemaphoreGive(stationListMutex);

    if (oldItems != nullptr) {
      vTaskDelay(pdMS_TO_TICKS(50));  // Wait for any ongoing rendering
      delete[] oldItems;
    }
    return;
  }

  UIListViewItem* newItems = new UIListViewItem[stationItemCount];

  for (int i = 0; i < stationItemCount; i++) {
    Station station = ConfigManager::getStation(i);
    newItems[i].text = station.name;
    newItems[i].data = nullptr;
  }

  stationItems = newItems;

  // Release mutex before the delay
  xSemaphoreGive(stationListMutex);

  // Wait for any ongoing rendering to finish before deleting old items
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
      streamMetadata.bitRate[0] = '\0';
      streamMetadata.id3data[0] = '\0';
      streamMetadata.info[0] = '\0';
      streamMetadata.description[0] = '\0';
      streamMetadata.lyrics[0] = '\0';
      streamMetadata.log[0] = '\0';
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
  extern const int TXT_RADIO_DETAILS;
  extern const int TXT_BITRATE;
  extern const int TXT_INFO;
  extern const int TXT_DESCRIPTION;

  UIComponent* txtRadioName = findComponentById(radioWindow, TXT_RADIO_NAME);
  if (txtRadioName && txtRadioName->customData) {
    UIRunningText* runningText = (UIRunningText*)txtRadioName->customData;
    if (runningText) {
      runningText->text = currentStationName;
      runningText->scrollOffset = 0;
    }
  }

  UIComponent* txtRadioDetails = findComponentById(radioWindow, TXT_RADIO_DETAILS);
  if (txtRadioDetails && txtRadioDetails->customData) {
    UIRunningText* runningText = (UIRunningText*)txtRadioDetails->customData;
    runningText->text = "";
    runningText->scrollOffset = 0;
  }

  UIComponent* txtBitRate = findComponentById(radioWindow, TXT_BITRATE);
  if (txtBitRate && txtBitRate->customData) {
    UIRunningText* runningText = (UIRunningText*)txtBitRate->customData;
    runningText->text = "";
    runningText->scrollOffset = 0;
  }

  UIComponent* txtInfo = findComponentById(radioWindow, TXT_INFO);
  if (txtInfo && txtInfo->customData) {
    UIRunningText* runningText = (UIRunningText*)txtInfo->customData;
    runningText->text = "";
    runningText->scrollOffset = 0;
  }

  UIComponent* txtDescription = findComponentById(radioWindow, TXT_DESCRIPTION);
  if (txtDescription && txtDescription->customData) {
    UIRunningText* runningText = (UIRunningText*)txtDescription->customData;
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

  UIComponent* playButton = findComponentById(radioWindow, 1);
  if (playButton && playButton->customData) {
    UIButton* btnData = (UIButton*)playButton->customData;
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
  extern const int BTN_EDIT_STATION;
  extern const int BTN_DELETE_STATION;

  clearChildComponents(stationWindow);

  UIComponent* btnAddStation = createButtonComponent(310, 42, 90, 30, BTN_ADD_STATION, "Add");
  btnAddStation->onClick = [](int componentId) { onAddStationButtonClick(); };
  addChildComponent(stationWindow, btnAddStation);

  UIComponent* btnEditStation = createButtonComponent(310, 77, 90, 30, BTN_EDIT_STATION, "Edit");
  btnEditStation->onClick = [](int componentId) { onEditStationButtonClick(); };
  addChildComponent(stationWindow, btnEditStation);

  UIComponent* btnDeleteStation =
      createButtonComponent(310, 112, 90, 30, BTN_DELETE_STATION, "Delete");
  btnDeleteStation->onClick = [](int componentId) { onDeleteStationButtonClick(); };
  addChildComponent(stationWindow, btnDeleteStation);

  // Take mutex before accessing station list
  UIComponent* stationList = nullptr;
  if (stationListMutex && xSemaphoreTake(stationListMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    stationList =
        createListViewComponent(10, 42, 290, 188, 300, stationItems, stationItemCount, 30);
    xSemaphoreGive(stationListMutex);
  } else {
    // If we can't get the mutex, create an empty list
    stationList = createListViewComponent(10, 42, 290, 188, 300, nullptr, 0, 30);
  }

  if (stationList && stationList->customData) {
    UIListView* listViewData = (UIListView*)stationList->customData;
    listViewData->onItemClick = onStationItemClick;
    listViewData->font = FONT_CHICAGO_9PT;  // Set Chicago font
  }

  addChildComponent(stationWindow, stationList);
}

void onAddStationButtonClick() {
  extern bool isEditMode;
  extern int stationToEditIndex;
  extern const int INPUT_STATION_NAME;
  extern const int INPUT_STATION_URL;

  isEditMode = false;
  stationToEditIndex = -1;

  UIComponent* nameInputComp = findComponentById(addStationWindow, INPUT_STATION_NAME);
  UIComponent* urlInputComp = findComponentById(addStationWindow, INPUT_STATION_URL);

  if (nameInputComp && urlInputComp) {
    UIInputField* nameInput = (UIInputField*)nameInputComp->customData;
    UIInputField* urlInput = (UIInputField*)urlInputComp->customData;

    nameInput->text = "";
    nameInput->cursorPos = 0;
    urlInput->text = "https://";
    urlInput->cursorPos = urlInput->text.length();
  }

  stationWindow.visible = false;
  stationWindow.active = false;
  addStationWindow.visible = true;
  addStationWindow.active = true;
  addStationWindow.minimized = false;

  drawCheckeredPatternArea(lcd, stationWindow.x, stationWindow.y, stationWindow.w + 5,
                           stationWindow.h + 5);
  drawWindow(lcd, addStationWindow);
}

void onEditStationButtonClick() {
  extern bool isEditMode;
  extern int stationToEditIndex;
  extern const int INPUT_STATION_NAME;
  extern const int INPUT_STATION_URL;

  UIComponent* stationListComp = findComponentById(stationWindow, 300);

  if (stationListComp && stationListComp->customData) {
    UIListView* listViewData = (UIListView*)stationListComp->customData;

    if (listViewData->selectedIndex >= 0 &&
        listViewData->selectedIndex < ConfigManager::getStationCount()) {
      isEditMode = true;
      stationToEditIndex = listViewData->selectedIndex;

      Station station = ConfigManager::getStation(stationToEditIndex);

      UIComponent* nameInputComp = findComponentById(addStationWindow, INPUT_STATION_NAME);
      UIComponent* urlInputComp = findComponentById(addStationWindow, INPUT_STATION_URL);

      if (nameInputComp && urlInputComp) {
        UIInputField* nameInput = (UIInputField*)nameInputComp->customData;
        UIInputField* urlInput = (UIInputField*)urlInputComp->customData;

        nameInput->text = station.name;
        nameInput->cursorPos = station.name.length();
        urlInput->text = station.url;
        urlInput->cursorPos = station.url.length();
      }

      stationWindow.visible = false;
      stationWindow.active = false;
      addStationWindow.visible = true;
      addStationWindow.active = true;
      addStationWindow.minimized = false;

      drawCheckeredPatternArea(lcd, stationWindow.x, stationWindow.y, stationWindow.w + 5,
                               stationWindow.h + 5);
      drawWindow(lcd, addStationWindow);
    }
  }
}

void onSaveStationButtonClick() {
  extern const int INPUT_STATION_NAME;
  extern const int INPUT_STATION_URL;
  extern bool isEditMode;
  extern int stationToEditIndex;

  UIComponent* nameInputComp = findComponentById(addStationWindow, INPUT_STATION_NAME);
  UIComponent* urlInputComp = findComponentById(addStationWindow, INPUT_STATION_URL);

  if (nameInputComp && urlInputComp) {
    UIInputField* nameInput = (UIInputField*)nameInputComp->customData;
    UIInputField* urlInput = (UIInputField*)urlInputComp->customData;

    String stationName = nameInput->text;
    String stationURL = urlInput->text;

    if (stationName.length() > 0 && stationURL.length() > 0) {
      bool success = false;

      if (isEditMode && stationToEditIndex >= 0) {
        success = ConfigManager::updateStation(stationToEditIndex, stationName, stationURL);
      } else {
        success = ConfigManager::addStation(stationName, stationURL);
      }

      if (success) {
        reloadStationList();
        initializeStationWindow();

        nameInput->text = "";
        nameInput->cursorPos = 0;
        urlInput->text = "";
        urlInput->cursorPos = 0;

        isEditMode = false;
        stationToEditIndex = -1;
      } else {
        return;
      }
    }
  }

  if (globalKeyboard) {
    UIKeyboard* keyboard = (UIKeyboard*)globalKeyboard->customData;
    keyboard->visible = false;
  }

  adjustWindowForKeyboard(addStationWindow, nullptr, false);

  addStationWindow.visible = false;
  addStationWindow.active = false;
  stationWindow.visible = true;
  stationWindow.active = true;

  drawCheckeredPatternArea(lcd, addStationWindow.x, addStationWindow.y, addStationWindow.w + 5,
                           addStationWindow.h + 5);
  drawWindow(lcd, stationWindow);
}

void onCancelAddStationButtonClick() {
  if (globalKeyboard) {
    UIKeyboard* keyboard = (UIKeyboard*)globalKeyboard->customData;
    keyboard->visible = false;
  }

  adjustWindowForKeyboard(addStationWindow, nullptr, false);

  addStationWindow.visible = false;
  addStationWindow.active = false;
  stationWindow.visible = true;
  stationWindow.active = true;

  drawCheckeredPatternArea(lcd, addStationWindow.x, addStationWindow.y, addStationWindow.w + 5,
                           addStationWindow.h + 5);
  drawWindow(lcd, stationWindow);
}

void onDeleteStationButtonClick() {
  UIComponent* stationListComp = findComponentById(stationWindow, 300);

  if (stationListComp && stationListComp->customData) {
    UIListView* listViewData = (UIListView*)stationListComp->customData;

    if (listViewData->selectedIndex >= 0 &&
        listViewData->selectedIndex < ConfigManager::getStationCount()) {
      stationToDeleteIndex = listViewData->selectedIndex;

      stationWindow.visible = false;
      stationWindow.active = false;

      initializeConfirmDeleteWindow();
      confirmDeleteWindow.visible = true;
      confirmDeleteWindow.active = true;

      drawCheckeredPatternArea(lcd, stationWindow.x, stationWindow.y, stationWindow.w + 5,
                               stationWindow.h + 5);
      drawWindow(lcd, confirmDeleteWindow);
    }
  }
}
