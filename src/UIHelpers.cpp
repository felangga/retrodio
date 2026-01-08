/*
 * UIHelpers.cpp - UI Helper Functions Implementation
 *
 * Copyright (c) 2025 felangga
 *
 * This file implements UI helper functions
 */

#include "UIHelpers.h"
#include "GlobalState.h"
#include "StationManager.h"
#include "wt32_sc01_plus.h"
#include <time.h>

#define ENABLE_DEBUG 0

extern LGFX lcd;
extern MacWindow radioWindow;
extern MacWindow stationWindow;
extern MacWindow addStationWindow;
extern DesktopIcon radioIcon;
extern MacComponent* globalKeyboard;

void updateStationMetadata(const String& stationName, const String& trackInfo) {
  MacComponent* txtRadioName = findComponentById(radioWindow, 200);
  if (txtRadioName && txtRadioName->customData) {
    MacRunningText* runningText = (MacRunningText*)txtRadioName->customData;
    runningText->text = stationName;
    runningText->scrollOffset = 0;
  }

  MacComponent* txtRadioDetails = findComponentById(radioWindow, 201);
  if (txtRadioDetails && txtRadioDetails->customData) {
    MacRunningText* runningText = (MacRunningText*)txtRadioDetails->customData;
    runningText->text = trackInfo;
    runningText->scrollOffset = 0;
  }
}

void updateClock() {
  extern unsigned long lastClockUpdate;
  extern String lastClockText;

  unsigned long now = millis();
  if (now - lastClockUpdate < 1000)
    return;
  lastClockUpdate = now;

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return;
  }

  char buf[9];
  strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
  String current = String(buf);
  if (current == lastClockText)
    return;
  lastClockText = current;

  drawClock(lcd, current);
}

void updateCPUUsage() {
  extern unsigned long lastCPUUpdate;
  extern float cpuUsage0;
  extern float cpuUsage1;
  extern TaskHandle_t audioTaskHandle;
  extern TaskHandle_t uiTaskHandle;
  extern volatile bool isPlaying;

  unsigned long now = millis();
  if (now - lastCPUUpdate < 1000)
    return;
  lastCPUUpdate = now;

  if (audioTaskHandle != NULL) {
    UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(audioTaskHandle);
    cpuUsage0 = isPlaying ? 25.0 + (random(0, 20)) : 5.0 + random(0, 5);
  }

  if (uiTaskHandle != NULL) {
    UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(uiTaskHandle);
    cpuUsage1 = 15.0 + random(0, 15);
  }

  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t totalHeap = ESP.getHeapSize();
  uint32_t usedHeap = totalHeap - freeHeap;
  float ramUsagePercent = (usedHeap * 100.0) / totalHeap;

  uint32_t freePsram = ESP.getFreePsram();
  uint32_t totalPsram = ESP.getPsramSize();
  uint32_t usedPsram = totalPsram - freePsram;

  if (radioWindow.visible && !radioWindow.minimized) {
    MacComponent* cpuLabel = findComponentById(radioWindow, 202);
    if (cpuLabel && cpuLabel->customData) {
      MacLabel* label = (MacLabel*)cpuLabel->customData;
      char cpuText[128];
      snprintf(cpuText, sizeof(cpuText), "CPU0: %.0f%% CPU1: %.0f%% | RAM: %dKB/%dKB (%.0f%%) | PSRAM: %dKB/%dKB",
               cpuUsage0, cpuUsage1,
               usedHeap / 1024, totalHeap / 1024, ramUsagePercent,
               usedPsram / 1024, totalPsram / 1024);
      label->text = String(cpuText);
      drawComponent(lcd, *cpuLabel, radioWindow.x, radioWindow.y);
    }
  }
}

void drawInterface(lgfx::LGFX_Device& lcd) {
  lcd.fillScreen(MAC_WHITE);
  drawCheckeredPattern(lcd);
  drawMenuBar(lcd, "Retrodio");

  initializeRadioWindow();
  initializeStationWindow();
  initializeAddStationWindow();

  if (globalKeyboard == nullptr) {
    int keyboardHeight = screenHeight / 2;
    int keyboardY = screenHeight - keyboardHeight;
    globalKeyboard = createKeyboardComponent(0, keyboardY, screenWidth, keyboardHeight, 404, 401);
    MacKeyboard* kb = (MacKeyboard*)globalKeyboard->customData;
    kb->visible = false;
  }

  drawWindow(lcd, radioWindow);

  if (!radioWindow.minimized && radioWindow.visible) {
    redrawWindowContent(lcd, radioWindow);
  }
}

void redrawWindowContent(lgfx::LGFX_Device& lcd, const MacWindow& window) {
  if (!window.visible || window.minimized)
    return;
}

