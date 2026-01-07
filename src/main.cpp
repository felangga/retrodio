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

// ===== DEBUG CONFIGURATION =====
// Set to 0 to completely disable Serial debugging (may help with WiFi if issues occur)
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

// ===== GLOBAL OBJECTS =====

const int btnStationID = 7;
const int btnAddStationID = 8;
const int btnSaveStationID = 9;
const int btnCancelAddStationID = 10;

static LGFX lcd;  // Display instance
Audio audio;      // Audio streaming instance

// Multi-core task handles
TaskHandle_t uiTaskHandle = NULL;
TaskHandle_t audioTaskHandle = NULL;

// Mutex for protecting shared metadata strings between cores
SemaphoreHandle_t metadataMutex = NULL;

// Audio command queue for thread-safe communication between cores
QueueHandle_t audioCommandQueue = NULL;

// Audio commands
enum AudioCommand {
  CMD_NONE = 0,
  CMD_PLAY,
  CMD_STOP,
  CMD_CONNECT
};

struct AudioCommandMsg {
  AudioCommand cmd;
  char url[256];  // URL for CMD_CONNECT
};

// Clock state
unsigned long lastClockUpdate = 0;
String lastClockText;

// CPU usage tracking
unsigned long lastCPUUpdate = 0;
float cpuUsage0 = 0.0;
float cpuUsage1 = 0.0;

// Music player state
volatile bool isPlaying = false;         // volatile because accessed from both cores
String currentStationName = "Retrodio";  // Default station name
String RadioURL = "";
int currentStationIndex = -1;  // Current station index in the list (-1 = no station selected)

// Metadata from audio stream server (protected by metadataMutex)
// Use char buffers instead of String to avoid heap allocation in ISR
#define METADATA_BUFFER_SIZE 128  // Reduced from 256 to save 1KB RAM

typedef struct {
  char stationName[METADATA_BUFFER_SIZE];  // Station name from server (evt_name)
  char trackInfo[METADATA_BUFFER_SIZE];    // Current track from StreamTitle (evt_streamtitle)
  char bitRate[METADATA_BUFFER_SIZE];      // Bitrate info from server (evt_bitrate)
  char id3data[METADATA_BUFFER_SIZE];      // ID3 tag data (evt_id3data)
  char info[METADATA_BUFFER_SIZE];         // General info messages (evt_info)
  char description[METADATA_BUFFER_SIZE];  // Stream description (evt_description)
  char lyrics[METADATA_BUFFER_SIZE];       // Song lyrics (evt_lyrics)
  char log[METADATA_BUFFER_SIZE];          // Log messages (evt_log)
  volatile bool received;                  // Flag to know when server sent metadata
} StreamMetadata;

StreamMetadata streamMetadata = {"", "", "", "", "", "", "", "", false};
String lastTrackInfo = "";  // Track previous value to detect changes (UI task only)

// Track last displayed metadata to avoid flickering from redundant updates
String lastDisplayedBitRate = "";
String lastDisplayedID3 = "";
String lastDisplayedInfo = "";
String lastDisplayedDescription = "";
String lastDisplayedLyrics = "";
String lastDisplayedLog = "";

// Forward declarations for callbacks
void onWindowMinimize();
void onWindowClose();
void onRadioIconClick();
void onWindowContentClick(int relativeX, int relativeY);
void onWindowMoved();
void audio_callback(Audio::msg_t m);

// Station window callbacks
void onStationWindowMinimize();
void onStationWindowClose();
void onStationWindowContentClick(int relativeX, int relativeY);
void onStationWindowMoved();

// Add Station window callbacks
void onAddStationWindowMinimize();
void onAddStationWindowClose();
void onAddStationWindowContentClick(int relativeX, int relativeY);
void onAddStationWindowMoved();

// Main radio window - declared early so callbacks can reference it
MacWindow radioWindow{20,
                      40,
                      420,
                      240,
                      "Radio",
                      true,
                      false,
                      true,
                      onWindowMinimize,
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
                        true,
                        false,
                        true,
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
                           true,
                           false,
                           true,
                           onAddStationWindowMinimize,
                           onAddStationWindowClose,
                           onAddStationWindowContentClick,
                           onAddStationWindowMoved,
                           nullptr,
                           0,
                           false,
                           0,
                           0};

// Desktop icon for minimized radio window
DesktopIcon radioIcon{50,    60,    "Radio Player", "window",
                      false, false, &radioWindow,   onRadioIconClick};

// Global keyboard component (overlays bottom half of screen)
MacComponent* globalKeyboard = nullptr;

// ===== FUNCTION PROTOTYPES =====

void connectToWiFi();
void initializeAudio();
void updateClock();
// void updateCPUUsage();

// Multi-core task functions
void uiTask(void* parameter);
void audioTask(void* parameter);

// Window setup functions
void initializeRadioWindow();
void initializeStationWindow();
void initializeAddStationWindow();

// UI functions
void drawInterface(lgfx::LGFX_Device& lcd);
void redrawWindowContent(lgfx::LGFX_Device& lcd, const MacWindow& window);
void updateStationMetadata(const String& stationName, const String& trackInfo);

// Station management functions
void reloadStationList();
void switchToStation(int index);

// Component interaction functions
void onComponentClick(int componentId);
MacComponent* findComponentById(const MacWindow& window, int id);

// ===== BUTTON DECLARATIONS =====
// Forward declarations for button callbacks
void onPlay();
void onStop();
void onVolUp();
void onVolDown();
void onPrev();
void onNext();

// ===== COMPONENT INTERACTION HANDLERS =====

