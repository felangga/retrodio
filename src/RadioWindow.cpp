/*
 * RadioWindow.cpp - Radio Window Implementation
 *
 * Copyright (c) 2025 felangga
 *
 */

#include "RadioWindow.h"
#include "GlobalState.h"
#include "UIHelpers.h"
#include "WindowCallbacks.h"
#include "AudioHandlers.h"
#include "ConfigManager.h"
#include "StationManager.h"
#include "wt32_sc01_plus.h"

MacComponent* findComponentById(const MacWindow& window, int id);
void updateComponentSymbol(const MacWindow& window, int componentId, SymbolType newSymbol);

// Forward declarations for button handlers
void onPlay();
void onStop();
void onVolUp();
void onVolDown();
void onPrev();
void onNext();

void initializeRadioWindow() {
  extern String currentStationName;
  extern const int BTN_STATION;

  clearChildComponents(radioWindow);

  MacComponent* btnPrev = createButtonComponent(30, 165, 50, 50, 4, "", SYMBOL_PREV);
  btnPrev->onClick = [](int componentId) { onPrev(); };
  addChildComponent(radioWindow, btnPrev);

  MacComponent* btnPlay = createButtonComponent(80, 160, 60, 60, 1, "", SYMBOL_PLAY);
  btnPlay->onClick = [](int componentId) { onPlay(); };
  addChildComponent(radioWindow, btnPlay);

  MacComponent* btnStation = createButtonComponent(350, 165, 50, 50, BTN_STATION, "", SYMBOL_LIST);
  btnStation->onClick = [](int componentId) { onStationButtonClick(); };
  addChildComponent(radioWindow, btnStation);

  MacComponent* btnNext = createButtonComponent(140, 165, 50, 50, 5, "", SYMBOL_NEXT);
  btnNext->onClick = [](int componentId) { onNext(); };
  addChildComponent(radioWindow, btnNext);

  extern const int TXT_RADIO_NAME;
  extern const int TXT_RADIO_DETAILS;
  extern const int TXT_BITRATE;
  extern const int TXT_ID3;
  extern const int TXT_INFO;
  extern const int TXT_DESCRIPTION;
  extern const int TXT_CPU_LABEL;

  MacComponent* txtRadioName = createRunningTextComponent(20, 45, 380, 45, TXT_RADIO_NAME,
                                                          currentStationName, 2, MAC_BLACK, 3);
  addChildComponent(radioWindow, txtRadioName);

  MacComponent* txtRadioDetails = createRunningTextComponent(20, 85, 250, 20, TXT_RADIO_DETAILS,
                                                             "Standby waiting for metadata ...", 2, MAC_BLACK, 1);
  addChildComponent(radioWindow, txtRadioDetails);

  MacComponent* txtBitRate = createRunningTextComponent(20, 100, 100, 20, TXT_BITRATE,
                                                        "Bitrate: N/A", 2, MAC_BLACK, 1);
  addChildComponent(radioWindow, txtBitRate);

  MacComponent* txtInfo = createRunningTextComponent(20, 115, 250, 20, TXT_INFO,
                                                     "", 2, MAC_BLACK, 1);
  addChildComponent(radioWindow, txtInfo);

  MacComponent* txtDescription = createRunningTextComponent(20, 130, 250, 20, TXT_DESCRIPTION,
                                                            "", 2, MAC_BLACK, 1);
  addChildComponent(radioWindow, txtDescription);

  #if ENABLE_DEBUG
  MacComponent* cpuLabel = createLabelComponent(0, 6, 200, 15, TXT_CPU_LABEL, "CPU0: 0% CPU1: 0%", MAC_BLACK);
  addChildComponent(radioWindow, cpuLabel);
  #endif

  MacComponent* btnVolUp = createButtonComponent(200, 165, 50, 50, 3, "", SYMBOL_VOL_UP);
  btnVolUp->onClick = [](int componentId) { onVolUp(); };
  addChildComponent(radioWindow, btnVolUp);

  MacComponent* btnVolDn = createButtonComponent(250, 165, 50, 50, 6, "", SYMBOL_VOL_DOWN);
  btnVolDn->onClick = [](int componentId) { onVolDown(); };
  addChildComponent(radioWindow, btnVolDn);
}

// Radio button handler implementations
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
    draw3DFrame(lcd, radioWindow.x + 310, radioWindow.y + 35, 90, 25);
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
    draw3DFrame(lcd, radioWindow.x + 310, radioWindow.y + 35, 90, 25);
    lcd.setTextColor(MAC_BLACK, MAC_WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(radioWindow.x + 315, radioWindow.y + 43);
    lcd.printf("Volume: %d", newVol);
  }
}
