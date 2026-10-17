/*
 * NetworkHandlers.cpp - Network Connection Management Implementation
 *
 * Copyright (c) 2025 felangga
 *
 * WiFi is re-established automatically. The disconnect event only used to flip
 * a flag, so a router reboot or a dropout left the radio offline until someone
 * power-cycled it -- the web remote and OTA both disappear with the link. The
 * event handler now schedules a retry and the main loop performs it, because
 * calling WiFi.begin()/reconnect() from the event task risks racing the driver.
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

// Reconnect bookkeeping. `wifiWantConnect` is set while the radio should be on
// the network; `wifiRetryAt` is the millis() deadline for the next attempt.
static volatile bool wifiWantConnect = false;
static volatile bool wifiRetryPending = false;
static unsigned long wifiRetryAt = 0;
static unsigned long wifiRetryDelayMs = 2000;
static const unsigned long WIFI_RETRY_MAX_MS = 30000;

static void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      wifiConnecting = false;
      wifiConnected = true;
      wifiRetryPending = false;
      wifiRetryDelayMs = 2000;  // reset backoff after a healthy association
      configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      wifiConnecting = false;
      wifiConnected = false;
      if (wifiWantConnect) {
        // Do not reconnect here: this runs in the WiFi event task and racing the
        // driver with begin()/reconnect() from it is unreliable. Hand it to the
        // main loop with a backoff.
        wifiRetryPending = true;
        wifiRetryAt = millis() + wifiRetryDelayMs;
        wifiRetryDelayMs = (wifiRetryDelayMs * 2 > WIFI_RETRY_MAX_MS)
                               ? WIFI_RETRY_MAX_MS
                               : wifiRetryDelayMs * 2;
      }
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
    wifiWantConnect = true;
    wifiRetryPending = false;
    wifiRetryDelayMs = 2000;
  } else {
    wifiConnecting = false;
    wifiConnected = false;
    wifiWantConnect = false;
  }
}

void handleWiFiReconnect() {
  if (!wifiWantConnect || wifiConnected) {
    return;
  }
  if (!wifiRetryPending) {
    return;
  }
  // millis() wraps after ~49 days; compare with subtraction so the deadline
  // still works across the wrap.
  if ((long)(millis() - wifiRetryAt) < 0) {
    return;
  }

  wifiRetryPending = false;
  wifiConnecting = true;
  DEBUG_PRINTLN("WiFi: reconnecting...");
  WiFi.reconnect();
}

bool isWiFiConnecting() {
  return wifiConnecting;
}

bool isWiFiConnected() {
  return wifiConnected && (WiFi.status() == WL_CONNECTED);
}
