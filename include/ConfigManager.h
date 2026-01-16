/*
 * ConfigManager.h - Configuration file manager for Retrodio
 *
 * Copyright (c) 2025 felangga
 *
 * Manages persistent storage of settings and station list using LittleFS
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

// Maximum stations supported
#define MAX_STATIONS 30  // Reduced from 50 to save ~10KB RAM

// Configuration file paths
#define SETTINGS_FILE "/settings.json"
#define STATIONS_FILE "/stations.json"

// Station data structure
struct Station {
  String name;
  String url;
};

// Last station data structure (same as Station)
typedef Station LastStation;

class ConfigManager {
 public:
  // Initialize the configuration manager
  static bool begin();

  // Settings management
  static bool loadSettings();
  static bool saveSettings();
  static int getVolume();
  static void setVolume(int volume);
  static LastStation getLastStation();
  static void setLastStation(const LastStation& station);

  // Station list management
  static bool loadStations();
  static bool saveStations();
  static int getStationCount();
  static Station getStation(int index);
  static bool addStation(const String& name, const String& url);
  static bool removeStation(int index);
  static bool updateStation(int index, const String& name, const String& url);
  static void clearStations();

  // Utility functions
  static void factoryReset();
  static void printDebugInfo();

 private:
  // Current settings
  static int volume;
  static LastStation lastStation;

  // Station list
  static Station stations[MAX_STATIONS];
  static int stationCount;

  // Helper functions
  static bool createDefaultSettings();
  static bool createDefaultStations();
};

#endif  // CONFIG_MANAGER_H