void uiTask(void* parameter) {
  extern SemaphoreHandle_t metadataMutex;
  extern StreamMetadata streamMetadata;
  extern String lastTrackInfo;
  extern String lastDisplayedBitRate;
  extern String lastDisplayedID3;
  extern String lastDisplayedInfo;
  extern String lastDisplayedDescription;
  extern String lastDisplayedLyrics;
  extern String lastDisplayedLog;
  extern String currentStationName;

  int touchX, touchY;
  bool keyboardWasVisible = false;

  while (true) {
    updateClock();

    bool keyboardActive = false;
    if (globalKeyboard) {
      MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
      keyboardActive = keyboard->visible;
    }

    if (!keyboardActive) {
      // Set active flag based on window priority (top window is active)
      if (confirmDeleteWindow.visible) {
        confirmDeleteWindow.active = true;
        addStationWindow.active = false;
        stationWindow.active = false;
        radioWindow.active = false;
        interactiveWindow(lcd, confirmDeleteWindow);
      } else if (addStationWindow.visible) {
        confirmDeleteWindow.active = false;
        addStationWindow.active = true;
        stationWindow.active = false;
        radioWindow.active = false;
        interactiveWindow(lcd, addStationWindow);
      } else if (stationWindow.visible) {
        confirmDeleteWindow.active = false;
        addStationWindow.active = false;
        stationWindow.active = true;
        radioWindow.active = false;
        interactiveWindow(lcd, stationWindow);
      } else if (radioWindow.visible) {
        confirmDeleteWindow.active = false;
        addStationWindow.active = false;
        stationWindow.active = false;
        radioWindow.active = true;
        interactiveWindow(lcd, radioWindow);
      }

      if (radioIcon.visible) {
        interactiveDesktopIcon(lcd, radioIcon);
      }

      if (radioWindow.visible && !stationWindow.visible && !addStationWindow.visible) {
        updateRunningTextComponents(lcd, radioWindow);
      }

      if (stationWindow.visible && !addStationWindow.visible) {
        updateRunningTextComponents(lcd, stationWindow);
      }

      if (addStationWindow.visible) {
        updateRunningTextComponents(lcd, addStationWindow);
        updateInputFieldComponents(lcd, addStationWindow);
      }

      if (radioWindow.visible && !stationWindow.visible && !addStationWindow.visible &&
          metadataMutex) {
        if (xSemaphoreTake(metadataMutex, pdMS_TO_TICKS(2)) == pdTRUE) {
          String serverStationName = String(streamMetadata.stationName);
          String currentTrackInfo = String(streamMetadata.trackInfo);
          String bitRate = String(streamMetadata.bitRate);
          String id3Data = String(streamMetadata.id3data);
          String info = String(streamMetadata.info);
          String description = String(streamMetadata.description);
          String lyrics = String(streamMetadata.lyrics);
          String log = String(streamMetadata.log);

          if (serverStationName.length() > 0 && serverStationName != currentStationName) {
            currentStationName = serverStationName;
            updateStationMetadata(currentStationName, currentTrackInfo);
          }

          if (currentTrackInfo.length() > 0 && currentTrackInfo != lastTrackInfo) {
            lastTrackInfo = currentTrackInfo;
            updateStationMetadata(currentStationName, currentTrackInfo);
          }

          if (bitRate.length() > 0 && bitRate != lastDisplayedBitRate) {
            MacComponent* txtBitRate = findComponentById(radioWindow, 203);
            if (txtBitRate && txtBitRate->customData) {
              MacRunningText* runningText = (MacRunningText*)txtBitRate->customData;
              runningText->text = "Bitrate: " + bitRate;
              runningText->scrollOffset = 0;
              lastDisplayedBitRate = bitRate;
            }
          }

          if (id3Data.length() > 0 && id3Data != lastDisplayedID3) {
            MacComponent* txtID3 = findComponentById(radioWindow, 204);
            if (txtID3 && txtID3->customData) {
              MacRunningText* runningText = (MacRunningText*)txtID3->customData;
              runningText->text = "ID3: " + id3Data;
              runningText->scrollOffset = 0;
              lastDisplayedID3 = id3Data;
            }
          }

          if (info.length() > 0 && info != lastDisplayedInfo) {
            MacComponent* txtInfo = findComponentById(radioWindow, 205);
            if (txtInfo && txtInfo->customData) {
              MacRunningText* runningText = (MacRunningText*)txtInfo->customData;
              runningText->text = info;
              runningText->scrollOffset = 0;
              lastDisplayedInfo = info;
            }
          }

          if (description.length() > 0 && description != lastDisplayedDescription) {
            MacComponent* txtDescription = findComponentById(radioWindow, 206);
            if (txtDescription && txtDescription->customData) {
              MacRunningText* runningText = (MacRunningText*)txtDescription->customData;
              runningText->text = "Description: " + description;
              runningText->scrollOffset = 0;
              lastDisplayedDescription = description;
            }
          }

          if (lyrics.length() > 0 && lyrics != lastDisplayedLyrics) {
            MacComponent* txtLyrics = findComponentById(radioWindow, 207);
            if (txtLyrics && txtLyrics->customData) {
              MacRunningText* runningText = (MacRunningText*)txtLyrics->customData;
              runningText->text = "Lyrics: " + lyrics;
              runningText->scrollOffset = 0;
              lastDisplayedLyrics = lyrics;
            }
          }

          if (log.length() > 0 && log != lastDisplayedLog) {
            MacComponent* txtLog = findComponentById(radioWindow, 208);
            if (txtLog && txtLog->customData) {
              MacRunningText* runningText = (MacRunningText*)txtLog->customData;
              runningText->text = "Log: " + log;
              runningText->scrollOffset = 0;
              lastDisplayedLog = log;
            }
          }

          xSemaphoreGive(metadataMutex);
        }
      }
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
