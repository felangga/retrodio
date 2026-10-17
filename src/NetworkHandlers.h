/*
 * NetworkHandlers.h - Network Connection Management
 *
 * Copyright (c) 2025 felangga
 */

#ifndef NETWORK_HANDLERS_H
#define NETWORK_HANDLERS_H

// Async WiFi connection
void initWiFiAsync();
bool isWiFiConnecting();
bool isWiFiConnected();

// Call from loop(): performs the scheduled reconnect after a dropout.
void handleWiFiReconnect();

// Join the saved SSID/password, honouring the pinned BSSID if one is stored.
void beginSavedWiFi();

#endif