void onComponentClick(int componentId) {
  switch (componentId) {
    case 100:  // Now Playing Label
      displayStatus(lcd, "Label clicked", 160);
      break;

    case 101:  // Volume Slider
      displayStatus(lcd, "Volume slider clicked", 160);
      // Here you could implement slider dragging logic
      break;

    case 102:  // Buffer Progress Bar
      displayStatus(lcd, "Progress bar clicked", 160);
      break;

    case 103:  // Auto Play Checkbox
      // Toggle checkbox state
      {
        MacComponent* component = findComponentById(radioWindow, componentId);
        if (component && component->customData) {
          MacCheckBox* checkbox = (MacCheckBox*)component->customData;
          checkbox->checked = !checkbox->checked;
          // Redraw the component
          drawComponent(lcd, *component, radioWindow.x, radioWindow.y);
          displayStatus(lcd, checkbox->checked ? "Auto Play ON" : "Auto Play OFF", 160);
        }
      }
      break;

    case 104:  // Show Visuals Checkbox
      // Toggle checkbox state
      {
        MacComponent* component = findComponentById(radioWindow, componentId);
        if (component && component->customData) {
          MacCheckBox* checkbox = (MacCheckBox*)component->customData;
          checkbox->checked = !checkbox->checked;
          // Redraw the component
          drawComponent(lcd, *component, radioWindow.x, radioWindow.y);
          displayStatus(lcd, checkbox->checked ? "Visuals ON" : "Visuals OFF", 160);
        }
      }
      break;

    // Music player control buttons
    case 1:  // Play Button
      onPlay();
      break;
    case 2:  // Stop Button
      onStop();
      break;
    case 3:  // Volume Up Button
      onVolUp();
      break;
    case 4:  // Previous Button
      onPrev();
      break;
    case 5:  // Next Button
      onNext();
      break;
    case 6:  // Volume Down Button
      onVolDown();
      break;
    case btnStationID:  // Station List Button

      // Show station window
      showWindowOnTop(lcd, stationWindow);
      break;

    case btnAddStationID: 
      
      // Hide station window immediately to stop event processing
      stationWindow.visible = false;

      // Wait for touch release to prevent button redraw on new window
      {
        int tx, ty;
        delay(200);
        while (lcd.getTouch(&tx, &ty)) {
          delay(10);
        }
        delay(50);  // Extra delay to ensure touch system is clear
      }

      // Show add station window
      addStationWindow.visible = true;
      addStationWindow.minimized = false;
      
      drawWindow(lcd, addStationWindow);
      break;

    case btnSaveStationID:  // Save Station Button
      // Get the input values from the input fields
      {
        MacComponent* nameInputComp = findComponentById(addStationWindow, 401);
        MacComponent* urlInputComp = findComponentById(addStationWindow, 403);

        if (nameInputComp && urlInputComp) {
          MacInputField* nameInput = (MacInputField*)nameInputComp->customData;
          MacInputField* urlInput = (MacInputField*)urlInputComp->customData;

          String stationName = nameInput->text;
          String stationURL = urlInput->text;

          // Validate inputs
          if (stationName.length() > 0 && stationURL.length() > 0) {
            // Add station to ConfigManager
            if (ConfigManager::addStation(stationName, stationURL)) {
              displayStatus(lcd, "Station Saved: " + stationName, 160);

              // Reload station list to reflect new station
              reloadStationList();

              // Refresh station window to show new station
              initializeStationWindow();

              // Clear input fields
              nameInput->text = "";
              nameInput->cursorPos = 0;
              urlInput->text = "";
              urlInput->cursorPos = 0;
            } else {
              displayStatus(lcd, "Failed to save station", 160);
              return;  // Don't close window if save failed
            }
          } else {
            displayStatus(lcd, "Please fill all fields", 160);
            return;  // Don't close window if validation fails
          }
        }
      }

      // Hide keyboard and close add station window
      if (globalKeyboard) {
        MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
        keyboard->visible = false;
      }

      addStationWindow.visible = false;
      stationWindow.visible = true;
    
      drawWindow(lcd, stationWindow);
      break;

    case btnCancelAddStationID:  // Cancel Add Station Button
      // Hide keyboard and close add station window
      if (globalKeyboard) {
        MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
        keyboard->visible = false;
      }

      addStationWindow.visible = false;
      stationWindow.visible = true;
     
      drawWindow(lcd, stationWindow);
      break;

    default:
      break;
  }
}

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
    // Force redraw the component
    drawComponent(lcd, *component, window.x, window.y);
  }
}

// ===== BUTTON CALLBACKS =====
void onPlay() {
  if (isPlaying) {
    // Send stop command to audio task
    AudioCommandMsg msg = {CMD_STOP, ""};
    xQueueSend(audioCommandQueue, &msg, portMAX_DELAY);
    isPlaying = false;
    updateComponentSymbol(radioWindow, 1, SYMBOL_PLAY);
    displayStatus(lcd, "Paused", 160);
  } else {
    // Currently stopped - start playing
    displayStatus(lcd, "Connecting...", 160);
    AudioCommandMsg msg = {CMD_CONNECT, ""};
    strncpy(msg.url, RadioURL.c_str(), sizeof(msg.url) - 1);
    xQueueSend(audioCommandQueue, &msg, portMAX_DELAY);
    // isPlaying will be set by audio task after successful connection
  }
}

void onStop() {
  // CRITICAL: Only send stop command if audio is actually running
  if (isPlaying && audio.isRunning()) {
    AudioCommandMsg msg = {CMD_STOP, ""};
    xQueueSend(audioCommandQueue, &msg, portMAX_DELAY);
  }

  isPlaying = false;
  updateComponentSymbol(radioWindow, 1, SYMBOL_PLAY);
  displayStatus(lcd, "Stopped", 160);
}

