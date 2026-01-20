/*
 * NetworkHandlers.cpp - Network Connection Management Implementation
 *
 * Copyright (c) 2025 felangga
 *
 */

#include "NetworkHandlers.h"
#include <WiFi.h>
#include <time.h>
#include "ConfigManager.h"
#include "config.h"

#define ENABLE_SERIAL_DEBUG 0
#define ENABLE_DEBUG 0

#if ENABLE_SERIAL_DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#endif

// Async WiFi state
static volatile bool wifiConnecting = false;
static volatile bool wifiConnected = false;

static void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      wifiConnecting = false;
      wifiConnected = true;
      configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      wifiConnecting = false;
      wifiConnected = false;
      break;
    default:
      break;
  }
}

void initWiFiAsync() {
  WiFi.onEvent(onWiFiEvent);
  WiFi.mode(WIFI_STA);
  delay(100);

  String savedSSID = ConfigManager::getWifiSSID();
  String savedPassword = ConfigManager::getWifiPassword();

  if (savedSSID.length() > 0) {
    if (savedPassword.length() > 0) {
      WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    } else {
      WiFi.begin(savedSSID.c_str());
    }
    wifiConnecting = true;
    wifiConnected = false;
  } else {
    wifiConnecting = false;
    wifiConnected = false;
  }
}

bool isWiFiConnecting() {
  return wifiConnecting;
}

bool isWiFiConnected() {
  return wifiConnected && (WiFi.status() == WL_CONNECTED);
}
