/*
 * UIHelpers.cpp - UI Helper Functions Implementation
 *
 * Copyright (c) 2025 felangga
 *
 * This file implements UI helper functions
 */

#include "UIHelpers.h"
#include <WiFi.h>
#include <time.h>
#include "AddStationWindow.h"
#include "ConfigManager.h"
#include "GlobalState.h"
#include "RadioWindow.h"
#include "SettingsWindow.h"
#include "StationManager.h"
#include "WifiWindow.h"
#include "config.h"
#include "wt32_sc01_plus.h"

#define ENABLE_DEBUG 0

extern LGFX lcd;
extern UIWindow radioWindow;
extern UIWindow stationWindow;
extern UIWindow addStationWindow;
extern UIWindow wifiWindow;
extern DesktopIcon radioIcon;
extern UIComponent* globalKeyboard;

void updateStationMetadata(const String& stationName, const String& trackInfo) {
  extern const int TXT_RADIO_NAME;
  extern const int TXT_RADIO_DETAILS;
  extern bool volumeDisplayActive;
  extern String savedStationName;

  UIComponent* txtRadioName = findComponentById(radioWindow, TXT_RADIO_NAME);
  if (txtRadioName && txtRadioName->customData) {
    UIRunningText* runningText = (UIRunningText*)txtRadioName->customData;

    // If volume is being displayed, save the station name for later
    // but don't update the display yet
    if (volumeDisplayActive) {
      savedStationName = stationName;
    } else {
      runningText->text = stationName;
      runningText->scrollOffset = 0;
    }
  }

  UIComponent* txtRadioDetails = findComponentById(radioWindow, TXT_RADIO_DETAILS);
  if (txtRadioDetails && txtRadioDetails->customData) {
    UIRunningText* runningText = (UIRunningText*)txtRadioDetails->customData;
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
  if (WiFi.status() != WL_CONNECTED) {
    drawClock(lcd, "--:--:--");
    return;
  }

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

static unsigned long lastWifiUpdate = 0;
static int lastRssi = 1;  // Use 1 as "not yet initialized" (valid RSSI is always negative)

void updateWifiSignal() {
  unsigned long now = millis();
  if (now - lastWifiUpdate < 2000)
    return;
  lastWifiUpdate = now;

  if (WiFi.status() != WL_CONNECTED) {
    if (lastRssi != -100) {
      lastRssi = -100;
      drawWifiSignal(lcd, lastRssi);
    }
    return;
  }

  int rssi = WiFi.RSSI();

  if (rssi == 0) {
    rssi = -100;
  }

  if (lastRssi > 0 || abs(rssi - lastRssi) >= 5) {
    lastRssi = rssi;
    drawWifiSignal(lcd, rssi);
  }
}

struct NotificationState {
  bool visible = false;
  String message = "";
  unsigned long startTime = 0;
  unsigned long duration = 0;  // 0 = permanent until hidden
};

static NotificationState notification;

void showNotification(const String& message, unsigned long duration) {
  notification.message = message;
  notification.duration = duration;
  notification.startTime = millis();
  notification.visible = true;

  drawBottomBar(lcd, message, true);
}

UIWindow** getVisibleWindows(int& windowCount) {
  static UIWindow* visibleWindows[10];
  windowCount = 0;

  int totalWindows = 0;
  UIWindow** allWindows = getRegisteredWindows(totalWindows);

  if (allWindows == nullptr) {
    return visibleWindows;
  }

  for (int i = 0; i < totalWindows; i++) {
    if (allWindows[i] != nullptr && allWindows[i]->visible && !allWindows[i]->minimized) {
      visibleWindows[windowCount++] = allWindows[i];
    }
  }

  return visibleWindows;
}

void hideNotification() {
  if (!notification.visible)
    return;

  notification.visible = false;
  notification.message = "";

  drawBottomBar(lcd, "", false);
}

void updateNotification() {
  if (!notification.visible)
    return;

  if (notification.duration > 0) {
    unsigned long elapsed = millis() - notification.startTime;
    if (elapsed >= notification.duration) {
      hideNotification();
      return;
    }
  }
}

void updateVolumeDisplay() {
  extern unsigned long volumeChangeTime;
  extern bool volumeDisplayActive;
  extern String savedStationName;
  extern String currentStationName;
  extern const int TXT_RADIO_NAME;

  if (!volumeDisplayActive)
    return;

  unsigned long elapsed = millis() - volumeChangeTime;

  // After 2 seconds (2000ms), restore the radio name
  if (elapsed >= 2000) {
    UIComponent* txtRadioName = findComponentById(radioWindow, TXT_RADIO_NAME);
    if (txtRadioName && txtRadioName->customData) {
      UIRunningText* runningText = (UIRunningText*)txtRadioName->customData;
      runningText->text = savedStationName;
      runningText->scrollOffset = 0;
      currentStationName = savedStationName;
    }
    volumeDisplayActive = false;
  }
}

void drawInterface(lgfx::LGFX_Device& lcd) {
  lcd.fillScreen(UI_WHITE);
  drawCheckeredPattern(lcd);
  drawMenuBar(lcd, "Retrodio");
  drawBottomBar(lcd, "", false);

  initializeRadioWindow();
  initializeStationWindow();
  initializeAddStationWindow();
  initializeSettingsWindow();

  if (globalKeyboard == nullptr) {
    extern const int KEYBOARD_COMPONENT;
    extern const int INPUT_STATION_NAME;
    int keyboardHeight = screenHeight / 2;
    int keyboardY = screenHeight - keyboardHeight;
    globalKeyboard = createKeyboardComponent(0, keyboardY, screenWidth, keyboardHeight,
                                             KEYBOARD_COMPONENT, INPUT_STATION_NAME);
    UIKeyboard* kb = (UIKeyboard*)globalKeyboard->customData;
    kb->visible = false;
  }

  drawWindow(lcd, radioWindow);

  if (!radioWindow.minimized && radioWindow.visible) {
    redrawWindowContent(lcd, radioWindow);
  }
}

void redrawWindowContent(lgfx::LGFX_Device& lcd, const UIWindow& window) {
  if (!window.visible || window.minimized)
    return;
}

void adjustWindowForKeyboard(UIWindow& window, UIComponent* inputComponent, bool show) {
  if (!inputComponent || !globalKeyboard) {
    return;
  }

  static int originalWindowY = -1;

  if (show) {
    if (originalWindowY == -1) {
      originalWindowY = window.y;
    }

    int keyboardHeight = screenHeight / 2;
    int keyboardY = screenHeight - keyboardHeight;

    int inputAbsoluteY = window.y + inputComponent->y;
    int inputBottom = inputAbsoluteY + inputComponent->h;

    if (inputBottom > keyboardY) {
      int overlap = inputBottom - keyboardY + 10;

      int newY = window.y - overlap;

      newY = max(21, newY);

      if (newY != window.y) {
        drawCheckeredPatternArea(lcd, window.x, window.y, window.w + 5, window.h + 5);

        window.y = newY;

        drawWindow(lcd, window);
      }
    }
  } else {
    if (originalWindowY != -1 && window.y != originalWindowY) {
      drawCheckeredPatternArea(lcd, window.x, window.y, window.w + 5, window.h + 5);

      window.y = originalWindowY;

      drawWindow(lcd, window);
    }

    originalWindowY = -1;
  }
}

void handleKeyboardInteraction() {
  if (!globalKeyboard) {
    return;
  }

  UIKeyboard* keyboard = (UIKeyboard*)globalKeyboard->customData;
  if (!keyboard->visible) {
    return;
  }

  // Find the target input field component
  UIComponent* targetInputComp = nullptr;
  for (int i = 0; i < addStationWindow.childComponentCount; i++) {
    UIComponent* comp = addStationWindow.childComponents[i];
    if (comp && comp->id == keyboard->targetInputId && comp->type == COMPONENT_INPUT_FIELD) {
      targetInputComp = comp;
      break;
    }
  }

  if (!targetInputComp) {
    return;
  }

  uint16_t tx, ty;
  bool isTouching = lcd.getTouch(&tx, &ty);

  if (!isTouching) {
    // No touch detected - reset key press state and restore key appearance
    if (keyboard->isKeyPressed) {
      // Check if we need to deactivate shift after typing a character
      // Only deactivate if shift is not locked (caps lock mode)
      bool needShiftDeactivation = keyboard->shiftActive && !keyboard->shiftLocked &&
                                   keyboard->selectedKey != -2 && keyboard->lastPressedChar != '\0';

      // Deactivate shift if it was active (but not locked) and a character was typed
      if (needShiftDeactivation) {
        keyboard->shiftActive = false;
      }

      // Restore just the pressed key to normal state (not the whole keyboard)
      extern void restorePressedKey(lgfx::LGFX_Device & lcd, UIKeyboard * keyboard, int x, int y,
                                    int w, int h);
      restorePressedKey(lcd, keyboard, globalKeyboard->x, globalKeyboard->y, globalKeyboard->w,
                        globalKeyboard->h);

      // If shift was deactivated, we need to redraw the shift button too
      if (needShiftDeactivation) {
        // Only redraw the shift key button, not the whole keyboard
        int rowHeight = (globalKeyboard->h - (2 * 5) - 28 - 2) /
                        4;  // KEYBOARD_MARGIN=5, SPECIAL_ROW_HEIGHT=28, KEY_SPACING=2
        int row3Y = globalKeyboard->y + 5 + 3 * rowHeight;
        int shiftX = globalKeyboard->x + 5;

        lcd.startWrite();
        lcd.fillRoundRect(shiftX, row3Y, 45, rowHeight - 2, 4,
                          UI_WHITE);  // SHIFT_WIDTH=45, radius=4
        lcd.drawRoundRect(shiftX, row3Y, 45, rowHeight - 2, 4, UI_BLACK);
        lcd.drawRoundRect(shiftX + 1, row3Y + 1, 45 - 2, rowHeight - 2 - 2, 2, UI_BLACK);
        lcd.setTextColor(UI_BLACK, UI_WHITE);
        lcd.setTextSize(1);
        extern const GFXfont* getFontFromType(FontType fontType);
        lcd.setFont(getFontFromType(FONT_CHICAGO_9PT));
        int shiftTextW = lcd.textWidth("Shift");
        lcd.setCursor(shiftX + (45 - shiftTextW) / 2, row3Y + (rowHeight - 2) / 2);
        lcd.print("Shift");
        lcd.endWrite();
      }

      keyboard->isKeyPressed = false;
      keyboard->isBackspace = false;
      keyboard->isSpace = false;
      keyboard->lastPressedChar = '\0';
    }
    return;
  }

  // Check if touch is within keyboard area
  bool touchInKeyboard = (tx >= globalKeyboard->x && tx <= globalKeyboard->x + globalKeyboard->w &&
                          ty >= globalKeyboard->y && ty <= globalKeyboard->y + globalKeyboard->h);

  // Check if touch is within the add station window (but not keyboard)
  bool touchInWindow = (tx >= addStationWindow.x && tx <= addStationWindow.x + addStationWindow.w &&
                        ty >= addStationWindow.y && ty <= addStationWindow.y + addStationWindow.h);

  if (touchInKeyboard) {
    // Handle keyboard touch with absolute coordinates
    bool textChanged =
        handleKeyboardTouch(lcd, globalKeyboard, targetInputComp, tx, ty, &addStationWindow);

    if (textChanged) {
      // Only redraw the input field to show the updated text
      lcd.startWrite();
      drawComponent(lcd, *targetInputComp, addStationWindow.x, addStationWindow.y);
      lcd.endWrite();
    }

    // Handle key repeat while key is held
    handleKeyboardRepeat(lcd, globalKeyboard, targetInputComp, &addStationWindow);
  } else if (touchInWindow) {
    // Touch is in window but not on keyboard - check what was clicked
    int relativeX = tx - addStationWindow.x;
    int relativeY = ty - addStationWindow.y;

    // Find all input field components
    extern const int INPUT_STATION_NAME;
    extern const int INPUT_STATION_URL;
    UIComponent* nameInputComp = nullptr;
    UIComponent* urlInputComp = nullptr;

    for (int i = 0; i < addStationWindow.childComponentCount; i++) {
      UIComponent* comp = addStationWindow.childComponents[i];
      if (comp && comp->type == COMPONENT_INPUT_FIELD) {
        if (comp->id == INPUT_STATION_NAME) {
          nameInputComp = comp;
        } else if (comp->id == INPUT_STATION_URL) {
          urlInputComp = comp;
        }
      }
    }

    // Check if user clicked on a different input field
    bool clickedNameInput = nameInputComp && relativeX >= nameInputComp->x &&
                            relativeX <= nameInputComp->x + nameInputComp->w &&
                            relativeY >= nameInputComp->y &&
                            relativeY <= nameInputComp->y + nameInputComp->h;

    bool clickedUrlInput = urlInputComp && relativeX >= urlInputComp->x &&
                           relativeX <= urlInputComp->x + urlInputComp->w &&
                           relativeY >= urlInputComp->y &&
                           relativeY <= urlInputComp->y + urlInputComp->h;

    if (clickedNameInput && keyboard->targetInputId != INPUT_STATION_NAME) {
      // Switch focus to name input
      UIInputField* nameInput = (UIInputField*)nameInputComp->customData;
      UIInputField* urlInput = (UIInputField*)urlInputComp->customData;

      nameInput->focused = true;
      urlInput->focused = false;
      keyboard->targetInputId = INPUT_STATION_NAME;

      // Adjust window position for the new focused input field
      adjustWindowForKeyboard(addStationWindow, nameInputComp, true);

      drawComponent(lcd, *nameInputComp, addStationWindow.x, addStationWindow.y);
      drawComponent(lcd, *urlInputComp, addStationWindow.x, addStationWindow.y);
    } else if (clickedUrlInput && keyboard->targetInputId != INPUT_STATION_URL) {
      // Switch focus to URL input
      UIInputField* nameInput = (UIInputField*)nameInputComp->customData;
      UIInputField* urlInput = (UIInputField*)urlInputComp->customData;

      nameInput->focused = false;
      urlInput->focused = true;
      keyboard->targetInputId = INPUT_STATION_URL;

      // Adjust window position for the new focused input field
      adjustWindowForKeyboard(addStationWindow, urlInputComp, true);

      drawComponent(lcd, *nameInputComp, addStationWindow.x, addStationWindow.y);
      drawComponent(lcd, *urlInputComp, addStationWindow.x, addStationWindow.y);
    } else if (!clickedNameInput && !clickedUrlInput) {
      // Clicked somewhere else in the window (not on input fields) - hide keyboard
      keyboard->visible = false;

      // Clear focus from all input fields
      if (nameInputComp && nameInputComp->customData) {
        UIInputField* nameInput = (UIInputField*)nameInputComp->customData;
        nameInput->focused = false;
      }
      if (urlInputComp && urlInputComp->customData) {
        UIInputField* urlInput = (UIInputField*)urlInputComp->customData;
        urlInput->focused = false;
      }

      // Restore window to original position
      adjustWindowForKeyboard(addStationWindow, nullptr, false);

      // Clear keyboard area and redraw window
      int keyboardHeight = screenHeight / 2;
      int keyboardY = screenHeight - keyboardHeight;
      drawCheckeredPatternArea(lcd, 0, keyboardY, screenWidth, keyboardHeight);
      drawWindow(lcd, addStationWindow);
    }
  } else {
    // Touch is completely outside the window - hide keyboard
    keyboard->visible = false;

    // Clear focus from all input fields
    extern const int INPUT_STATION_NAME;
    extern const int INPUT_STATION_URL;
    UIComponent* nameInputComp = nullptr;
    UIComponent* urlInputComp = nullptr;

    for (int i = 0; i < addStationWindow.childComponentCount; i++) {
      UIComponent* comp = addStationWindow.childComponents[i];
      if (comp && comp->type == COMPONENT_INPUT_FIELD) {
        if (comp->id == INPUT_STATION_NAME) {
          nameInputComp = comp;
        } else if (comp->id == INPUT_STATION_URL) {
          urlInputComp = comp;
        }
      }
    }

    if (nameInputComp && nameInputComp->customData) {
      UIInputField* nameInput = (UIInputField*)nameInputComp->customData;
      nameInput->focused = false;
    }
    if (urlInputComp && urlInputComp->customData) {
      UIInputField* urlInput = (UIInputField*)urlInputComp->customData;
      urlInput->focused = false;
    }

    // Restore window to original position
    adjustWindowForKeyboard(addStationWindow, nullptr, false);

    // Clear keyboard area
    int keyboardHeight = screenHeight / 2;
    int keyboardY = screenHeight - keyboardHeight;
    drawCheckeredPatternArea(lcd, 0, keyboardY, screenWidth, keyboardHeight);
  }
}

// Check if touch is on WiFi signal icon in menu bar
bool isInsideWifiSignal(int tx, int ty) {
  // WiFi signal is at screenWidth - 100, height 0-20
  int wifiX = screenWidth - 100;
  return tx >= wifiX - 5 && tx <= wifiX + 20 && ty >= 0 && ty <= 20;
}

static bool menuBarTouchActive = false;
static unsigned long lastMenuBarTouch = 0;

void checkMenuBarTouch() {
  uint16_t tx, ty;
  bool touching = lcd.getTouch(&tx, &ty);

  if (touching && !menuBarTouchActive) {
    // Check if in menu bar area (top 20 pixels)
    if (ty <= 20) {
      // Check if clicking on "Retrodio" text (approximately x: 30-110, y: 0-20)
      if (tx >= 30 && tx <= 110) {
        menuBarTouchActive = true;
        lastMenuBarTouch = millis();

        // Debounce
        delay(100);

        // Open settings window
        extern void onSettingsIconClick();
        onSettingsIconClick();
      }
      // Check if on WiFi signal
      else if (isInsideWifiSignal(tx, ty)) {
        menuBarTouchActive = true;
        lastMenuBarTouch = millis();

        // Debounce
        delay(100);

        // Call WiFi signal click handler
        extern void onWifiSignalClick();
        onWifiSignalClick();
      }
    }
  } else if (!touching) {
    menuBarTouchActive = false;
  }
}

void handleWifiKeyboardInteraction() {
  extern UIComponent* wifiKeyboard;
  extern UIWindow wifiWindow;

  if (!wifiKeyboard)
    return;

  UIKeyboard* keyboard = (UIKeyboard*)wifiKeyboard->customData;
  if (!keyboard->visible)
    return;

  // Find the target input field component
  UIComponent* targetInputComp = nullptr;
  for (int i = 0; i < wifiWindow.childComponentCount; i++) {
    UIComponent* comp = wifiWindow.childComponents[i];
    if (comp && comp->id == keyboard->targetInputId && comp->type == COMPONENT_INPUT_FIELD) {
      targetInputComp = comp;
      break;
    }
  }

  if (!targetInputComp)
    return;

  uint16_t tx, ty;
  bool isTouching = lcd.getTouch(&tx, &ty);

  if (!isTouching) {
    // No touch detected - reset key press state and restore key appearance
    if (keyboard->isKeyPressed) {
      // Restore just the pressed key to normal state
      extern void restorePressedKey(lgfx::LGFX_Device & lcd, UIKeyboard * keyboard, int x, int y,
                                    int w, int h);
      restorePressedKey(lcd, keyboard, wifiKeyboard->x, wifiKeyboard->y, wifiKeyboard->w,
                        wifiKeyboard->h);

      keyboard->isKeyPressed = false;
      keyboard->isBackspace = false;
      keyboard->isSpace = false;
      keyboard->lastPressedChar = '\0';
    }
    return;
  }

  // Check if touch is within keyboard area
  bool touchInKeyboard = (tx >= wifiKeyboard->x && tx <= wifiKeyboard->x + wifiKeyboard->w &&
                          ty >= wifiKeyboard->y && ty <= wifiKeyboard->y + wifiKeyboard->h);

  // Check if touch is within the WiFi window (but not keyboard)
  bool touchInWindow = (tx >= wifiWindow.x && tx <= wifiWindow.x + wifiWindow.w &&
                        ty >= wifiWindow.y && ty <= wifiWindow.y + wifiWindow.h);

  if (touchInKeyboard) {
    // Handle keyboard touch with absolute coordinates
    bool textChanged = handleKeyboardTouch(lcd, wifiKeyboard, targetInputComp, tx, ty, &wifiWindow);

    if (textChanged) {
      // Only redraw the input field to show the updated text
      lcd.startWrite();
      drawComponent(lcd, *targetInputComp, wifiWindow.x, wifiWindow.y);
      lcd.endWrite();
    }

    // Handle key repeat while key is held
    handleKeyboardRepeat(lcd, wifiKeyboard, targetInputComp, &wifiWindow);
  } else if (!touchInWindow) {
    // Touch is completely outside the window - hide keyboard and restore list view
    extern void hideWifiKeyboard();
    hideWifiKeyboard();
  }
}

// ===== Power Save Functions =====

void sleepLCD() {
  if (lcdSleeping) return;

  // Save current brightness
  savedBrightness = lcd.getBrightness();

  // Turn off backlight
  lcd.setBrightness(0);
  lcd.sleep();

  lcdSleeping = true;
}

void wakeLCD() {
  if (!lcdSleeping) return;

  // Wake up display
  lcd.wakeup();
  lcd.setBrightness(savedBrightness);

  lcdSleeping = false;

  // Reset activity timer
  lastActivityTime = millis();
}

void resetActivityTimer() {
  lastActivityTime = millis();

  // Wake LCD if it's sleeping
  if (lcdSleeping) {
    wakeLCD();
  }
}

void checkPowerSave() {
  // Skip if power save is disabled
  if (!ConfigManager::getPowerSaveEnabled()) {
    return;
  }

  // Don't sleep if already sleeping
  if (lcdSleeping) {
    return;
  }

  // Check if idle timeout has elapsed
  unsigned long idleTime = millis() - lastActivityTime;
  unsigned long timeout = ConfigManager::getLcdIdleTimeout();

  if (idleTime >= timeout) {
    sleepLCD();
  }
}

// ===== UI Task =====

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

  // Initialize activity timer
  lastActivityTime = millis();

  while (true) {
    // Check for touch to wake LCD and reset activity timer
    int16_t tx, ty;
    if (lcd.getTouch(&tx, &ty)) {
      resetActivityTimer();
    }

    // Check if LCD should sleep due to inactivity
    checkPowerSave();

    // Skip UI updates if LCD is sleeping
    if (lcdSleeping) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
      continue;
    }

    updateClock();
    updateWifiSignal();
    updateNotification();
    updateVolumeDisplay();

    // Check for UI redraw requests from web server
    extern volatile bool needsVolumeSliderRedraw;
    extern volatile bool needsStationListReload;
    extern const int CMP_VOLUME_SLIDER;

    if (needsVolumeSliderRedraw) {
      needsVolumeSliderRedraw = false;
      if (radioWindow.visible && !radioWindow.minimized) {
        UIComponent* volumeSlider = findComponentById(radioWindow, CMP_VOLUME_SLIDER);
        if (volumeSlider) {
          lcd.startWrite();
          drawComponent(lcd, *volumeSlider, radioWindow.x, radioWindow.y);
          lcd.endWrite();
        }
      }
    }

    if (needsStationListReload) {
      needsStationListReload = false;
      reloadStationList();
      initializeStationWindow();

      if (stationWindow.visible && !stationWindow.minimized) {
        lcd.startWrite();
        drawWindow(lcd, stationWindow);
        lcd.endWrite();
      }
    }

    // Check WiFi connection status (non-blocking)
    updateWifiConnectionStatus();

    // Check for menu bar touch (WiFi signal click)
    checkMenuBarTouch();

    bool keyboardActive = false;
    bool wifiKeyboardActive = false;

    if (globalKeyboard) {
      UIKeyboard* keyboard = (UIKeyboard*)globalKeyboard->customData;
      keyboardActive = keyboard->visible;
    }

    extern UIComponent* wifiKeyboard;
    if (wifiKeyboard) {
      UIKeyboard* keyboard = (UIKeyboard*)wifiKeyboard->customData;
      wifiKeyboardActive = keyboard->visible;
    }

    // Handle keyboard touch input when keyboard is visible
    if (keyboardActive) {
      handleKeyboardInteraction();

      // Update input field cursor blinking when keyboard is visible
      if (addStationWindow.visible) {
        updateInputFieldComponents(lcd, addStationWindow);
      }
    }

    // Handle WiFi keyboard input when visible
    if (wifiKeyboardActive) {
      handleWifiKeyboardInteraction();

      // Update input field cursor blinking
      if (wifiWindow.visible) {
        updateInputFieldComponents(lcd, wifiWindow);
      }
    }

    if (!keyboardActive && !wifiKeyboardActive) {
      // Set active flag based on window priority (top window is active)
      if (wifiWindow.visible) {
        wifiWindow.active = true;
        settingsWindow.active = false;
        confirmDeleteWindow.active = false;
        addStationWindow.active = false;
        stationWindow.active = false;
        radioWindow.active = false;
        interactiveWindow(lcd, wifiWindow);
      } else if (settingsWindow.visible) {
        wifiWindow.active = false;
        settingsWindow.active = true;
        confirmDeleteWindow.active = false;
        addStationWindow.active = false;
        stationWindow.active = false;
        radioWindow.active = false;
        interactiveWindow(lcd, settingsWindow);
      } else if (confirmDeleteWindow.visible) {
        wifiWindow.active = false;
        settingsWindow.active = false;
        confirmDeleteWindow.active = true;
        addStationWindow.active = false;
        stationWindow.active = false;
        radioWindow.active = false;
        interactiveWindow(lcd, confirmDeleteWindow);
      } else if (addStationWindow.visible) {
        wifiWindow.active = false;
        settingsWindow.active = false;
        confirmDeleteWindow.active = false;
        addStationWindow.active = true;
        stationWindow.active = false;
        radioWindow.active = false;
        interactiveWindow(lcd, addStationWindow);
      } else if (stationWindow.visible) {
        wifiWindow.active = false;
        settingsWindow.active = false;
        confirmDeleteWindow.active = false;
        addStationWindow.active = false;
        stationWindow.active = true;
        radioWindow.active = false;
        interactiveWindow(lcd, stationWindow);
      } else if (radioWindow.visible) {
        wifiWindow.active = false;
        settingsWindow.active = false;
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

          if (bitRate != lastDisplayedBitRate) {
            UIComponent* txtBitRate = findComponentById(radioWindow, TXT_BITRATE);
            if (txtBitRate && txtBitRate->customData) {
              UIRunningText* runningText = (UIRunningText*)txtBitRate->customData;
              runningText->text = bitRate.length() > 0 ? "Bitrate: " + bitRate : "";
              runningText->scrollOffset = 0;
              lastDisplayedBitRate = bitRate;
            }
          }

          if (id3Data != lastDisplayedID3) {
            UIComponent* txtID3 = findComponentById(radioWindow, TXT_ID3);
            if (txtID3 && txtID3->customData) {
              UIRunningText* runningText = (UIRunningText*)txtID3->customData;
              runningText->text = id3Data.length() > 0 ? "ID3: " + id3Data : "";
              runningText->scrollOffset = 0;
              lastDisplayedID3 = id3Data;
            }
          }

          if (info != lastDisplayedInfo) {
            UIComponent* txtInfo = findComponentById(radioWindow, TXT_INFO);
            if (txtInfo && txtInfo->customData) {
              UIRunningText* runningText = (UIRunningText*)txtInfo->customData;
              runningText->text = info;
              runningText->scrollOffset = 0;
              lastDisplayedInfo = info;
            }
          }

          if (description != lastDisplayedDescription) {
            UIComponent* txtDescription = findComponentById(radioWindow, TXT_DESCRIPTION);
            if (txtDescription && txtDescription->customData) {
              UIRunningText* runningText = (UIRunningText*)txtDescription->customData;
              runningText->text = description;
              runningText->scrollOffset = 0;
              lastDisplayedDescription = description;
            }
          }

          if (lyrics != lastDisplayedLyrics) {
            UIComponent* txtLyrics = findComponentById(radioWindow, TXT_LYRICS);
            if (txtLyrics && txtLyrics->customData) {
              UIRunningText* runningText = (UIRunningText*)txtLyrics->customData;
              runningText->text = lyrics.length() > 0 ? "Lyrics: " + lyrics : "";
              runningText->scrollOffset = 0;
              lastDisplayedLyrics = lyrics;
            }
          }

          if (log != lastDisplayedLog) {
            UIComponent* txtLog = findComponentById(radioWindow, TXT_LOG);
            if (txtLog && txtLog->customData) {
              UIRunningText* runningText = (UIRunningText*)txtLog->customData;
              runningText->text = log.length() > 0 ? "Log: " + log : "";
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
