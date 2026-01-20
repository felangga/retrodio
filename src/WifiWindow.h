/*
 * WifiWindow.h - WiFi Window Header
 *
 * Copyright (c) 2025 felangga
 *
 * This header file declares WiFi window functions
 */

#ifndef WIFI_WINDOW_H
#define WIFI_WINDOW_H

#include "UI.h"

// Initialize WiFi window components
void initializeWifiWindow();

// Scan for available WiFi networks and populate the list
void scanWifiNetworks();

// Connect to selected WiFi network
void connectToSelectedWifi();
void connectToSelectedWifi(const String& password);

// Handle WiFi password entry
void showWifiPasswordEntry();
void hideWifiPasswordEntry();

// Update WiFi list display
void updateWifiListDisplay();

// Check and update WiFi connection status (call from UI task)
void updateWifiConnectionStatus();

// Cancel ongoing connection attempt
void cancelWifiConnection();

// Check if currently trying to connect
bool isWifiConnecting();

#endif
