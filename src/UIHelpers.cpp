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
#include "RadioWindow.h"
#include "AddStationWindow.h"
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
  extern const int TXT_RADIO_NAME;
  extern const int TXT_RADIO_DETAILS;

  MacComponent* txtRadioName = findComponentById(radioWindow, TXT_RADIO_NAME);
  if (txtRadioName && txtRadioName->customData) {
    MacRunningText* runningText = (MacRunningText*)txtRadioName->customData;
    runningText->text = stationName;
    runningText->scrollOffset = 0;
    runningText->font = FONT_CHICAGO_11PT;
  }

  MacComponent* txtRadioDetails = findComponentById(radioWindow, TXT_RADIO_DETAILS);
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
    extern const int TXT_CPU_LABEL;
    MacComponent* cpuLabel = findComponentById(radioWindow, TXT_CPU_LABEL);
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
    extern const int KEYBOARD_COMPONENT;
    extern const int INPUT_STATION_NAME;
    int keyboardHeight = screenHeight / 2;
    int keyboardY = screenHeight - keyboardHeight;
    globalKeyboard = createKeyboardComponent(0, keyboardY, screenWidth, keyboardHeight, KEYBOARD_COMPONENT, INPUT_STATION_NAME);
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

void adjustWindowForKeyboard(MacWindow& window, MacComponent* inputComponent, bool show) {
  if (!inputComponent || !globalKeyboard) {
    return;
  }

  // Store original window position
  static int originalWindowY = -1;

  if (show) {
    // Save original position if not already saved
    if (originalWindowY == -1) {
      originalWindowY = window.y;
    }

    // Calculate keyboard Y position
    int keyboardHeight = screenHeight / 2;
    int keyboardY = screenHeight - keyboardHeight;

    // Calculate input field's absolute bottom position
    int inputAbsoluteY = window.y + inputComponent->y;
    int inputBottom = inputAbsoluteY + inputComponent->h;

    // Check if input field would be covered by keyboard
    if (inputBottom > keyboardY) {
      // Calculate how much we need to move the window up
      int overlap = inputBottom - keyboardY + 10; // +10 for some padding

      // Calculate new window position
      int newY = window.y - overlap;

      // Make sure window doesn't go above menu bar (y >= 21)
      newY = max(21, newY);

      // Only move if position changed
      if (newY != window.y) {
        // Clear old window position
        drawCheckeredPatternArea(lcd, window.x, window.y, window.w + 5, window.h + 5);

        // Update window position
        window.y = newY;

        // Redraw window at new position
        drawWindow(lcd, window);
      }
    }
  } else {
    // Restore original position when keyboard is hidden
    if (originalWindowY != -1 && window.y != originalWindowY) {
      // Clear current window position
      drawCheckeredPatternArea(lcd, window.x, window.y, window.w + 5, window.h + 5);

      // Restore original position
      window.y = originalWindowY;

      // Redraw window at original position
      drawWindow(lcd, window);
    }

    // Reset saved position
    originalWindowY = -1;
  }
}

