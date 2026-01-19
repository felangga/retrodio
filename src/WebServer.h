/*
 * WebServer.h - Web-based Remote Control
 *
 * Copyright (c) 2025 felangga
 *
 * Provides a web interface for remote control of the radio
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WebServer.h>

// Web server instance
extern WebServer server;

// Initialize and start web server
void initWebServer();

// Handle client requests
void handleWebServer();

// Web server handlers
void handleRoot();
void handleGetStatus();
void handleSetVolume();
void handleSelectStation();
void handlePlayPause();
void handleNext();
void handlePrevious();
void handleGetStations();
void handleAddStation();
void handleEditStation();
void handleDeleteStation();
void handleNotFound();

#endif // WEB_SERVER_H