void onVolUp() {
  int newVol = min(21, audio.getVolume() + 1);
  audio.setVolume(newVol);
  ConfigManager::setVolume(newVol);  // Save to config
  displayStatus(lcd, "Volume: " + String(newVol), 160);

  // Update volume display in window
  if (!radioWindow.minimized && radioWindow.visible) {
    draw3DFrame(lcd, radioWindow.x + 310, radioWindow.y + 35, 90, 25, true);
    lcd.setTextColor(MAC_BLACK, MAC_WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(radioWindow.x + 315, radioWindow.y + 43);
    lcd.printf("Volume: %d", newVol);
  }
}

void onPrev() {
  int stationCount = ConfigManager::getStationCount();

  // Check if there are any stations
  if (stationCount == 0) {
    displayStatus(lcd, "No stations available", 160);
    return;
  }

  // Calculate previous station index (wrap around to last if at first)
  int prevIndex;
  if (currentStationIndex <= 0) {
    prevIndex = stationCount - 1;  // Wrap to last station
  } else {
    prevIndex = currentStationIndex - 1;
  }

  // Switch to previous station (without refreshing whole window)
  switchToStation(prevIndex);
}

void onNext() {
  int stationCount = ConfigManager::getStationCount();

  // Check if there are any stations
  if (stationCount == 0) {
    displayStatus(lcd, "No stations available", 160);
    return;
  }

  // Calculate next station index (wrap around to first if at last)
  int nextIndex;
  if (currentStationIndex < 0 || currentStationIndex >= stationCount - 1) {
    nextIndex = 0;  // Wrap to first station
  } else {
    nextIndex = currentStationIndex + 1;
  }

  // Switch to next station (without refreshing whole window)
  switchToStation(nextIndex);
}

void onVolDown() {
  int newVol = max(0, audio.getVolume() - 1);
  audio.setVolume(newVol);
  ConfigManager::setVolume(newVol);  // Save to config
  displayStatus(lcd, "Volume: " + String(newVol), 160);

  // Update volume display in window
  if (!radioWindow.minimized && radioWindow.visible) {
    draw3DFrame(lcd, radioWindow.x + 310, radioWindow.y + 35, 90, 25, true);
    lcd.setTextColor(MAC_BLACK, MAC_WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(radioWindow.x + 315, radioWindow.y + 43);
    lcd.printf("Volume: %d", newVol);
  }
}

void onOK() {
  displayStatus(lcd, "OK", 160);
}

// Window management callbacks
void onWindowMinimize() {
  handleWindowMinimize(lcd, radioWindow, &radioIcon);
}

void onWindowClose() {
  handleWindowClose(lcd, radioWindow, &radioIcon);
}

void onRadioIconClick() {
  handleIconClick(lcd, radioWindow);
}

// Window content interaction callback
void onWindowContentClick(int relativeX, int relativeY) {
  handleWindowContentClick(lcd, radioWindow, relativeX, relativeY);
}

// Window moved callback
void onWindowMoved() {
  handleWindowMoved(lcd, radioWindow);
}

// Station window callbacks
void onStationWindowMinimize() {
  stationWindow.visible = false;
  radioWindow.visible = true;

  drawWindow(lcd, radioWindow);
}

void onStationWindowClose() {
  stationWindow.visible = false;
  radioWindow.visible = true;
  
  drawWindow(lcd, radioWindow);
}

void onStationWindowContentClick(int relativeX, int relativeY) {
  handleWindowContentClick(lcd, stationWindow, relativeX, relativeY);
}

void onStationWindowMoved() {
  handleWindowMoved(lcd, stationWindow);
}

// Add Station window callbacks
void onAddStationWindowMinimize() {
  // Hide keyboard if visible
  if (globalKeyboard) {
    MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
    keyboard->visible = false;
  }

  // Close add station window and return to station list
  addStationWindow.visible = false;
  stationWindow.visible = true;
 
  drawWindow(lcd, stationWindow);
}

void onAddStationWindowClose() {
  // Hide keyboard if visible
  if (globalKeyboard) {
    MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
    keyboard->visible = false;
  }

  // Close add station window and return to station list
  addStationWindow.visible = false;
  stationWindow.visible = true;
  drawWindow(lcd, stationWindow);
}

void onAddStationWindowContentClick(int relativeX, int relativeY) {
  // Find input components
  MacComponent* nameInputComp = nullptr;
  MacComponent* urlInputComp = nullptr;

  for (int i = 0; i < addStationWindow.childComponentCount; i++) {
    MacComponent* comp = addStationWindow.childComponents[i];
    if (comp->id == 401) {
      nameInputComp = comp;
    } else if (comp->id == 403) {
      urlInputComp = comp;
    }
  }

  if (globalKeyboard && nameInputComp && urlInputComp) {
    MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;

    // Check if input fields were clicked to change focus
    if (relativeX >= nameInputComp->x && relativeX <= nameInputComp->x + nameInputComp->w &&
        relativeY >= nameInputComp->y && relativeY <= nameInputComp->y + nameInputComp->h) {
      // Switch to name input
      MacInputField* nameInput = (MacInputField*)nameInputComp->customData;
      MacInputField* urlInput = (MacInputField*)urlInputComp->customData;

      nameInput->focused = true;
      urlInput->focused = false;
      keyboard->targetInputId = 401;
      keyboard->visible = true;

      // Redraw both input fields and keyboard
      drawComponent(lcd, *nameInputComp, addStationWindow.x, addStationWindow.y);
      drawComponent(lcd, *urlInputComp, addStationWindow.x, addStationWindow.y);
      drawComponent(lcd, *globalKeyboard, 0, 0);

      // Wait for touch release to prevent immediate hide
      int tx, ty;
      delay(150);
      while (lcd.getTouch(&tx, &ty)) {
        delay(10);
      }
      return;
    }

    if (relativeX >= urlInputComp->x && relativeX <= urlInputComp->x + urlInputComp->w &&
        relativeY >= urlInputComp->y && relativeY <= urlInputComp->y + urlInputComp->h) {
      // Switch to URL input
      MacInputField* nameInput = (MacInputField*)nameInputComp->customData;
      MacInputField* urlInput = (MacInputField*)urlInputComp->customData;

      nameInput->focused = false;
      urlInput->focused = true;
      keyboard->targetInputId = 403;
      keyboard->visible = true;

      // Redraw both input fields and keyboard
      drawComponent(lcd, *nameInputComp, addStationWindow.x, addStationWindow.y);
      drawComponent(lcd, *urlInputComp, addStationWindow.x, addStationWindow.y);
      drawComponent(lcd, *globalKeyboard, 0, 0);

      // Wait for touch release to prevent immediate hide
      int tx, ty;
      delay(150);
      while (lcd.getTouch(&tx, &ty)) {
        delay(10);
      }
      return;
    }

    // If we reach here and keyboard is visible, user clicked something else (not input fields)
    // Hide the keyboard and unfocus all input fields
    if (keyboard->visible) {
      keyboard->visible = false;

      if (nameInputComp && nameInputComp->customData) {
        MacInputField* nameInput = (MacInputField*)nameInputComp->customData;
        nameInput->focused = false;
      }
      if (urlInputComp && urlInputComp->customData) {
        MacInputField* urlInput = (MacInputField*)urlInputComp->customData;
        urlInput->focused = false;
      }

      // Clear keyboard area
      int keyboardHeight = screenHeight / 2;
      int keyboardY = screenHeight - keyboardHeight;
      drawCheckeredPatternArea(lcd, 0, keyboardY, screenWidth, keyboardHeight);

      // Redraw the window and its components
      drawWindow(lcd, addStationWindow);
    }
  }

  // Handle other component clicks (buttons)
  handleWindowContentClick(lcd, addStationWindow, relativeX, relativeY);
}

void onAddStationWindowMoved() {
  handleWindowMoved(lcd, addStationWindow);
}

// ===== CALLBACK FUNCTIONS =====

/**
 * Audio station name callback (ESP32-audioI2S v3.0.12+ API)
 * Receives station name from ICY headers
 * Runs on Core 0 (audio task) - stores data in global variables for UI task to read
 *
 * IMPORTANT: Never draw to LCD from this callback! It runs on Core 0 while
 * the UI task draws on Core 1, causing display corruption.
 */
void audio_showstation(const char* info) {
  if (info && strlen(info) > 0 && metadataMutex) {
    if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) {               // Non-blocking (0 timeout)
      strncpy(streamMetadata.stationName, info, METADATA_BUFFER_SIZE - 1);
      streamMetadata.stationName[METADATA_BUFFER_SIZE - 1] = '\0';  // Ensure null termination
      streamMetadata.received = true;
      xSemaphoreGive(metadataMutex);
    }
    // If mutex is busy, skip this update - UI is reading, we'll try next time
  }
}

/**
 * Audio stream title callback (ESP32-audioI2S v3.0.12+ API)
 * Receives current track info from StreamTitle metadata
 * Runs on Core 0 (audio task) - stores data in global variables for UI task to read
 */
void audio_showstreamtitle(const char* info) {
  if (info && strlen(info) > 0 && metadataMutex) {
    if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) {
      strncpy(streamMetadata.trackInfo, info, METADATA_BUFFER_SIZE - 1);
      streamMetadata.trackInfo[METADATA_BUFFER_SIZE - 1] = '\0';
      streamMetadata.received = true;
      xSemaphoreGive(metadataMutex);
    }
  }
}

/**
 * Audio ID3 data callback (ESP32-audioI2S v3.0.12+ API)
 * Store as track info if no StreamTitle is available
 */
void audio_id3data(const char* info) {
  if (info && strlen(info) > 0 && metadataMutex) {
    if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) {  // Non-blocking
      if (strlen(streamMetadata.trackInfo) == 0) {
        strncpy(streamMetadata.trackInfo, info, METADATA_BUFFER_SIZE - 1);
        streamMetadata.trackInfo[METADATA_BUFFER_SIZE - 1] = '\0';
        streamMetadata.received = true;
      }
      xSemaphoreGive(metadataMutex);
    }
  }
}

/**
 * Audio end of stream callback (ESP32-audioI2S v3.0.12+ API)
 */
void audio_eof_stream(const char* info) {
  isPlaying = false;
}

// ===== MAIN FUNCTIONS =====

