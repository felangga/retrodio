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

// Consecutive failed associations before a pinned BSSID is abandoned.
static const int WIFI_PIN_MAX_FAILURES = 5;
static volatile int wifiPinFailures = 0;

// Set from the event task, drained in loop(): the flash write must not happen
// inside the WiFi callback.
static volatile bool wifiPinSavePending = false;
static char wifiPinCandidate[18] = {0};


// "aa:bb:cc:dd:ee:ff" -> 6 bytes. Returns false on anything malformed so a
// corrupt setting cannot stop the radio from joining the network.
static bool parseBssid(const String& text, uint8_t out[6]) {
  if (text.length() != 17) return false;
  for (int i = 0; i < 6; i++) {
    int hi = text[i * 3];
    int lo = text[i * 3 + 1];
    if (i < 5 && text[i * 3 + 2] != ':') return false;
    auto nib = [](int c) -> int {
      if (c >= '0' && c <= '9') return c - '0';
      if (c >= 'a' && c <= 'f') return c - 'a' + 10;
      if (c >= 'A' && c <= 'F') return c - 'A' + 10;
      return -1;
    };
    int h = nib(hi), l = nib(lo);
    if (h < 0 || l < 0) return false;
    out[i] = (uint8_t)((h << 4) | l);
  }
  return true;
}

// Join the saved network. When a BSSID is pinned we pass it, which stops the
// ESP32 from roaming to a same-named AP on another subnet.
void beginSavedWiFi() {
  String ssid = ConfigManager::getWifiSSID();
  String pass = ConfigManager::getWifiPassword();
  uint8_t bssid[6];

  if (ssid.length() == 0) return;

  bool pinned = parseBssid(ConfigManager::getWifiBSSID(), bssid);

  if (pinned) {
    // Channel 0 tells the driver to scan for the BSSID first, so a channel change
    // on the AP does not break the pin. WiFi.channel() is 0 before association,
    // which is exactly the state we are in here.
    WiFi.begin(ssid.c_str(), pass.length() ? pass.c_str() : NULL, 0, bssid, true);
  } else if (pass.length() > 0) {
    WiFi.begin(ssid.c_str(), pass.c_str());
  } else {
    WiFi.begin(ssid.c_str());
  }
}

static void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      wifiConnecting = false;
      wifiConnected = true;

      // Pin to the AP we actually landed on, once. Two APs sharing an SSID on
      // different subnets is common and the radio otherwise roams between them,
      // changing IP and breaking OTA. Only the MAC is read here: saving touches
      // LittleFS, which must not happen in the WiFi event task, so loop()
      // persists it from wifiPinSavePending.
      if (ConfigManager::getWifiBSSID().length() == 0 && !wifiPinSavePending) {
        uint8_t mac[6];
        if (WiFi.BSSID(mac) != nullptr) {
          snprintf(wifiPinCandidate, sizeof(wifiPinCandidate),
                   "%02x:%02x:%02x:%02x:%02x:%02x",
                   mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
          wifiPinSavePending = true;
        }
      }

      wifiRetryPending = false;
      wifiRetryDelayMs = 2000;  // reset backoff after a healthy association
      wifiPinFailures = 0;
      configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      wifiConnecting = false;
      wifiConnected = false;
      if (wifiWantConnect) {
        // A pinned BSSID that no longer exists would lock the radio out of its own
        // network forever. After enough consecutive failures, drop the pin and
        // let the driver pick any AP carrying the SSID.
        wifiPinFailures++;
        if (wifiPinFailures >= WIFI_PIN_MAX_FAILURES &&
            ConfigManager::getWifiBSSID().length() > 0) {
          DEBUG_PRINTLN("WiFi: pin failing, clearing BSSID");
          ConfigManager::setWifiBSSID("");
          wifiPinFailures = 0;
        }

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

  if (savedSSID.length() > 0) {
    beginSavedWiFi();
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
  if (wifiPinSavePending) {
    wifiPinSavePending = false;
    ConfigManager::setWifiBSSID(String(wifiPinCandidate));
    DEBUG_PRINTF("WiFi: pinned to BSSID %s\n", wifiPinCandidate);
  }

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
  // reconnect() reuses the stored BSSID, so a cleared pin takes effect next boot;
  // re-issue begin() instead so clearing it applies immediately.
  if (ConfigManager::getWifiBSSID().length() == 0) {
    beginSavedWiFi();
  } else {
    WiFi.reconnect();
  }
}

bool isWiFiConnecting() {
  return wifiConnecting;
}

bool isWiFiConnected() {
  return wifiConnected && (WiFi.status() == WL_CONNECTED);
}
