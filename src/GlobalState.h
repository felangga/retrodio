/*
 * GlobalState.h - Shared Global Variables
 *
 * Copyright (c) 2025 felangga
 *
 */

#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include <Arduino.h>
#include "Audio.h"
#include "UI.h"
#include "wt32_sc01_plus.h"

// Component ID Constants
extern const int CMP_NOW_PLAYING_LABEL;
extern const int CMP_VOLUME_SLIDER;
extern const int CMP_BUFFER_PROGRESS;
extern const int CMP_AUTO_PLAY_CHECKBOX;
extern const int CMP_VISUALS_CHECKBOX;

// Radio Window Text Components
extern const int TXT_RADIO_NAME;
extern const int TXT_RADIO_DETAILS;
extern const int TXT_CPU_LABEL;
extern const int TXT_BITRATE;
extern const int TXT_ID3;
extern const int TXT_INFO;
extern const int TXT_DESCRIPTION;
extern const int TXT_LYRICS;
extern const int TXT_LOG;

// Add Station Window Components
extern const int LBL_STATION_NAME;
extern const int INPUT_STATION_NAME;
extern const int LBL_STATION_URL;
extern const int INPUT_STATION_URL;
extern const int KEYBOARD_COMPONENT;

extern const int BTN_PLAY;
extern const int BTN_STOP;
extern const int BTN_VOL_UP;
extern const int BTN_PREV;
extern const int BTN_NEXT;
extern const int BTN_VOL_DOWN;
extern const int BTN_STATION;
extern const int BTN_ADD_STATION;
extern const int BTN_EDIT_STATION;
extern const int BTN_SAVE_STATION;
extern const int BTN_CANCEL_ADD_STATION;
extern const int BTN_DELETE_STATION;
extern const int BTN_CONFIRM_YES;
extern const int BTN_CONFIRM_NO;

// Settings Window Components
extern const int BTN_SETTINGS_SAVE;
extern const int BTN_SETTINGS_CANCEL;
extern const int CMP_POWER_SAVE_CHECKBOX;
extern const int LBL_POWER_SAVE;
extern const int LBL_IDLE_TIMEOUT;
extern const int CMP_IDLE_TIMEOUT_SLIDER;
extern const int LBL_TIMEOUT_VALUE;
extern const int LBL_ABOUT_TITLE;
extern const int LBL_ABOUT_AUTHOR;
extern const int LBL_ABOUT_GITHUB;

// WiFi Window Components
extern const int WIFI_LIST_COMPONENT;
extern const int BTN_WIFI_CONNECT;
extern const int BTN_WIFI_CANCEL;
extern const int BTN_WIFI_REFRESH;
extern const int LBL_WIFI_TITLE;
extern const int INPUT_WIFI_PASSWORD;
extern const int LBL_WIFI_PASSWORD;
extern const int BTN_WIFI_PASSWORD_OK;
extern const int BTN_WIFI_PASSWORD_CANCEL;
extern const int WIFI_KEYBOARD_COMPONENT;

// Global Objects
extern LGFX lcd;
extern Audio audio;

// Task Handles
extern TaskHandle_t uiTaskHandle;
extern TaskHandle_t audioTaskHandle;
extern SemaphoreHandle_t metadataMutex;
extern QueueHandle_t audioCommandQueue;

// Audio Command Types
enum AudioCommand { CMD_NONE = 0, CMD_PLAY, CMD_STOP, CMD_CONNECT };

struct AudioCommandMsg {
  AudioCommand cmd;
  char url[256];
};

// Clock and CPU Monitoring
extern unsigned long lastClockUpdate;
extern String lastClockText;
extern unsigned long lastCPUUpdate;
extern float cpuUsage0;
extern float cpuUsage1;

// Volume Display Timeout
extern unsigned long volumeChangeTime;
extern bool volumeDisplayActive;
extern String savedStationName;

// Power Save State
extern unsigned long lastActivityTime;
extern bool lcdSleeping;
extern uint8_t savedBrightness;

// UI Redraw Flags
extern volatile bool needsVolumeSliderRedraw;
extern volatile bool needsStationListReload;

// Station State
extern volatile bool isPlaying;
extern String currentStationName;
extern String RadioURL;
extern int currentStationIndex;

// Metadata Buffer
#define METADATA_BUFFER_SIZE 128

typedef struct {
  char stationName[METADATA_BUFFER_SIZE];
  char trackInfo[METADATA_BUFFER_SIZE];
  char bitRate[METADATA_BUFFER_SIZE];
  char id3data[METADATA_BUFFER_SIZE];
  char info[METADATA_BUFFER_SIZE];
  char description[METADATA_BUFFER_SIZE];
  char lyrics[METADATA_BUFFER_SIZE];
  char log[METADATA_BUFFER_SIZE];
  volatile bool received;
} StreamMetadata;

extern StreamMetadata streamMetadata;
extern String lastTrackInfo;
extern String lastDisplayedBitRate;
extern String lastDisplayedID3;
extern String lastDisplayedInfo;
extern String lastDisplayedDescription;
extern String lastDisplayedLyrics;
extern String lastDisplayedLog;

// Windows and Icons
extern UIWindow radioWindow;
extern UIWindow stationWindow;
extern UIWindow addStationWindow;
extern UIWindow confirmDeleteWindow;
extern UIWindow settingsWindow;
extern UIWindow wifiWindow;
extern DesktopIcon radioIcon;
extern DesktopIcon settingsIcon;
extern UIComponent* globalKeyboard;
extern UIComponent* wifiKeyboard;
extern int stationToDeleteIndex;
extern bool isEditMode;
extern int stationToEditIndex;

// Helper Functions
UIComponent* findComponentById(const UIWindow& window, int id);
void updateComponentSymbol(const UIWindow& window, int componentId, SymbolType newSymbol);

#endif