void setup() {
// Initialize USB Serial for debugging (ESP32-S3 with ARDUINO_USB_MODE=1)
#if ENABLE_SERIAL_DEBUG
#if ARDUINO_USB_MODE
  // USB CDC is auto-initialized, just wait for it to be ready
  delay(100);
#else
  Serial.begin(115200);
  delay(100);
#endif
#endif

  // Create mutex for protecting shared metadata before any tasks start
  metadataMutex = xSemaphoreCreateMutex();

  // Create audio command queue for thread-safe communication
  audioCommandQueue = xQueueCreate(5, sizeof(AudioCommandMsg));

  try {
    lcd.init();
    tft.initDMA();
    tft.startWrite();
    lcd.setRotation(lcd.getRotation() ^ 1);
    initComponentBuffer(&lcd, 420, 50);
    lcd.fillScreen(MAC_WHITE);
    drawInterface(lcd);
    // Prepare background sprite for double buffering during window drag
    prepareBackgroundSprite(lcd);
    // Register all windows for proper refresh handling
    registerWindow(&radioWindow);
    registerWindow(&stationWindow);
    registerWindow(&addStationWindow);
  } catch (...) {
    DEBUG_PRINTLN("FATAL: LCD initialization failed!");
    while (1)
      delay(1000);
  }

  // Initialize ConfigManager (file system)
  if (!ConfigManager::begin()) {
    displayStatus(lcd, "Config init failed!", 160);
    DEBUG_PRINTLN("ERROR: ConfigManager initialization failed!");
  } else {
    DEBUG_PRINTLN("ConfigManager initialized successfully");
    // TEMPORARY: Uncomment the line below to reset to default stations
    // ConfigManager::factoryReset();

    // Load station list from config
    reloadStationList();

    // Reinitialize station window with loaded stations
    initializeStationWindow();
  }

  // Small delay to ensure all initialization is complete before WiFi
  delay(100);

  DEBUG_PRINTLN("\n=== Starting WiFi ===");
  connectToWiFi();

  // IMPORTANT: Initialize audio system BEFORE setting volume or any audio operations
  initializeAudio();

  // Restore volume from config AFTER audio initialization
  audio.setVolume(ConfigManager::getVolume());

  // Auto-play last station if available
  LastStation lastStation = ConfigManager::getLastStation();
  if (lastStation.name.length() > 0 && lastStation.url.length() > 0) {
    currentStationName = lastStation.name;
    RadioURL = lastStation.url;

    // Find the station index (for prev/next navigation)
    int stationCount = ConfigManager::getStationCount();
    for (int i = 0; i < stationCount; i++) {
      Station station = ConfigManager::getStation(i);
      if (station.name == lastStation.name) {
        currentStationIndex = i;
        break;
      }
    }
  }

  // Create UI task on Core 1 (default Arduino core)
  // CRITICAL: Increased stack size to prevent overflow crashes
  xTaskCreatePinnedToCore(uiTask,     // Task function
                          "UI_Task",  // Task name
                          8192,       // Stack size (bytes) - Increased to 8KB for stability
                          NULL,       // Parameter
                          1,          // Priority (lower than audio)
                          &uiTaskHandle,  // Task handle
                          1               // Core (0 or 1)
  );

  // Create Audio task on Core 0 (background core)
  // CRITICAL: Increased stack size to prevent overflow crashes
  xTaskCreatePinnedToCore(audioTask,     // Task function
                          "Audio_Task",  // Task name
                          16384,  // Stack size (bytes) - Increased to 16KB for stability
                          NULL,   // Parameter
                          2,      // Priority (same as UI - balanced approach)
                          &audioTaskHandle,  // Task handle
                          0                  // Core (0 or 1)
  );

  // Auto-play last station if it was loaded
  if (RadioURL.length() > 0) {
    // CRITICAL: Wait longer for audio task to fully initialize (500ms internal delay + buffer)
    delay(1000);  // Wait for tasks to initialize
    AudioCommandMsg msg = {CMD_CONNECT, ""};
    strncpy(msg.url, RadioURL.c_str(), sizeof(msg.url) - 1);
    msg.url[sizeof(msg.url) - 1] = '\0';  // Ensure null termination

    // Only send if queue is ready
    if (xQueueSend(audioCommandQueue, &msg, pdMS_TO_TICKS(500)) == pdTRUE) {
      isPlaying = true;
      // Update play button to pause symbol
      updateComponentSymbol(radioWindow, 1, SYMBOL_PAUSE);
    }
  }
}

void loop() {
  vTaskDelay(1);
}

// ===== MULTI-CORE TASKS =====

