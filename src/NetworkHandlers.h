/*
 * NetworkHandlers.h - Network Connection Management
 *
 * Copyright (c) 2025 felangga
 *
 * This header file contains declarations for network functions
 */

#ifndef NETWORK_HANDLERS_H
#define NETWORK_HANDLERS_H

// Network initialization and management
bool connectToWiFi();  // Blocking version (legacy)

// Async WiFi connection
void initWiFiAsync();
bool isWiFiConnecting();
bool isWiFiConnected();

#endif
