/*
 * SettingsWindow.cpp - Settings Window Implementation
 *
 * Copyright (c) 2025 felangga
 *
 */

#include "SettingsWindow.h"
#include "ConfigManager.h"
#include "GlobalState.h"
#include "UIHelpers.h"
#include "WindowCallbacks.h"
#include "config.h"
#include "wt32_sc01_plus.h"

void initializeSettingsWindow() {
  extern const int BTN_SETTINGS_SAVE;
  extern const int BTN_SETTINGS_CANCEL;
  extern const int LBL_IDLE_TIMEOUT;
  extern const int CMP_IDLE_TIMEOUT_SLIDER;

  clearChildComponents(settingsWindow);

  // Sleep Timeout Slider (0 to 30 minutes, 0 = disabled)
  // Convert milliseconds to minutes for display
  unsigned long timeoutMs = ConfigManager::getLcdIdleTimeout();
  bool powerSaveEnabled = ConfigManager::getPowerSaveEnabled();
  int timeoutMinutes = powerSaveEnabled ? (timeoutMs / 60000) : 0;

  // Sleep Timeout Label with current value inline
  String labelText = "Sleep Timeout (";
  if (timeoutMinutes == 0) {
    labelText += "Disabled";
  } else {
    labelText += String(timeoutMinutes) + (timeoutMinutes == 1 ? " minute" : " minutes");
  }
  labelText += "):";

  UIComponent* lblIdleTimeout = createLabelComponent(33, 50, 200, 20, LBL_IDLE_TIMEOUT, labelText);
  UILabel* labelData = (UILabel*)lblIdleTimeout->customData;
  labelData->font = FONT_CHICAGO_9PT;
  addChildComponent(settingsWindow, lblIdleTimeout);
  int minMinutes = 0;   // 0 = disabled
  int maxMinutes = 30;  // 30 minutes max

  UIComponent* sliderTimeout = createSliderComponent(20, 70, 300, 40, CMP_IDLE_TIMEOUT_SLIDER,
                                                     minMinutes, maxMinutes, timeoutMinutes);

  sliderTimeout->onValueChanged = [](int componentId, int value) {
    // Update the timeout label and save settings
    extern const int LBL_IDLE_TIMEOUT;
    UIComponent* lblComp = findComponentById(settingsWindow, LBL_IDLE_TIMEOUT);

    String newLabelText = "Sleep Timeout (";
    if (value == 0) {
      // Disabled
      ConfigManager::setPowerSaveEnabled(false);
      ConfigManager::setLcdIdleTimeout(
          DEFAULT_LCD_IDLE_TIMEOUT);  // Keep default for when re-enabled
      newLabelText += "Disabled";
    } else {
      // Enabled with specific timeout
      ConfigManager::setPowerSaveEnabled(true);
      unsigned long timeoutMs = value * 60000UL;
      ConfigManager::setLcdIdleTimeout(timeoutMs);
      newLabelText += String(value) + (value == 1 ? " minute" : " minutes");
    }
    newLabelText += "):";

    if (lblComp && lblComp->customData) {
      UILabel* label = (UILabel*)lblComp->customData;
      label->text = newLabelText;

      // Redraw the label
      lcd.startWrite();
      drawComponent(lcd, *lblComp, settingsWindow.x, settingsWindow.y);
      lcd.endWrite();
    }
  };
  addChildComponent(settingsWindow, sliderTimeout);

  // Save Button
  UIComponent* btnSave = createButtonComponent(160, 130, 80, 30, BTN_SETTINGS_SAVE, "Save");
  btnSave->onClick = [](int componentId) { onSettingsSaveButtonClick(); };
  addChildComponent(settingsWindow, btnSave);

  // Cancel Button
  UIComponent* btnCancel = createButtonComponent(250, 130, 80, 30, BTN_SETTINGS_CANCEL, "Cancel");
  btnCancel->onClick = [](int componentId) { onSettingsCancelButtonClick(); };
  addChildComponent(settingsWindow, btnCancel);
}

// ===== Settings Window Callbacks =====

void onSettingsWindowMinimize() {
  settingsWindow.visible = false;
  settingsWindow.active = false;

  lcd.startWrite();
  // Clear the settings window area
  drawCheckeredPatternArea(lcd, settingsWindow.x, settingsWindow.y, settingsWindow.w + 5,
                           settingsWindow.h + 5);
  // Restore radio window
  radioWindow.visible = true;
  radioWindow.active = true;
  drawWindow(lcd, radioWindow);
  lcd.endWrite();
}

void onSettingsWindowClose() {
  settingsWindow.visible = false;
  settingsWindow.active = false;

  lcd.startWrite();
  // Clear the settings window area
  drawCheckeredPatternArea(lcd, settingsWindow.x, settingsWindow.y, settingsWindow.w + 5,
                           settingsWindow.h + 5);
  // Restore radio window
  radioWindow.visible = true;
  radioWindow.active = true;
  drawWindow(lcd, radioWindow);
  lcd.endWrite();
}

void onSettingsWindowContentClick(int relativeX, int relativeY) {
  handleWindowContentClick(lcd, settingsWindow, relativeX, relativeY);
}

void onSettingsWindowMoved() {
  handleWindowMoved(lcd, settingsWindow);
}

void onSettingsIconClick() {
  // Hide the radio window
  radioWindow.visible = false;
  radioWindow.active = false;

  // Show settings window
  settingsWindow.visible = true;
  settingsWindow.active = true;
  initializeSettingsWindow();

  lcd.startWrite();
  // Clear the area where radio window was
  drawCheckeredPatternArea(lcd, radioWindow.x, radioWindow.y, radioWindow.w + 5, radioWindow.h + 5);
  // Draw settings window
  drawWindow(lcd, settingsWindow);
  lcd.endWrite();
}

void onSettingsSaveButtonClick() {
  // Settings are saved immediately via onChange handlers
  // Just close the window
  showNotification("Settings saved", 2000);
  onSettingsWindowClose();
}

void onSettingsCancelButtonClick() {
  // Reload settings to revert any changes
  ConfigManager::loadSettings();
  onSettingsWindowClose();
}