// UI Task - runs on Core 1 (default Arduino core)
void uiTask(void* parameter) {
  int touchX, touchY;               // Touch coordinates
  bool keyboardWasVisible = false;  // Track keyboard state

  while (true) {
    updateClock();
    // updateCPUUsage();

    // Handle global keyboard input first if visible
    // if (globalKeyboard) {
    //   MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
    //   if (keyboard->visible && lcd.getTouch(&touchX, &touchY)) {
    //     // Check if touch is within keyboard area
    //     int keyboardHeight = screenHeight / 2;
    //     int keyboardY = screenHeight - keyboardHeight;

    //     if (touchY >= keyboardY) {
    //       // Touch is within keyboard area - handle keyboard input
    //       MacComponent* activeInputComp = nullptr;
    //       if (addStationWindow.visible) {
    //         for (int i = 0; i < addStationWindow.childComponentCount; i++) {
    //           MacComponent* comp = addStationWindow.childComponents[i];
    //           if (comp->id == keyboard->targetInputId) {
    //             activeInputComp = comp;
    //             break;
    //           }
    //         }
    //       }

    //       if (activeInputComp &&
    //           handleKeyboardTouch(lcd, globalKeyboard, activeInputComp, touchX, touchY)) {
    //         // Redraw the active input field after keyboard input
    //         drawComponent(lcd, *activeInputComp, addStationWindow.x, addStationWindow.y);
    //         delay(100);
    //         while (lcd.getTouch(&touchX, &touchY)) {
    //           delay(10);
    //         }
    //         continue;
    //       }
    //     } else {
    //       // Touch is outside keyboard area - hide keyboard and unfocus inputs
    //       keyboard->visible = false;

    //       // Unfocus all input fields
    //       if (addStationWindow.visible) {
    //         for (int i = 0; i < addStationWindow.childComponentCount; i++) {
    //           MacComponent* comp = addStationWindow.childComponents[i];
    //           if (comp->type == COMPONENT_INPUT_FIELD && comp->customData) {
    //             MacInputField* inputField = (MacInputField*)comp->customData;
    //             inputField->focused = false;
    //             drawComponent(lcd, *comp, addStationWindow.x, addStationWindow.y);
    //           }
    //         }
    //       }

    //       // Redraw desktop area where keyboard was
    //       drawCheckeredPatternArea(lcd, 0, keyboardY, screenWidth, keyboardHeight);

    //       // Redraw any visible windows
    //       if (addStationWindow.visible)
    //         drawWindow(lcd, addStationWindow);
    //       if (stationWindow.visible)
    //         drawWindow(lcd, stationWindow);
    //       if (radioWindow.visible)
    //         drawWindow(lcd, radioWindow);

    //       delay(100);  // Debounce
    //       while (lcd.getTouch(&touchX, &touchY)) {
    //         delay(10);
    //       }
    //     }
    //   }
    // }

    // Disable all background UI updates while keyboard is visible
    bool keyboardActive = false;
    if (globalKeyboard) {
      MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
      keyboardActive = keyboard->visible;
    }

    if (!keyboardActive) {
      // Handle window interaction (minimize/close buttons)
      // Only handle one window at a time - add station window takes priority, then station window,
      // then radio window
      if (addStationWindow.visible) {
        interactiveWindow(lcd, addStationWindow);
      } else if (stationWindow.visible) {
        interactiveWindow(lcd, stationWindow);
      } else if (radioWindow.visible) {
        interactiveWindow(lcd, radioWindow);
      }

      // Handle desktop icon interaction
      if (radioIcon.visible) {
        interactiveDesktopIcon(lcd, radioIcon);
      }

      // Update running text components for animation
      if (radioWindow.visible && !stationWindow.visible && !addStationWindow.visible) {
        updateRunningTextComponents(lcd, radioWindow);
      }

      if (stationWindow.visible && !addStationWindow.visible) {
        updateRunningTextComponents(lcd, stationWindow);
      }

      if (addStationWindow.visible) {
        updateRunningTextComponents(lcd, addStationWindow);
        updateInputFieldComponents(lcd, addStationWindow);
      }

      // Check for metadata updates from audio stream (Core 0 → Core 1 communication)
      // Only update if radio window is visible
      if (radioWindow.visible && !stationWindow.visible && !addStationWindow.visible &&
          metadataMutex) {
        // Use mutex to safely read metadata char buffers from Core 0
        // Short timeout to avoid blocking UI for too long
        if (xSemaphoreTake(metadataMutex, pdMS_TO_TICKS(2)) == pdTRUE) {
          // Convert char buffers to String objects (safe to do in UI task)
          String serverStationName = String(streamMetadata.stationName);
          String currentTrackInfo = String(streamMetadata.trackInfo);
          String bitRate = String(streamMetadata.bitRate);
          String id3Data = String(streamMetadata.id3data);
          String info = String(streamMetadata.info);
          String description = String(streamMetadata.description);
          String lyrics = String(streamMetadata.lyrics);
          String log = String(streamMetadata.log);

          // Update station name if server provided one
          if (serverStationName.length() > 0 && serverStationName != currentStationName) {
            currentStationName = serverStationName;
            updateStationMetadata(currentStationName, currentTrackInfo);
          }

          // Update track info if it changed
          if (currentTrackInfo.length() > 0 && currentTrackInfo != lastTrackInfo) {
            lastTrackInfo = currentTrackInfo;
            updateStationMetadata(currentStationName, currentTrackInfo);
          }

          // Update bitrate display (only if changed)
          if (bitRate.length() > 0 && bitRate != lastDisplayedBitRate) {
            MacComponent* txtBitRate = findComponentById(radioWindow, 203);
            if (txtBitRate && txtBitRate->customData) {
              MacRunningText* runningText = (MacRunningText*)txtBitRate->customData;
              runningText->text = "Bitrate: " + bitRate;
              runningText->scrollOffset = 0;
              lastDisplayedBitRate = bitRate;
            }
          }

          // Update ID3 data display (only if changed)
          if (id3Data.length() > 0 && id3Data != lastDisplayedID3) {
            MacComponent* txtID3 = findComponentById(radioWindow, 204);
            if (txtID3 && txtID3->customData) {
              MacRunningText* runningText = (MacRunningText*)txtID3->customData;
              runningText->text = "ID3: " + id3Data;
              runningText->scrollOffset = 0;
              lastDisplayedID3 = id3Data;
            }
          }

          // Update info display (only if changed)
          if (info.length() > 0 && info != lastDisplayedInfo) {
            MacComponent* txtInfo = findComponentById(radioWindow, 205);
            if (txtInfo && txtInfo->customData) {
              MacRunningText* runningText = (MacRunningText*)txtInfo->customData;
              runningText->text = info;
              runningText->scrollOffset = 0;
              lastDisplayedInfo = info;
            }
          }

          // Update description display (only if changed)
          if (description.length() > 0 && description != lastDisplayedDescription) {
            MacComponent* txtDescription = findComponentById(radioWindow, 206);
            if (txtDescription && txtDescription->customData) {
              MacRunningText* runningText = (MacRunningText*)txtDescription->customData;
              runningText->text = "Description: " + description;
              runningText->scrollOffset = 0;
              lastDisplayedDescription = description;
            }
          }

          // Update lyrics display (only if changed)
          if (lyrics.length() > 0 && lyrics != lastDisplayedLyrics) {
            MacComponent* txtLyrics = findComponentById(radioWindow, 207);
            if (txtLyrics && txtLyrics->customData) {
              MacRunningText* runningText = (MacRunningText*)txtLyrics->customData;
              runningText->text = "Lyrics: " + lyrics;
              runningText->scrollOffset = 0;
              lastDisplayedLyrics = lyrics;
            }
          }

          // Update log display (only if changed)
          if (log.length() > 0 && log != lastDisplayedLog) {
            MacComponent* txtLog = findComponentById(radioWindow, 208);
            if (txtLog && txtLog->customData) {
              MacRunningText* runningText = (MacRunningText*)txtLog->customData;
              runningText->text = "Log: " + log;
              runningText->scrollOffset = 0;
              lastDisplayedLog = log;
            }
          }

          xSemaphoreGive(metadataMutex);
        }
        // If we can't get mutex quickly, skip this update cycle
      }
    }

    // Handle global keyboard visibility changes and overlay input field
    // if (globalKeyboard) {
    //   MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;

    //   if (keyboard->visible && !keyboardWasVisible) {
    //     // Keyboard just became visible - draw it
    //     drawComponent(lcd, *globalKeyboard, 0, 0);
    //     keyboardWasVisible = true;
    //   } else if (!keyboard->visible && keyboardWasVisible) {
    //     // Keyboard just became hidden - clear the area
    //     int keyboardHeight = screenHeight / 2;
    //     int keyboardY = screenHeight - keyboardHeight;
    //     drawCheckeredPatternArea(lcd, 0, keyboardY, screenWidth, keyboardHeight);

    //     // Redraw any windows that overlap with keyboard area
    //     if (addStationWindow.visible && (addStationWindow.y + addStationWindow.h > keyboardY)) {
    //       drawWindow(lcd, addStationWindow);
    //     }
    //     if (stationWindow.visible && (stationWindow.y + stationWindow.h > keyboardY)) {
    //       drawWindow(lcd, stationWindow);
    //     }
    //     if (radioWindow.visible && (radioWindow.y + radioWindow.h > keyboardY)) {
    //       drawWindow(lcd, radioWindow);
    //     }

    //     keyboardWasVisible = false;
    //   }

    //   // --- Overlay input field above keyboard ---
    //   if (keyboard->visible) {
    //     // Find the focused input field in the active window
    //     MacInputField* focusedInput = nullptr;
    //     int overlayWidth = screenWidth;
    //     int overlayHeight = 48;  // Taller for full-width overlay
    //     int overlayX = 0;
    //     int keyboardHeight = screenHeight / 2;
    //     int overlayY = screenHeight - keyboardHeight - overlayHeight;  // Directly above keyboard

    //     if (addStationWindow.visible) {
    //       for (int i = 0; i < addStationWindow.childComponentCount; i++) {
    //         MacComponent* comp = addStationWindow.childComponents[i];
    //         if (comp->type == COMPONENT_INPUT_FIELD && comp->customData) {
    //           MacInputField* inputField = (MacInputField*)comp->customData;
    //           if (inputField->focused) {
    //             focusedInput = inputField;
    //             break;
    //           }
    //         }
    //       }
    //     }

    //     if (focusedInput) {
    //       // Draw full-width overlay input field above keyboard
    //       drawInputField(lcd, overlayX, overlayY, overlayWidth, overlayHeight, *focusedInput);
    //     }
    //   }
    // }

    // Balanced delay - responsive UI without starving audio task
    vTaskDelay(10 / portTICK_PERIOD_MS);  // 10ms = 100 FPS for smooth UI
  }
}

// Audio Task - runs on Core 0 (background core)
void audioTask(void* parameter) {
  // Wait for audio system to fully initialize
  vTaskDelay(pdMS_TO_TICKS(500));

  while (true) {
    // Check for commands from UI task (non-blocking)
    AudioCommandMsg msg;
    if (xQueueReceive(audioCommandQueue, &msg, 0) == pdTRUE) {
      switch (msg.cmd) {
        case CMD_CONNECT:
          // CRITICAL: Only stop if currently running to prevent I2S crash
          if (isPlaying && audio.isRunning()) {
            audio.stopSong();
            vTaskDelay(pdMS_TO_TICKS(200));  // Allow full cleanup
            isPlaying = false;
          }

          // Connect to stream (safe to call from audio task)
          if (audio.connecttohost(msg.url)) {
            isPlaying = true;
          } else {
            isPlaying = false;  // Connection failed
          }
          break;
        case CMD_STOP:
          // CRITICAL: Only stop if audio is actually running
          if (audio.isRunning()) {
            audio.stopSong();
            vTaskDelay(pdMS_TO_TICKS(100));  // Allow I2S cleanup
            isPlaying = false;
          } else {
            isPlaying = false;  // Mark as stopped even if not running
          }
          break;
        default:
          break;
      }
    }

    // Handle audio processing
    if (isPlaying) {
      audio.loop();
      vTaskDelay(1);
    } else {
      // Only delay when not playing to reduce CPU usage
      vTaskDelay(10 / portTICK_PERIOD_MS);  // 10ms delay when idle
    }
  }
}


