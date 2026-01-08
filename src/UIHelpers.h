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
#include "MacUI.h"

// UI update functions
void updateStationMetadata(const String& stationName, const String& trackInfo);
void updateClock();
void updateCPUUsage();

// Interface drawing
void drawInterface(lgfx::LGFX_Device& lcd);
void redrawWindowContent(lgfx::LGFX_Device& lcd, const MacWindow& window);

// UI task
void uiTask(void* parameter);

#endif
