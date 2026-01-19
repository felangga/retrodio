/*
 * OTAHandler.cpp - Over-The-Air Update Handler Implementation
 *
 * Copyright (c) 2025 felangga
 *
 * This file implements OTA update functionality for Arduino IDE uploads
 */

#include "OTAHandler.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFi.h>

#define ENABLE_OTA_DEBUG 1

#if ENABLE_OTA_DEBUG
#define OTA_DEBUG_PRINT(x) Serial.print(x)
#define OTA_DEBUG_PRINTLN(x) Serial.println(x)
#define OTA_DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define OTA_DEBUG_PRINT(x)
#define OTA_DEBUG_PRINTLN(x)
#define OTA_DEBUG_PRINTF(...)
#endif

void setupOTA() {
  ArduinoOTA.setHostname("retrodio");

  // Set OTA password (optional but recommended for security)
  // ArduinoOTA.setPassword("admin"); // Uncomment and set your password

  // Set OTA port (default is 3232)
  // ArduinoOTA.setPort(3232);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_SPIFFS
      type = "filesystem";
    }
    OTA_DEBUG_PRINTLN("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() { OTA_DEBUG_PRINTLN("\nOTA Update Complete!"); });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    OTA_DEBUG_PRINTF("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    OTA_DEBUG_PRINTF("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      OTA_DEBUG_PRINTLN("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      OTA_DEBUG_PRINTLN("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      OTA_DEBUG_PRINTLN("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      OTA_DEBUG_PRINTLN("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      OTA_DEBUG_PRINTLN("End Failed");
    }
  });

  ArduinoOTA.begin();

  OTA_DEBUG_PRINTLN("OTA Ready");
  OTA_DEBUG_PRINT("IP address: ");
  OTA_DEBUG_PRINTLN(WiFi.localIP());
  OTA_DEBUG_PRINTLN("Hostname: retrodio");
}

void handleOTA() {
  if (WiFi.isConnected()) {
    ArduinoOTA.handle();
  }
}