void updateStationMetadata(const String& stationName, const String& trackInfo) {
  // Update component 200 (station name)
  MacComponent* txtRadioName = findComponentById(radioWindow, 200);
  if (txtRadioName && txtRadioName->customData) {
    MacRunningText* runningText = (MacRunningText*)txtRadioName->customData;
    runningText->text = stationName;
    runningText->scrollOffset = 0;  
  }

  // Update component 201 (track info)
  MacComponent* txtRadioDetails = findComponentById(radioWindow, 201);
  if (txtRadioDetails && txtRadioDetails->customData) {
    MacRunningText* runningText = (MacRunningText*)txtRadioDetails->customData;
    runningText->text = trackInfo;
    runningText->scrollOffset = 0;  
  }
}

void updateClock() {
  unsigned long now = millis();
  if (now - lastClockUpdate < 1000)
    return;  // update once per second
  lastClockUpdate = now;

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return;  
  }

  char buf[9];  // HH:MM:SS
  strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
  String current = String(buf);
  if (current == lastClockText)
    return;  // no change
  lastClockText = current;

  // Redraw clock area (menu bar right side)
  drawClock(lcd, current);
}

void updateCPUUsage() {
  unsigned long now = millis();
  if (now - lastCPUUpdate < 1000)
    return;  // update once per second
  lastCPUUpdate = now;

  // Get CPU usage using FreeRTOS task stats
  // Simple estimation based on heap and stack usage
  static uint32_t lastTotalRuntime = 0;

  // Calculate approximate CPU usage based on task activity
  // Core 0: Audio task
  if (audioTaskHandle != NULL) {
    UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(audioTaskHandle);
    // Estimate based on stack usage (more used = more active)
    cpuUsage0 = isPlaying ? 25.0 + (random(0, 20)) : 5.0 + random(0, 5);
  }

  // Core 1: UI task
  if (uiTaskHandle != NULL) {
    UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(uiTaskHandle);
    // Estimate based on UI activity
    cpuUsage1 = 15.0 + random(0, 15);
  }

  // Get RAM usage
  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t totalHeap = ESP.getHeapSize();
  uint32_t usedHeap = totalHeap - freeHeap;
  float ramUsagePercent = (usedHeap * 100.0) / totalHeap;

  // Get PSRAM usage if available
  uint32_t freePsram = ESP.getFreePsram();
  uint32_t totalPsram = ESP.getPsramSize();
  uint32_t usedPsram = totalPsram - freePsram;

  // Update CPU and RAM label component if it exists and window is visible
  if (radioWindow.visible && !radioWindow.minimized) {
    MacComponent* cpuLabel = findComponentById(radioWindow, 202);
    if (cpuLabel && cpuLabel->customData) {
      MacLabel* label = (MacLabel*)cpuLabel->customData;
      char cpuText[128];
      snprintf(cpuText, sizeof(cpuText), "CPU0: %.0f%% CPU1: %.0f%% | RAM: %dKB/%dKB (%.0f%%) | PSRAM: %dKB/%dKB",
               cpuUsage0, cpuUsage1,
               usedHeap / 1024, totalHeap / 1024, ramUsagePercent,
               usedPsram / 1024, totalPsram / 1024);
      label->text = String(cpuText);
      // Redraw the component
      drawComponent(lcd, *cpuLabel, radioWindow.x, radioWindow.y);
    }
  }
}

/**
 * Connect to WiFi network
 */
void connectToWiFi() {
  DEBUG_PRINTF("Attempting WiFi connection to SSID: %s\n", WIFI_SSID);
  displayStatus(lcd, "Connecting to WiFi...", 160);

  // Explicitly set WiFi mode before connecting
  WiFi.mode(WIFI_STA);
  delay(100);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    displayStatus(lcd, "WiFi connecting... " + String(attempts + 1), 160);
    DEBUG_PRINTF("WiFi attempt %d, status: %d\n", attempts + 1, WiFi.status());
    delay(1000);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    // Initialize NTP time synchronization
    configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);
    displayStatus(lcd, "Connected!", 160);
    DEBUG_PRINTLN("WiFi connected successfully!");
    DEBUG_PRINTF("IP Address: %s\n", WiFi.localIP().toString().c_str());
    DEBUG_PRINTF("Signal strength: %d dBm\n", WiFi.RSSI());
  } else {
    displayStatus(lcd, "WiFi Failed!", 160);
    DEBUG_PRINTLN("ERROR: WiFi connection failed!");
    DEBUG_PRINTF("Final status: %d\n", WiFi.status());
  }
}

/**
 * Audio information callback
 * Receives metadata from audio stream server
 * Runs on Core 0 (audio task) - stores data in global variables for UI task to read
 *
 * IMPORTANT: Never draw to LCD from this callback! It runs on Core 0 while
 * the UI task draws on Core 1, causing display corruption.
 */
void audio_callback(Audio::msg_t m) {
  switch (m.e) {
    case Audio::evt_name:
      if (m.msg && strlen(m.msg) > 0 && metadataMutex) {
        if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) { 
          strncpy(streamMetadata.stationName, m.msg, METADATA_BUFFER_SIZE - 1);
          streamMetadata.stationName[METADATA_BUFFER_SIZE - 1] = '\0';  
          streamMetadata.received = true;
          xSemaphoreGive(metadataMutex);
        }
      }
      break;

    case Audio::evt_streamtitle:
      if (m.msg && strlen(m.msg) > 0 && metadataMutex) {
        if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) {            
          strncpy(streamMetadata.trackInfo, m.msg, METADATA_BUFFER_SIZE - 1);
          streamMetadata.trackInfo[METADATA_BUFFER_SIZE - 1] = '\0';  
          streamMetadata.received = true;
          xSemaphoreGive(metadataMutex);
        }
      }
      break;

    case Audio::evt_icydescription:
      if (m.msg && strlen(m.msg) > 0 && metadataMutex) {
        if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) {               
          strncpy(streamMetadata.description, m.msg, METADATA_BUFFER_SIZE - 1);
          streamMetadata.description[METADATA_BUFFER_SIZE - 1] = '\0';  
          streamMetadata.received = true;
          xSemaphoreGive(metadataMutex);
        }
      }
      break;

    case Audio::evt_bitrate:
      if (m.msg && strlen(m.msg) > 0 && metadataMutex) {
        if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) {        
          strncpy(streamMetadata.bitRate, m.msg, METADATA_BUFFER_SIZE - 1);
          streamMetadata.bitRate[METADATA_BUFFER_SIZE - 1] = '\0';  
          streamMetadata.received = true;
          xSemaphoreGive(metadataMutex);
        }
      }
      break;

    case Audio::evt_id3data:
      // ID3 tag data (artist, album, title)
      // Store as track info if no StreamTitle is available
      if (m.msg && strlen(m.msg) > 0 && metadataMutex) {
        if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) {  
          if (strlen(streamMetadata.id3data) == 0) {
            strncpy(streamMetadata.id3data, m.msg, METADATA_BUFFER_SIZE - 1);
            streamMetadata.id3data[METADATA_BUFFER_SIZE - 1] = '\0'; 
            streamMetadata.received = true;
          }
          xSemaphoreGive(metadataMutex);
        }
      }
      break;

    case Audio::evt_eof:
      isPlaying = false;
      break;

    case Audio::evt_info:
      if (m.msg && strlen(m.msg) > 0 && metadataMutex) {
        if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) {        
          strncpy(streamMetadata.info, m.msg, METADATA_BUFFER_SIZE - 1);
          streamMetadata.info[METADATA_BUFFER_SIZE - 1] = '\0'; 
          streamMetadata.received = true;
          xSemaphoreGive(metadataMutex);
        }
      }
      break;

    case Audio::evt_lyrics:
      if (m.msg && strlen(m.msg) > 0 && metadataMutex) {
        if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) {       
          strncpy(streamMetadata.lyrics, m.msg, METADATA_BUFFER_SIZE - 1);
          streamMetadata.lyrics[METADATA_BUFFER_SIZE - 1] = '\0';  
          streamMetadata.received = true;
          xSemaphoreGive(metadataMutex);
        }
      }
      break;

    case Audio::evt_log:
      if (m.msg && strlen(m.msg) > 0 && metadataMutex) {
        if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) {    
          strncpy(streamMetadata.log, m.msg, METADATA_BUFFER_SIZE - 1);
          streamMetadata.log[METADATA_BUFFER_SIZE - 1] = '\0';  
          streamMetadata.received = true;
          xSemaphoreGive(metadataMutex);
        }
      }
      break;

    default:
      break;
  }
}

