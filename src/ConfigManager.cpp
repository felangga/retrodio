/*
 * ConfigManager.cpp - Configuration file manager implementation
 *
 * Copyright (c) 2025 felangga
 */

#include "ConfigManager.h"

// Static member initialization
int ConfigManager::volume = 5;
LastStation ConfigManager::lastStation = {"", ""};
Station ConfigManager::stations[MAX_STATIONS];
int ConfigManager::stationCount = 0;

/**
 * Initialize the file system and load configurations
 */
bool ConfigManager::begin() {
  // Initialize LittleFS
  if (!LittleFS.begin(true)) {  // true = format on failure
    return false;
  }

  // Load settings and stations
  if (!loadSettings()) {
    createDefaultSettings();
  }

  // if (!loadStations()) {
    createDefaultStations();
  // }

  return true;
}

/**
 * Load settings from file
 */
bool ConfigManager::loadSettings() {
  File file = LittleFS.open(SETTINGS_FILE, "r");
  if (!file) {
    return false;
  }

  // Parse JSON
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    return false;
  }

  // Read settings
  volume = doc["volume"] | 5;
  lastStation.name = doc["lastStationName"] | "";
  lastStation.url = doc["lastStationURL"] | "";

  return true;
}

/**
 * Save settings to file
 */
bool ConfigManager::saveSettings() {
  // Create JSON document
  JsonDocument doc;
  doc["volume"] = volume;
  doc["lastStationName"] = lastStation.name;
  doc["lastStationURL"] = lastStation.url;

  // Write to file
  File file = LittleFS.open(SETTINGS_FILE, "w");
  if (!file) {
    return false;
  }

  serializeJson(doc, file);
  file.close();

  return true;
}

/**
 * Load stations from file
 */
bool ConfigManager::loadStations() {
  File file = LittleFS.open(STATIONS_FILE, "r");
  if (!file) {
    return false;
  }

  // Parse JSON
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    return false;
  }

  // Read stations array
  JsonArray stationsArray = doc["stations"];
  stationCount = 0;

  for (JsonObject station : stationsArray) {
    if (stationCount >= MAX_STATIONS)
      break;

    stations[stationCount].name = station["name"] | "";
    stations[stationCount].url = station["url"] | "";
    stationCount++;
  }

  return true;
}

/**
 * Save stations to file
 */
bool ConfigManager::saveStations() {
  // Create JSON document
  JsonDocument doc;
  JsonArray stationsArray = doc["stations"].to<JsonArray>();

  for (int i = 0; i < stationCount; i++) {
    JsonObject station = stationsArray.add<JsonObject>();
    station["name"] = stations[i].name;
    station["url"] = stations[i].url;
  }

  // Write to file
  File file = LittleFS.open(STATIONS_FILE, "w");
  if (!file) {
    return false;
  }

  serializeJson(doc, file);
  file.close();

  return true;
}

/**
 * Get current volume setting
 */
int ConfigManager::getVolume() {
  return volume;
}

/**
 * Set volume and save
 */
void ConfigManager::setVolume(int vol) {
  volume = constrain(vol, 0, 21);
  saveSettings();
}

/**
 * Get last played station
 */
LastStation ConfigManager::getLastStation() {
  return lastStation;
}

/**
 * Set last played station and save
 */
void ConfigManager::setLastStation(const LastStation& station) {
  lastStation = station;
  saveSettings();
}

/**
 * Get number of stations
 */
int ConfigManager::getStationCount() {
  return stationCount;
}

/**
 * Get station by index
 */
Station ConfigManager::getStation(int index) {
  if (index >= 0 && index < stationCount) {
    return stations[index];
  }
  return {"", ""};
}

/**
 * Add a new station
 */
bool ConfigManager::addStation(const String& name, const String& url) {
  if (stationCount >= MAX_STATIONS) {
    return false;
  }

  stations[stationCount].name = name;
  stations[stationCount].url = url;
  stationCount++;

  return saveStations();
}

/**
 * Remove station by index
 */
bool ConfigManager::removeStation(int index) {
  if (index < 0 || index >= stationCount) {
    return false;
  }

  // Shift stations down
  for (int i = index; i < stationCount - 1; i++) {
    stations[i] = stations[i + 1];
  }
  stationCount--;

  return saveStations();
}

/**
 * Update station by index
 */
bool ConfigManager::updateStation(int index, const String& name, const String& url) {
  if (index < 0 || index >= stationCount) {
    return false;
  }

  stations[index].name = name;
  stations[index].url = url;

  return saveStations();
}

/**
 * Clear all stations
 */
void ConfigManager::clearStations() {
  stationCount = 0;
  saveStations();
}

/**
 * Reset to factory defaults
 */
void ConfigManager::factoryReset() {
  createDefaultSettings();
  createDefaultStations();
}

/**
 * Create default settings file
 */
bool ConfigManager::createDefaultSettings() {
  volume = 5;
  lastStation = {"", ""};
  return saveSettings();
}

/**
 * Create default stations list
 */
bool ConfigManager::createDefaultStations() {
  stationCount = 0;

  addStation("Swaragama FM - Jogja", "http://202.65.114.229:9314/");
  addStation("Geronimo FM - Jogja", "https://ig.idstreamer.com:8090/live");
  addStation("Prambors FM - Jakarta", "https://s2.cloudmu.id/listen/prambors/radio.aac");
  addStation("Trax FM - Jakarta",
             "https://n09.radiojar.com/rrqf78p3bnzuv?rj-ttl=5&rj-tok=AAABm15hIEIAih4Fwo4sru3EjQ");
  addStation("KISS.FM", "http://topradio-stream31.radiohost.de/kissfm_mp3-128");
  addStation("Paradise FM", "http://stream-uk1.radioparadise.com/aac-320");
  addStation("Costa FM", "http://radio4.cdm-radio.com:8020/stream-mp3-Chill_autodj");
  addStation("iRadio FM",
             "https://n09.radiojar.com/4ywdgup3bnzuv?rj-ttl=5&rj-tok=AAABm2VAjqYAZAvMAKsAkFVYDg");
  addStation("Sonora FM", "https://cast3.asurahosting.com/proxy/radios28/stream");
  addStation("BBC World Service",
             "https://radio.garden/api/ara/content/listen/FXyhz9Xk/channel.mp3?1766930004566");
  addStation("TEST",
             "https://streamcdnb4-dd782ed59e2a4e86aabf6fc508674b59.msvdn.net/live/S97044836/tbbP8T1ZRPBL/playlist_audio.m3u8");
  return true;
}

/**
 * Print debug information
 */
void ConfigManager::printDebugInfo() {
  // Note: Serial prints removed as per user request
  // This function is kept for future debugging via display
}
