/*
 * NetworkHandlers.cpp - Network Connection Management Implementation
 *
 * Copyright (c) 2025 felangga
 *
 * This file implements network system functions
 */

#include "NetworkHandlers.h"
#include "config.h"
#include <WiFi.h>
#include <time.h>

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

bool connectToWiFi() {
  DEBUG_PRINTF("Attempting WiFi connection to SSID: %s\n", WIFI_SSID);

  WiFi.mode(WIFI_STA);
  delay(100);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    DEBUG_PRINTF("WiFi attempt %d, status: %d\n", attempts + 1, WiFi.status());
    delay(1000);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);
    DEBUG_PRINTLN("WiFi connected successfully!");
    DEBUG_PRINTF("IP Address: %s\n", WiFi.localIP().toString().c_str());
    DEBUG_PRINTF("Signal strength: %d dBm\n", WiFi.RSSI());
    return true;
  } else {
    DEBUG_PRINTLN("ERROR: WiFi connection failed!");
    DEBUG_PRINTF("Final status: %d\n", WiFi.status());
    return false;
  }
}