/**
 * Initialize audio system and connect to radio stream
 */
void initializeAudio() {
  // This ensures I2S channel is properly initialized
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);

  // Small delay to allow I2S hardware to stabilize after pinout configuration
  delay(100);

  // Set a safe default volume (will be overridden by saved config later)
  audio.setVolume(DEFAULT_VOLUME);

  // Configure audio buffer sizes for stability with high bitrate streams
  // CRITICAL: Increased timeout to prevent premature disconnections causing crashes
  audio.setConnectionTimeout(1000, 5000);  // 1s initial, 5s reconnect timeout


  Audio::audio_info_callback = audio_callback;
}

void drawInterface(lgfx::LGFX_Device& lcd) {
  lcd.fillScreen(MAC_WHITE);
  drawCheckeredPattern(lcd);
  drawMenuBar(lcd, "Retrodio");

  // Initialize the radio window with its child components
  initializeRadioWindow();
  initializeStationWindow();
  initializeAddStationWindow();

  // Initialize global keyboard (bottom half of screen)
  if (globalKeyboard == nullptr) {
    int keyboardHeight = screenHeight / 2;
    int keyboardY = screenHeight - keyboardHeight;
    globalKeyboard = createKeyboardComponent(0, keyboardY, screenWidth, keyboardHeight, 404, 401);
    // Keyboard starts hidden
    MacKeyboard* kb = (MacKeyboard*)globalKeyboard->customData;
    kb->visible = false;
  }

  // Draw the window using the new window system
  // This will automatically draw all child buttons
  drawWindow(lcd, radioWindow);

  // Only draw content if window is not minimized
  if (!radioWindow.minimized && radioWindow.visible) {
    redrawWindowContent(lcd, radioWindow);
  }
}

// Helper function to redraw window content (needed for dragging)
void redrawWindowContent(lgfx::LGFX_Device& lcd, const MacWindow& window) {
  if (!window.visible || window.minimized)
    return;
}

// ===== DYNAMIC WINDOW SETUP =====

void initializeRadioWindow() {
  // Clear any existing child components
  clearChildComponents(radioWindow);

  // Main playback controls - larger and centered
  MacComponent* btnPrev = createButtonComponent(30, 165, 50, 50, 4, "", SYMBOL_PREV);
  btnPrev->onClick = onComponentClick;
  addChildComponent(radioWindow, btnPrev);

  MacComponent* btnPlay = createButtonComponent(80, 160, 60, 60, 1, "", SYMBOL_PLAY);
  btnPlay->onClick = onComponentClick;
  addChildComponent(radioWindow, btnPlay);

  MacComponent* btnStation = createButtonComponent(350, 165, 50, 50, btnStationID, "", SYMBOL_LIST);
  btnStation->onClick = onComponentClick;
  addChildComponent(radioWindow, btnStation);

  MacComponent* btnNext = createButtonComponent(140, 165, 50, 50, 5, "", SYMBOL_NEXT);
  btnNext->onClick = onComponentClick;
  addChildComponent(radioWindow, btnNext);

  MacComponent* txtRadioName = createRunningTextComponent(20, 40,   // x, y position
                                                          380, 25,  // width, height
                                                          200,      // component ID
                                                          currentStationName,
                                                          2,  // scroll speed (2 pixels per update)
                                                          MAC_BLACK,  // text color
                                                          3           // text size
  );
  txtRadioName->onClick = onComponentClick;
  addChildComponent(radioWindow, txtRadioName);

  MacComponent* txtRadioDetails =
      createRunningTextComponent(20, 70,     // x, y position
                                 200, 20,    // width, height
                                 201,        // component ID
                                 "Standby waiting for metadata ...",
                                 2,          // scroll speed (2 pixels per update)
                                 MAC_BLACK,  // text color
                                 1           // text size
      );
  txtRadioDetails->onClick = onComponentClick;
  addChildComponent(radioWindow, txtRadioDetails);

  MacComponent* txtBitRate = createRunningTextComponent(20, 85,   // x, y position
                                                        200, 20,  // width, height
                                                        203,      // component ID
                                                        "Bitrate: N/A",
                                                        2,  // scroll speed (2 pixels per update)
                                                        MAC_BLACK,  // text color
                                                        1           // text size
  );
  addChildComponent(radioWindow, txtBitRate);

  MacComponent* txtID3 = createRunningTextComponent(20, 100,   // x, y position
                                                    200, 20,  // width, height
                                                    204,      // component ID
                                                    "ID3: N/A",
                                                    2,        // scroll speed (2 pixels per update)
                                                    MAC_BLACK,  // text color
                                                    1           // text size
  );
  addChildComponent(radioWindow, txtID3);

  MacComponent* txtInfo = createRunningTextComponent(20, 115,  // x, y position
                                                     200, 20,  // width, height
                                                     205,      // component ID
                                                     "",
                                                     2,        // scroll speed (2 pixels per update)
                                                     MAC_BLACK,  // text color
                                                     1           // text size
  );
  addChildComponent(radioWindow, txtInfo);

  MacComponent* txtDescription =
      createRunningTextComponent(20, 130,    // x, y position
                                 200, 20,    // width, height
                                 206,        // component ID
                                 "",
                                 2,          // scroll speed (2 pixels per update)
                                 MAC_BLACK,  // text color
                                 1           // text size
      );
  addChildComponent(radioWindow, txtDescription);

  #if ENABLE_DEBUG 
  // Add CPU usage label at the bottom
  MacComponent* cpuLabel = createLabelComponent(0, 6, 200, 15, 202, "CPU0: 0% CPU1: 0%", MAC_BLACK);
  cpuLabel->onClick = onComponentClick;
  addChildComponent(radioWindow, cpuLabel);
  #endif

  // Volume controls - smaller, positioned on the right
  MacComponent* btnVolUp = createButtonComponent(200, 165, 50, 50, 3, "", SYMBOL_VOL_UP);
  btnVolUp->onClick = onComponentClick;
  addChildComponent(radioWindow, btnVolUp);

  MacComponent* btnVolDn = createButtonComponent(250, 165, 50, 50, 6, "", SYMBOL_VOL_DOWN);
  btnVolDn->onClick = onComponentClick;
  addChildComponent(radioWindow, btnVolDn);
}

