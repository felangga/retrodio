/*
 * StationManager.h - Station Management
 *
 * Copyright (c) 2025 felangga
 *
 * This header file contains declarations for station management functions
 */

#ifndef STATION_MANAGER_H
#define STATION_MANAGER_H

#include "MacUI.h"

// Station list management
extern MacListViewItem* stationItems;
extern int stationItemCount;

void reloadStationList();
void switchToStation(int index);
void onStationItemClick(int index, void* itemData);

// Window initialization
void initializeRadioWindow();
void initializeStationWindow();
void initializeAddStationWindow();

#endif
