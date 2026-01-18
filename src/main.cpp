/*
 * Radio.ino - Internet Radio Player with Classic Mac OS UI
 *
 * Copyright (c) 2025 felangga
 *
 * This Arduino sketch implements an internet radio player with a classic
 * Macintosh OS-style user interface using the MacUI library.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "Audio.h"
#include "ConfigManager.h"
#include "MacUI.h"
#include "config.h"
#include "esp32-hal-psram.h"
#include "esp_heap_caps.h"
#include "esp_task_wdt.h"
#include "wt32_sc01_plus.h"

#include "AddStationWindow.h"
#include "AudioHandlers.h"
#include "ConfirmDeleteWindow.h"
#include "GlobalState.h"
#include "NetworkHandlers.h"
#include "OTAHandler.h"
#include "RadioWindow.h"
#include "StationManager.h"
#include "UIHelpers.h"
#include "WifiWindow.h"
#include "WindowCallbacks.h"

// ===== DEBUG CONFIGURATION =====
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

// ===== COMPONENT ID CONSTANTS =====

const int CMP_NOW_PLAYING_LABEL = 100;
const int CMP_VOLUME_SLIDER = 101;
const int CMP_BUFFER_PROGRESS = 102;
const int CMP_AUTO_PLAY_CHECKBOX = 103;
const int CMP_VISUALS_CHECKBOX = 104;

// Radio Window Text Components
const int TXT_RADIO_NAME = 200;
const int TXT_RADIO_DETAILS = 201;
const int TXT_CPU_LABEL = 202;
const int TXT_BITRATE = 203;
const int TXT_ID3 = 204;
const int TXT_INFO = 205;
const int TXT_DESCRIPTION = 206;
const int TXT_LYRICS = 207;
const int TXT_LOG = 208;

// Add Station Window Components
const int LBL_STATION_NAME = 400;
const int INPUT_STATION_NAME = 401;
const int LBL_STATION_URL = 402;
const int INPUT_STATION_URL = 403;
const int KEYBOARD_COMPONENT = 404;

const int BTN_PLAY = 1;
const int BTN_STOP = 2;
const int BTN_VOL_UP = 3;
const int BTN_PREV = 4;
const int BTN_NEXT = 5;
const int BTN_VOL_DOWN = 6;
const int BTN_STATION = 7;
const int BTN_ADD_STATION = 8;
const int BTN_EDIT_STATION = 9;
const int BTN_SAVE_STATION = 10;
const int BTN_CANCEL_ADD_STATION = 11;
const int BTN_DELETE_STATION = 12;
const int BTN_CONFIRM_YES = 13;
const int BTN_CONFIRM_NO = 14;

// WiFi Window Components
const int WIFI_LIST_COMPONENT = 500;
const int BTN_WIFI_CONNECT = 501;
const int BTN_WIFI_CANCEL = 502;
const int BTN_WIFI_REFRESH = 503;
const int LBL_WIFI_TITLE = 504;
const int INPUT_WIFI_PASSWORD = 505;
const int LBL_WIFI_PASSWORD = 506;
const int BTN_WIFI_PASSWORD_OK = 507;
const int BTN_WIFI_PASSWORD_CANCEL = 508;
const int WIFI_KEYBOARD_COMPONENT = 509;

// ===== GLOBAL OBJECTS =====

LGFX lcd;
Audio audio;

TaskHandle_t uiTaskHandle = NULL;
TaskHandle_t audioTaskHandle = NULL;
SemaphoreHandle_t metadataMutex = NULL;
QueueHandle_t audioCommandQueue = NULL;

unsigned long lastClockUpdate = 0;
String lastClockText;

unsigned long lastCPUUpdate = 0;
float cpuUsage0 = 0.0;
float cpuUsage1 = 0.0;

volatile bool isPlaying = false;
String currentStationName = "Retrodio";
String RadioURL = "";
int currentStationIndex = -1;

StreamMetadata streamMetadata = {"", "", "", "", "", "", "", "", false};
String lastTrackInfo = "";
String lastDisplayedBitRate = "";
String lastDisplayedID3 = "";
String lastDisplayedInfo = "";
String lastDisplayedDescription = "";
String lastDisplayedLyrics = "";
String lastDisplayedLog = "";

MacWindow radioWindow{30,
                      40,
                      420,
                      240,
                      "Radio",
                      true,
                      false,
                      true,
                      nullptr,
                      onWindowClose,
                      onWindowContentClick,
                      onWindowMoved,
                      nullptr,
                      0,
                      false,
                      0,
                      0};

MacWindow stationWindow{20,
                        40,
                        420,
                        240,
                        "Station List",
                        false,
                        false,
                        false,
                        onStationWindowMinimize,
                        onStationWindowClose,
                        onStationWindowContentClick,
                        onStationWindowMoved,
                        nullptr,
                        0,
                        false,
                        0,
                        0};

MacWindow addStationWindow{60,
                           40,
                           360,
                           160,
                           "Add Station",
                           false,
                           false,
                           false,
                           onAddStationWindowMinimize,
                           onAddStationWindowClose,
                           onAddStationWindowContentClick,
                           onAddStationWindowMoved,
                           nullptr,
                           0,
                           false,
                           0,
                           0};

MacWindow confirmDeleteWindow{100,
                              100,
                              280,
                              120,
                              "Confirm Delete",
                              false,
                              false,
                              false,
                              nullptr,
                              onConfirmDeleteWindowClose,
                              onConfirmDeleteWindowContentClick,
                              onConfirmDeleteWindowMoved,
                              nullptr,
                              0,
                              false,
                              0,
                              0};

MacWindow wifiWindow{80,
                     40,
                     300,
                     225,
                     "WiFi Networks",
                     false,
                     false,
                     false,
                     onWifiWindowMinimize,
                     onWifiWindowClose,
                     onWifiWindowContentClick,
                     onWifiWindowMoved,
                     nullptr,
                     0,
                     false,
                     0,
                     0};

DesktopIcon radioIcon{50, 60, "Radio", "window", false, false, &radioWindow, onRadioIconClick};

MacComponent* globalKeyboard = nullptr;
MacComponent* wifiKeyboard = nullptr;
int stationToDeleteIndex = -1;  // Track which station to delete
bool isEditMode = false;        // Track if we're in edit mode
int stationToEditIndex = -1;    // Track which station to edit

// ===== HELPER FUNCTIONS =====

MacComponent* findComponentById(const MacWindow& window, int id) {
  if (window.childComponents == nullptr || window.childComponentCount == 0) {
    return nullptr;
  }

  for (int i = 0; i < window.childComponentCount; i++) {
    MacComponent* component = window.childComponents[i];
    if (component != nullptr && component->id == id) {
      return component;
    }
  }

  return nullptr;
}

void updateComponentSymbol(const MacWindow& window, int componentId, SymbolType newSymbol) {
  MacComponent* component = findComponentById(window, componentId);
  if (component != nullptr && component->type == COMPONENT_BUTTON &&
      component->customData != nullptr) {
    MacButton* btnData = (MacButton*)component->customData;
    btnData->symbol = newSymbol;
    drawComponent(lcd, *component, window.x, window.y);
  }
}

// ===== MAIN FUNCTIONS =====

void setup() {
#if ENABLE_SERIAL_DEBUG
#if ARDUINO_USB_MODE
  delay(100);
#else
  Serial.begin(115200);
  delay(100);
#endif
#endif

  metadataMutex = xSemaphoreCreateMutex();
  audioCommandQueue = xQueueCreate(5, sizeof(AudioCommandMsg));

  lcd.init();
  tft.initDMA();
  tft.startWrite();
  lcd.setRotation(lcd.getRotation() ^ 1);
  initComponentBuffer(&lcd, 420, 50);
  lcd.fillScreen(MAC_WHITE);
  drawInterface(lcd);

  registerWindow(&radioWindow);
  registerWindow(&stationWindow);
  registerWindow(&addStationWindow);
  registerWindow(&confirmDeleteWindow);
  registerWindow(&wifiWindow);

  if (!ConfigManager::begin()) {
    DEBUG_PRINTLN("ERROR: ConfigManager initialization failed!");
  } else {
    DEBUG_PRINTLN("ConfigManager initialized successfully");
    reloadStationList();
    initializeStationWindow();
  }

  delay(100);

  DEBUG_PRINTLN("\n=== Starting WiFi (async) ===");
  showNotification("Connecting to WiFi...");
  initWiFiAsync();

  // Wait for WiFi connection before proceeding
  // while (isWiFiConnecting()) {
  //   vTaskDelay(pdMS_TO_TICKS(100));
  // }

  if (isWiFiConnected()) {
    DEBUG_PRINTLN("WiFi connected!");
    showNotification("WiFi Connected!", 2000);  // Show for 2 seconds
  } else {
    DEBUG_PRINTLN("WiFi connection failed!");
    showNotification("WiFi Failed!", 3000);  // Show for 3 seconds
  }

  // Initialize OTA after WiFi connection
  DEBUG_PRINTLN("=== Setting up OTA ===");
  setupOTA();

  initializeAudio();

  audio.setVolume(ConfigManager::getVolume());
  audio.setAudioTaskCore(0);

  LastStation lastStation = ConfigManager::getLastStation();
  if (lastStation.name.length() > 0 && lastStation.url.length() > 0) {
    currentStationName = lastStation.name;
    RadioURL = lastStation.url;

    int stationCount = ConfigManager::getStationCount();
    for (int i = 0; i < stationCount; i++) {
      Station station = ConfigManager::getStation(i);
      if (station.name == lastStation.name) {
        currentStationIndex = i;
        break;
      }
    }
  }

  xTaskCreatePinnedToCore(uiTask, "UI_Task", 8192, NULL, 1, &uiTaskHandle, 1);
  xTaskCreatePinnedToCore(audioTask, "Audio_Task", 16384, NULL, 2, &audioTaskHandle, 0);

  if (RadioURL.length() > 0 && isWiFiConnected()) {
    delay(1000);
    AudioCommandMsg msg = {CMD_CONNECT, ""};
    strncpy(msg.url, RadioURL.c_str(), sizeof(msg.url) - 1);
    msg.url[sizeof(msg.url) - 1] = '\0';

    if (xQueueSend(audioCommandQueue, &msg, pdMS_TO_TICKS(500)) == pdTRUE) {
      isPlaying = true;
      updateComponentSymbol(radioWindow, 1, SYMBOL_PAUSE);
    }
  }
}

void loop() {
  handleOTA();
  vTaskDelay(10);
}
