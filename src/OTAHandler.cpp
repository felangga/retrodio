/*
 * OTAHandler.cpp - Over-The-Air Update Handler Implementation
 *
 * Copyright (c) 2025 felangga
 *
 * Arming is deferred, not one-shot. ArduinoOTA.begin() registers the mDNS
 * _arduino._tcp service; calling it before the interface has an IP leaves the
 * service unpublished and nothing re-registers it, so the radio becomes
 * unreachable for OTA until the next reboot. The previous code called
 * setupOTA() unconditionally after a fixed 10 s wait, so a slow DHCP lease
 * silently killed OTA for the whole session. ensureOTA() is now driven from
 * loop() and arms exactly once, as soon as WiFi actually has an IP.
 *
 * While an update is running the radio is stopped and the bottom bar reports
 * progress: audio decoding competes with the flash write for CPU and SPI, and a
 * listener staring at a frozen screen has no way to tell an update from a crash.
 */

#include "OTAHandler.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include "Audio.h"
#include "GlobalState.h"
#include "UIHelpers.h"
#include "UI.h"

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

static bool otaArmed = false;
static bool otaUpdating = false;
static unsigned long otaLastBarUpdate = 0;

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

    // Stop playback for the duration: the flash write and the audio decoder
    // fight over CPU and the SPI bus, and a half-written flash is worse than a
    // dropped stream.
    otaUpdating = true;
    extern QueueHandle_t audioCommandQueue;
    if (audioCommandQueue != nullptr) {
      AudioCommandMsg msg = {CMD_STOP, ""};
      xQueueSend(audioCommandQueue, &msg, pdMS_TO_TICKS(200));
    }
    isPlaying = false;

    // Wake the panel if it was asleep so the progress is actually visible.
    wakeLCD();
    showNotification("Updating via OTA...", 0);
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    OTA_DEBUG_PRINTF("Progress: %u%%\r", (progress / (total / 100)));

    // Throttle: the callback fires per chunk and repainting the bar that often
    // would itself slow the transfer down.
    unsigned long now = millis();
    if (now - otaLastBarUpdate < 250) {
      return;
    }
    otaLastBarUpdate = now;

    int pct = total ? (int)((uint64_t)progress * 100 / total) : 0;
    showNotification("OTA " + String(pct) + "%", 0);
  });

  ArduinoOTA.onEnd([]() {
    OTA_DEBUG_PRINTLN("\nOTA Update Complete!");
    otaUpdating = false;
    showNotification("OTA done, rebooting...", 0);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    OTA_DEBUG_PRINTF("Error[%u]: ", error);
    otaUpdating = false;
    if (error == OTA_AUTH_ERROR) {
      OTA_DEBUG_PRINTLN("Auth Failed");
      showNotification("OTA failed: auth", 4000);
    } else if (error == OTA_BEGIN_ERROR) {
      OTA_DEBUG_PRINTLN("Begin Failed");
      showNotification("OTA failed: begin", 4000);
    } else if (error == OTA_CONNECT_ERROR) {
      OTA_DEBUG_PRINTLN("Connect Failed");
      showNotification("OTA failed: connect", 4000);
    } else if (error == OTA_RECEIVE_ERROR) {
      OTA_DEBUG_PRINTLN("Receive Failed");
      showNotification("OTA failed: receive", 4000);
    } else if (error == OTA_END_ERROR) {
      OTA_DEBUG_PRINTLN("End Failed");
      showNotification("OTA failed: end", 4000);
    }
  });

  ArduinoOTA.begin();
  otaArmed = true;

  OTA_DEBUG_PRINTLN("OTA Ready");
  OTA_DEBUG_PRINT("IP address: ");
  OTA_DEBUG_PRINTLN(WiFi.localIP());
  OTA_DEBUG_PRINTLN("Hostname: retrodio");
}

void ensureOTA() {
  // Arm once, and only when the interface really has an IP: begin() before that
  // publishes nothing and is never retried.
  if (!otaArmed && WiFi.isConnected() && WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
    setupOTA();
  }
}

void handleOTA() {
  if (otaArmed && WiFi.isConnected()) {
    ArduinoOTA.handle();
  }
}

bool isOtaUpdating() {
  return otaUpdating;
}