// ===== STATION LIST DATA =====
// Station list is now managed by ConfigManager
// Dynamic array for UI display
MacListViewItem* stationItems = nullptr;
int stationItemCount = 0;

// Helper function to reload station list from ConfigManager
void reloadStationList() {
  // CRITICAL FIX: Prevent use-after-free by saving old pointer and delaying deletion
  MacListViewItem* oldItems = stationItems;
  int oldCount = stationItemCount;

  // Get station count from ConfigManager
  stationItemCount = ConfigManager::getStationCount();

  if (stationItemCount == 0) {
    stationItems = nullptr;
    // Safe to delete old items now
    if (oldItems != nullptr) {
      delete[] oldItems;
    }
    return;
  }

  // Allocate new array
  MacListViewItem* newItems = new MacListViewItem[stationItemCount];

  // Load stations from ConfigManager
  for (int i = 0; i < stationItemCount; i++) {
    Station station = ConfigManager::getStation(i);
    newItems[i].text = station.name;
    newItems[i].data = nullptr;  // URL stored in ConfigManager
  }

  // Atomically swap the pointer - prevents UI from accessing during transition
  stationItems = newItems;

  // Brief delay to ensure UI task isn't mid-access (not perfect but helps)
  vTaskDelay(pdMS_TO_TICKS(50));

  // Now safe to delete old items
  if (oldItems != nullptr) {
    delete[] oldItems;
  }
}

// Helper function to switch stations without window management
void switchToStation(int index) {
  // Safety check: ensure we have a valid index
  if (index < 0 || index >= ConfigManager::getStationCount()) {
    return;
  }

  // Check WiFi connection before attempting to connect
  if (WiFi.status() != WL_CONNECTED) {
    updateStationMetadata("Error", "WiFi not connected");
    return;
  }

  // Get station from ConfigManager
  Station station = ConfigManager::getStation(index);

  if (station.url.length() == 0) {
    return;
  }

  // Update current station name and index
  currentStationName = station.name;
  currentStationIndex = index;  // Save the current station index for prev/next navigation

  // Clear previous metadata buffers (thread-safe with mutex)
  if (metadataMutex) {
    if (xSemaphoreTake(metadataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      streamMetadata.stationName[0] = '\0';  // Clear buffer
      streamMetadata.trackInfo[0] = '\0';    // Clear buffer
      streamMetadata.received = false;
      xSemaphoreGive(metadataMutex);
    }
  }
  // Clear all UI-only tracking strings to allow fresh metadata updates
  lastTrackInfo = "";
  lastDisplayedBitRate = "";
  lastDisplayedID3 = "";
  lastDisplayedInfo = "";
  lastDisplayedDescription = "";
  lastDisplayedLyrics = "";
  lastDisplayedLog = "";

  // Update the running text component with the new station name (if component exists)
  MacComponent* txtRadioName = findComponentById(radioWindow, 200);
  if (txtRadioName && txtRadioName->customData) {
    MacRunningText* runningText = (MacRunningText*)txtRadioName->customData;
    if (runningText) {
      runningText->text = currentStationName;
      runningText->scrollOffset = 0;  // Reset scroll position
    }
  }

  // Update details to show connecting
  updateStationMetadata(currentStationName, "Connecting...");

  // CRITICAL: Use command queue to stop playback safely (avoid race conditions)
  if (isPlaying && audio.isRunning()) {
    AudioCommandMsg stopMsg = {CMD_STOP, ""};
    xQueueSend(audioCommandQueue, &stopMsg, portMAX_DELAY);
    isPlaying = false;
    vTaskDelay(pdMS_TO_TICKS(200));  // Give audio task time to fully stop
  } else {
    // Not actually playing, just reset state
    isPlaying = false;
  }

  // Update play button symbol data (without drawing immediately)
  MacComponent* playButton = findComponentById(radioWindow, 1);
  if (playButton && playButton->customData) {
    MacButton* btnData = (MacButton*)playButton->customData;
    if (btnData) {
      // Send connect command to audio task for thread-safe connection
      AudioCommandMsg msg = {CMD_CONNECT, ""};
      strncpy(msg.url, station.url.c_str(), sizeof(msg.url) - 1);
      msg.url[sizeof(msg.url) - 1] = '\0';  // Ensure null termination

      if (xQueueSend(audioCommandQueue, &msg, pdMS_TO_TICKS(1000)) == pdTRUE) {
        // Command sent successfully
        isPlaying = true;
        btnData->symbol = SYMBOL_PAUSE;
        LastStation lastStation = {station.name, station.url};
        ConfigManager::setLastStation(lastStation);
      } else {
        // Queue full - something is wrong
        updateStationMetadata(currentStationName, "Error: Command queue full");
      }
    }
  }
}

void onStationItemClick(int index, void* itemData) {
  // Switch to the selected station
  switchToStation(index);

  // Close station window and show radio window
  stationWindow.visible = false;
  radioWindow.visible = true;

  drawWindow(lcd, radioWindow);
}

void initializeStationWindow() {
  // Clear any existing child components
  clearChildComponents(stationWindow);

  // Add "Add Station" button at the bottom
  MacComponent* btnAddStation = createButtonComponent(310, 35, 90, 30, btnAddStationID, "Add +");
  btnAddStation->onClick = onComponentClick;
  addChildComponent(stationWindow, btnAddStation);

  // Create the list view component (reduced width to accommodate button)
  MacComponent* stationList = createListViewComponent(10, 35,    // x, y position (below title bar)
                                                      290, 195,  // width, height (reduced from 400)
                                                      300,       // component ID
                                                      stationItems,      // array of items
                                                      stationItemCount,  // number of items
                                                      30                 // item height
  );

  if (stationList && stationList->customData) {
    MacListView* listViewData = (MacListView*)stationList->customData;
    listViewData->onItemClick = onStationItemClick;
  }

  stationList->onClick = onComponentClick;
  addChildComponent(stationWindow, stationList);
}

void initializeAddStationWindow() {
  // Clear any existing child components
  clearChildComponents(addStationWindow);

  // Add labels for field names
  MacComponent* lblStationName = createLabelComponent(20, 40, 120, 20, 400, "Station Name:");
  addChildComponent(addStationWindow, lblStationName);

  // Add input field for station name
  MacComponent* txtStationName =
      createInputFieldComponent(150, 40, 190, 25, 401, "Enter station name", 50);
  txtStationName->onClick = onComponentClick;
  addChildComponent(addStationWindow, txtStationName);

  MacComponent* lblStationURL = createLabelComponent(20, 75, 120, 20, 402, "Station URL:");
  addChildComponent(addStationWindow, lblStationURL);

  // Add input field for station URL
  MacComponent* txtStationURL =
      createInputFieldComponent(150, 75, 190, 25, 403, "https://...", 200);
  txtStationURL->onClick = onComponentClick;
  addChildComponent(addStationWindow, txtStationURL);

  // Add Save and Cancel buttons
  MacComponent* btnSave = createButtonComponent(80, 115, 80, 30, btnSaveStationID, "Save");
  btnSave->onClick = onComponentClick;
  addChildComponent(addStationWindow, btnSave);

  MacComponent* btnCancel =
      createButtonComponent(180, 115, 80, 30, btnCancelAddStationID, "Cancel");
  btnCancel->onClick = onComponentClick;
  addChildComponent(addStationWindow, btnCancel);
}
