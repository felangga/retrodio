/*
 * WindowCallbacks.h - Window Event Handlers
 *
 * Copyright (c) 2025 felangga
 *
 * This header file contains declarations for window callback functions
 */

#ifndef WINDOW_CALLBACKS_H
#define WINDOW_CALLBACKS_H

#include "MacUI.h"

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

// Component interaction callbacks
void onComponentClick(int componentId);

// Button callbacks
void onPlay();
void onStop();
void onVolUp();
void onVolDown();
void onPrev();
void onNext();

#endif
