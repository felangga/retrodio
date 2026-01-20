/*
 * StationManager.h - Station Management
 *
 * Copyright (c) 2025 felangga
 *
 */

#ifndef STATION_MANAGER_H
#define STATION_MANAGER_H

#include "UI.h"

// Station list management
extern UIListViewItem* stationItems;
extern int stationItemCount;
extern SemaphoreHandle_t stationListMutex;

void reloadStationList();
void switchToStation(int index);
void onStationItemClick(int index, void* itemData);

// Window initialization
void initializeStationWindow();

#endif
