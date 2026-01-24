/*
 * WindowCallbacks.h - Window Event Handlers
 *
 * Copyright (c) 2025 felangga
 *
 */

#ifndef WINDOW_CALLBACKS_H
#define WINDOW_CALLBACKS_H

#include "UI.h"

// Radio window callbacks
void onWindowMinimize();
void onWindowClose();
void onRadioIconClick();
void onWindowContentClick(int relativeX, int relativeY);
void onWindowMoved();

// Station window callbacks
void onStationWindowMinimize();
void onStationWindowClose();
void onStationWindowContentClick(int relativeX, int relativeY);
void onStationWindowMoved();

// Add Station window callbacks
void onAddStationWindowMinimize();
void onAddStationWindowClose();
void onAddStationWindowContentClick(int relativeX, int relativeY);
void onAddStationWindowMoved();

// Confirm Delete window callbacks
void onConfirmDeleteWindowMinimize();
void onConfirmDeleteWindowClose();
void onConfirmDeleteWindowContentClick(int relativeX, int relativeY);
void onConfirmDeleteWindowMoved();

// Settings window callbacks
void onSettingsWindowMinimize();
void onSettingsWindowClose();
void onSettingsWindowContentClick(int relativeX, int relativeY);
void onSettingsWindowMoved();
void onSettingsIconClick();
void onSettingsSaveButtonClick();
void onSettingsCancelButtonClick();

// WiFi window callbacks
void onWifiWindowMinimize();
void onWifiWindowClose();
void onWifiWindowContentClick(int relativeX, int relativeY);
void onWifiWindowMoved();
void onWifiSignalClick();

// Station window button callbacks
void onStationButtonClick();
void onAddStationButtonClick();
void onEditStationButtonClick();
void onSaveStationButtonClick();
void onCancelAddStationButtonClick();
void onDeleteStationButtonClick();
void onConfirmYesButtonClick();
void onConfirmNoButtonClick();

#endif
