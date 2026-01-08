/*
 * GlobalState.h - Shared Global Variables
 *
 * Copyright (c) 2025 felangga
 *
 * This header file contains declarations for global state variables
 */

#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include <Arduino.h>
#include "Audio.h"
#include "MacUI.h"
#include "wt32_sc01_plus.h"

// Component ID Constants
extern const int CMP_NOW_PLAYING_LABEL;
extern const int CMP_VOLUME_SLIDER;
extern const int CMP_BUFFER_PROGRESS;
extern const int CMP_AUTO_PLAY_CHECKBOX;
extern const int CMP_VISUALS_CHECKBOX;

extern const int BTN_PLAY;
extern const int BTN_STOP;
extern const int BTN_VOL_UP;
extern const int BTN_PREV;
extern const int BTN_NEXT;
extern const int BTN_VOL_DOWN;
extern const int BTN_STATION;
extern const int BTN_ADD_STATION;
extern const int BTN_SAVE_STATION;
extern const int BTN_CANCEL_ADD_STATION;
extern const int BTN_DELETE_STATION;
extern const int BTN_CONFIRM_YES;
extern const int BTN_CONFIRM_NO;

// Global Objects
extern LGFX lcd;
extern Audio audio;

// Task Handles
extern TaskHandle_t uiTaskHandle;
extern TaskHandle_t audioTaskHandle;
extern SemaphoreHandle_t metadataMutex;
extern QueueHandle_t audioCommandQueue;

// Audio Command Types
enum AudioCommand {
  CMD_NONE = 0,
  CMD_PLAY,
  CMD_STOP,
  CMD_CONNECT
};

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
extern MacWindow radioWindow;
extern MacWindow stationWindow;
extern MacWindow addStationWindow;
extern MacWindow confirmDeleteWindow;
extern DesktopIcon radioIcon;
extern MacComponent* globalKeyboard;
extern int stationToDeleteIndex;

// Helper Functions
MacComponent* findComponentById(const MacWindow& window, int id);
void updateComponentSymbol(const MacWindow& window, int componentId, SymbolType newSymbol);

#endif
