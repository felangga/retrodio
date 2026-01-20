/*
 * UIHelpers.h - UI Helper Functions
 *
 * Copyright (c) 2025 felangga
 *
 * This header file contains declarations for UI helper functions
 */

#ifndef UI_HELPERS_H
#define UI_HELPERS_H

#include <Arduino.h>
#include "UI.h"

// UI update functions
void updateStationMetadata(const String& stationName, const String& trackInfo);
void updateClock();
void updateWifiSignal();
void updateVolumeDisplay();

// General notification bar (top-right corner)
void showNotification(const String& message, unsigned long duration = 0);
void hideNotification();
void updateNotification();

// Interface drawing
void drawInterface(lgfx::LGFX_Device& lcd);
void redrawWindowContent(lgfx::LGFX_Device& lcd, const UIWindow& window);

// Keyboard interaction handling
void handleKeyboardInteraction();
void adjustWindowForKeyboard(UIWindow& window, UIComponent* inputComponent, bool show);

// WiFi keyboard interaction handling
void handleWifiKeyboardInteraction();

// Menu bar touch detection
void checkMenuBarTouch();

// UI task
void uiTask(void* parameter);

// Helper function to get all visible windows
UIWindow** getVisibleWindows(int& windowCount);

#endif