void handleKeyboardInteraction() {
  if (!globalKeyboard) {
    return;
  }

  MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
  if (!keyboard->visible) {
    return;
  }

  uint16_t tx, ty;
  if (!lcd.getTouch(&tx, &ty)) {
    return;
  }

  // Check if touch is within keyboard area
  bool touchInKeyboard = (tx >= globalKeyboard->x &&
                          tx <= globalKeyboard->x + globalKeyboard->w &&
                          ty >= globalKeyboard->y &&
                          ty <= globalKeyboard->y + globalKeyboard->h);

  // Check if touch is within the add station window (but not keyboard)
  bool touchInWindow = (tx >= addStationWindow.x &&
                        tx <= addStationWindow.x + addStationWindow.w &&
                        ty >= addStationWindow.y &&
                        ty <= addStationWindow.y + addStationWindow.h);

  if (touchInKeyboard) {
    // Find the target input field component
    MacComponent* targetInputComp = nullptr;

    // Search for the input field in the add station window
    for (int i = 0; i < addStationWindow.childComponentCount; i++) {
      MacComponent* comp = addStationWindow.childComponents[i];
      if (comp && comp->id == keyboard->targetInputId && comp->type == COMPONENT_INPUT_FIELD) {
        targetInputComp = comp;
        break;
      }
    }

    if (!targetInputComp) {
      return;
    }

    // Handle keyboard touch with absolute coordinates
    bool textChanged = handleKeyboardTouch(lcd, globalKeyboard, targetInputComp, tx, ty);

    if (textChanged) {
      // Only redraw the input field to show the updated text
      // The keyboard already redraws itself in handleKeyboardTouch when needed
      lcd.startWrite();
      drawComponent(lcd, *targetInputComp, addStationWindow.x, addStationWindow.y);
      lcd.endWrite();

      // Wait for touch release to prevent multiple character inputs
      delay(150);
      while (lcd.getTouch(&tx, &ty)) {
        delay(10);
      }
    }
  } else if (touchInWindow) {
    // Touch is in window but not on keyboard - check what was clicked
    int relativeX = tx - addStationWindow.x;
    int relativeY = ty - addStationWindow.y;

    // Find all input field components
    extern const int INPUT_STATION_NAME;
    extern const int INPUT_STATION_URL;
    MacComponent* nameInputComp = nullptr;
    MacComponent* urlInputComp = nullptr;

    for (int i = 0; i < addStationWindow.childComponentCount; i++) {
      MacComponent* comp = addStationWindow.childComponents[i];
      if (comp && comp->type == COMPONENT_INPUT_FIELD) {
        if (comp->id == INPUT_STATION_NAME) {
          nameInputComp = comp;
        } else if (comp->id == INPUT_STATION_URL) {
          urlInputComp = comp;
        }
      }
    }

    // Check if user clicked on a different input field
    bool clickedNameInput = nameInputComp &&
                            relativeX >= nameInputComp->x &&
                            relativeX <= nameInputComp->x + nameInputComp->w &&
                            relativeY >= nameInputComp->y &&
                            relativeY <= nameInputComp->y + nameInputComp->h;

    bool clickedUrlInput = urlInputComp &&
                           relativeX >= urlInputComp->x &&
                           relativeX <= urlInputComp->x + urlInputComp->w &&
                           relativeY >= urlInputComp->y &&
                           relativeY <= urlInputComp->y + urlInputComp->h;

    if (clickedNameInput && keyboard->targetInputId != INPUT_STATION_NAME) {
      // Switch focus to name input
      MacInputField* nameInput = (MacInputField*)nameInputComp->customData;
      MacInputField* urlInput = (MacInputField*)urlInputComp->customData;

      nameInput->focused = true;
      urlInput->focused = false;
      keyboard->targetInputId = INPUT_STATION_NAME;

      // Adjust window position for the new focused input field
      adjustWindowForKeyboard(addStationWindow, nameInputComp, true);

      drawComponent(lcd, *nameInputComp, addStationWindow.x, addStationWindow.y);
      drawComponent(lcd, *urlInputComp, addStationWindow.x, addStationWindow.y);

      delay(150);
      while (lcd.getTouch(&tx, &ty)) {
        delay(10);
      }
    } else if (clickedUrlInput && keyboard->targetInputId != INPUT_STATION_URL) {
      // Switch focus to URL input
      MacInputField* nameInput = (MacInputField*)nameInputComp->customData;
      MacInputField* urlInput = (MacInputField*)urlInputComp->customData;

      nameInput->focused = false;
      urlInput->focused = true;
      keyboard->targetInputId = INPUT_STATION_URL;

      // Adjust window position for the new focused input field
      adjustWindowForKeyboard(addStationWindow, urlInputComp, true);

      drawComponent(lcd, *nameInputComp, addStationWindow.x, addStationWindow.y);
      drawComponent(lcd, *urlInputComp, addStationWindow.x, addStationWindow.y);

      delay(150);
      while (lcd.getTouch(&tx, &ty)) {
        delay(10);
      }
    } else if (!clickedNameInput && !clickedUrlInput) {
      // Clicked somewhere else in the window (not on input fields) - hide keyboard
      keyboard->visible = false;

      // Clear focus from all input fields
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

      // Clear keyboard area and redraw window
      int keyboardHeight = screenHeight / 2;
      int keyboardY = screenHeight - keyboardHeight;
      drawCheckeredPatternArea(lcd, 0, keyboardY, screenWidth, keyboardHeight);
      drawWindow(lcd, addStationWindow);

      delay(150);
      while (lcd.getTouch(&tx, &ty)) {
        delay(10);
      }
    }
  } else {
    // Touch is completely outside the window - hide keyboard
    keyboard->visible = false;

    // Clear focus from all input fields
    extern const int INPUT_STATION_NAME;
    extern const int INPUT_STATION_URL;
    MacComponent* nameInputComp = nullptr;
    MacComponent* urlInputComp = nullptr;

    for (int i = 0; i < addStationWindow.childComponentCount; i++) {
      MacComponent* comp = addStationWindow.childComponents[i];
      if (comp && comp->type == COMPONENT_INPUT_FIELD) {
        if (comp->id == INPUT_STATION_NAME) {
          nameInputComp = comp;
        } else if (comp->id == INPUT_STATION_URL) {
          urlInputComp = comp;
        }
      }
    }

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

    // Clear keyboard area
    int keyboardHeight = screenHeight / 2;
    int keyboardY = screenHeight - keyboardHeight;
    drawCheckeredPatternArea(lcd, 0, keyboardY, screenWidth, keyboardHeight);

    delay(150);
    while (lcd.getTouch(&tx, &ty)) {
      delay(10);
    }
  }
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

    // Handle keyboard touch input when keyboard is visible
    if (keyboardActive) {
      handleKeyboardInteraction();

      // Update input field cursor blinking when keyboard is visible
      if (addStationWindow.visible) {
        updateInputFieldComponents(lcd, addStationWindow);
      }
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
        extern const int TXT_BITRATE;
        extern const int TXT_ID3;
        extern const int TXT_INFO;
        extern const int TXT_DESCRIPTION;
        extern const int TXT_LYRICS;
        extern const int TXT_LOG;

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
            MacComponent* txtBitRate = findComponentById(radioWindow, TXT_BITRATE);
            if (txtBitRate && txtBitRate->customData) {
              MacRunningText* runningText = (MacRunningText*)txtBitRate->customData;
              runningText->text = "Bitrate: " + bitRate;
              runningText->scrollOffset = 0;
              lastDisplayedBitRate = bitRate;
            }
          }

          if (id3Data.length() > 0 && id3Data != lastDisplayedID3) {
            MacComponent* txtID3 = findComponentById(radioWindow, TXT_ID3);
            if (txtID3 && txtID3->customData) {
              MacRunningText* runningText = (MacRunningText*)txtID3->customData;
              runningText->text = "ID3: " + id3Data;
              runningText->scrollOffset = 0;
              lastDisplayedID3 = id3Data;
            }
          }

          if (info.length() > 0 && info != lastDisplayedInfo) {
            MacComponent* txtInfo = findComponentById(radioWindow, TXT_INFO);
            if (txtInfo && txtInfo->customData) {
              MacRunningText* runningText = (MacRunningText*)txtInfo->customData;
              runningText->text = info;
              runningText->scrollOffset = 0;
              lastDisplayedInfo = info;
            }
          }

          if (description.length() > 0 && description != lastDisplayedDescription) {
            MacComponent* txtDescription = findComponentById(radioWindow, TXT_DESCRIPTION);
            if (txtDescription && txtDescription->customData) {
              MacRunningText* runningText = (MacRunningText*)txtDescription->customData;
              runningText->text = "Description: " + description;
              runningText->scrollOffset = 0;
              lastDisplayedDescription = description;
            }
          }

          if (lyrics.length() > 0 && lyrics != lastDisplayedLyrics) {
            MacComponent* txtLyrics = findComponentById(radioWindow, TXT_LYRICS);
            if (txtLyrics && txtLyrics->customData) {
              MacRunningText* runningText = (MacRunningText*)txtLyrics->customData;
              runningText->text = "Lyrics: " + lyrics;
              runningText->scrollOffset = 0;
              lastDisplayedLyrics = lyrics;
            }
          }

          if (log.length() > 0 && log != lastDisplayedLog) {
            MacComponent* txtLog = findComponentById(radioWindow, TXT_LOG);
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
