/*
 * RadioWindow.cpp - Radio Window Implementation
 *
 * Copyright (c) 2025 felangga
 *
 */

#include "RadioWindow.h"
#include "AudioHandlers.h"
#include "ConfigManager.h"
#include "GlobalState.h"
#include "NetworkHandlers.h"
#include "StationManager.h"
#include "UIHelpers.h"
#include "WindowCallbacks.h"
#include "wt32_sc01_plus.h"

UIComponent* findComponentById(const UIWindow& window, int id);
void updateComponentSymbol(const UIWindow& window, int componentId, SymbolType newSymbol);

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
    if (!isWiFiConnected()) {
      showNotification("No WiFi connection!", 2000);
      return;
    }
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

void onVolumeChange(int componentId, int value) {
  extern Audio audio;
  extern unsigned long volumeChangeTime;
  extern bool volumeDisplayActive;
  extern String savedStationName;
  extern String currentStationName;
  extern const int TXT_RADIO_NAME;

  audio.setVolume(value);
  ConfigManager::setVolume(value);

  // Save current station name if not already displaying volume
  if (!volumeDisplayActive) {
    savedStationName = currentStationName;
  }

  // Calculate volume percentage (0-21 slider range to 0-100%)
  int volumePercent = (value * 100) / 21;

  // Update the radio name text to show volume percentage
  UIComponent* txtRadioName = findComponentById(radioWindow, TXT_RADIO_NAME);
  if (txtRadioName && txtRadioName->customData) {
    UIRunningText* runningText = (UIRunningText*)txtRadioName->customData;
    runningText->text = "Volume: " + String(volumePercent) + "%";
    runningText->scrollOffset = 0;
  }

  // Mark that we're displaying volume and record the time
  volumeDisplayActive = true;
  volumeChangeTime = millis();
}

void initializeRadioWindow() {
  extern String currentStationName;
  extern const int BTN_STATION;

  clearChildComponents(radioWindow);

  UIComponent* btnPrev = createButtonComponent(28, 165, 50, 50, 4, "", SYMBOL_PREV);
  btnPrev->onClick = [](int componentId) { onPrev(); };
  addChildComponent(radioWindow, btnPrev);

  UIComponent* btnPlay = createButtonComponent(80, 160, 60, 60, 1, "", SYMBOL_PLAY);
  btnPlay->onClick = [](int componentId) { onPlay(); };
  addChildComponent(radioWindow, btnPlay);

  UIComponent* btnStation = createButtonComponent(350, 165, 50, 50, BTN_STATION, "", SYMBOL_LIST);
  btnStation->onClick = [](int componentId) { onStationButtonClick(); };
  addChildComponent(radioWindow, btnStation);

  UIComponent* btnNext = createButtonComponent(142, 165, 50, 50, 5, "", SYMBOL_NEXT);
  btnNext->onClick = [](int componentId) { onNext(); };
  addChildComponent(radioWindow, btnNext);

  extern const int TXT_RADIO_NAME;
  extern const int TXT_RADIO_DETAILS;
  extern const int TXT_BITRATE;
  extern const int TXT_INFO;
  extern const int TXT_DESCRIPTION;

  UIComponent* txtRadioName = createRunningTextComponent(20, 45, 380, 45, TXT_RADIO_NAME,
                                                         currentStationName, 2, UI_BLACK, 3);
  addChildComponent(radioWindow, txtRadioName);

  UIComponent* txtRadioDetails = createRunningTextComponent(
      20, 85, 250, 20, TXT_RADIO_DETAILS, "Standby waiting for metadata ...", 2, UI_BLACK, 1);
  addChildComponent(radioWindow, txtRadioDetails);

  UIComponent* txtBitRate =
      createRunningTextComponent(20, 100, 100, 20, TXT_BITRATE, "", 2, UI_BLACK, 1);
  addChildComponent(radioWindow, txtBitRate);

  UIComponent* txtInfo = createRunningTextComponent(20, 115, 250, 20, TXT_INFO, "", 2, UI_BLACK, 1);
  addChildComponent(radioWindow, txtInfo);

  UIComponent* txtDescription =
      createRunningTextComponent(20, 130, 250, 20, TXT_DESCRIPTION, "", 2, UI_BLACK, 1);
  addChildComponent(radioWindow, txtDescription);

  extern Audio audio;
  extern const int CMP_VOLUME_SLIDER;
  int currentVolume = ConfigManager::getVolume();
  UIComponent* volumeSlider =
      createSliderComponent(200, 155, 120, 65, CMP_VOLUME_SLIDER, 0, 21, currentVolume, false);
  volumeSlider->onValueChanged = [](int componentId, int value) {
    onVolumeChange(componentId, value);
  };
  addChildComponent(radioWindow, volumeSlider);
}
