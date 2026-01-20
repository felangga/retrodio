/*
 * ConfigManager.cpp - Configuration file manager implementation
 *
 * Copyright (c) 2025 felangga
 */

#include "ConfigManager.h"

int ConfigManager::volume = 5;
LastStation ConfigManager::lastStation = {"", ""};
String ConfigManager::wifiSSID = "";
String ConfigManager::wifiPassword = "";
Station ConfigManager::stations[MAX_STATIONS];
int ConfigManager::stationCount = 0;

bool ConfigManager::begin() {
  if (!LittleFS.begin(true)) {  // true = format on failure
    return false;
  }

  if (!loadSettings()) {
    createDefaultSettings();
  }

  if (!loadStations()) {
    createDefaultStations();
  }

  return true;
}

bool ConfigManager::loadSettings() {
  File file = LittleFS.open(SETTINGS_FILE, "r");
  if (!file) {
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    return false;
  }

  volume = doc["volume"] | 5;
  lastStation.name = doc["lastStationName"] | "";
  lastStation.url = doc["lastStationURL"] | "";
  wifiSSID = doc["wifiSSID"] | "";
  wifiPassword = doc["wifiPassword"] | "";

  return true;
}

bool ConfigManager::saveSettings() {
  JsonDocument doc;
  doc["volume"] = volume;
  doc["lastStationName"] = lastStation.name;
  doc["lastStationURL"] = lastStation.url;
  doc["wifiSSID"] = wifiSSID;
  doc["wifiPassword"] = wifiPassword;

  File file = LittleFS.open(SETTINGS_FILE, "w");
  if (!file) {
    return false;
  }

  serializeJson(doc, file);
  file.close();

  return true;
}

bool ConfigManager::loadStations() {
  File file = LittleFS.open(STATIONS_FILE, "r");
  if (!file) {
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    return false;
  }

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

bool ConfigManager::saveStations() {
  JsonDocument doc;
  JsonArray stationsArray = doc["stations"].to<JsonArray>();

  for (int i = 0; i < stationCount; i++) {
    JsonObject station = stationsArray.add<JsonObject>();
    station["name"] = stations[i].name;
    station["url"] = stations[i].url;
  }

  File file = LittleFS.open(STATIONS_FILE, "w");
  if (!file) {
    return false;
  }

  serializeJson(doc, file);
  file.close();

  return true;
}

int ConfigManager::getVolume() {
  return volume;
}

void ConfigManager::setVolume(int vol) {
  volume = constrain(vol, 0, 21);
  saveSettings();
}

LastStation ConfigManager::getLastStation() {
  return lastStation;
}

void ConfigManager::setLastStation(const LastStation& station) {
  lastStation = station;
  saveSettings();
}

int ConfigManager::getStationCount() {
  return stationCount;
}

Station ConfigManager::getStation(int index) {
  if (index >= 0 && index < stationCount) {
    return stations[index];
  }
  return {"", ""};
}

bool ConfigManager::addStation(const String& name, const String& url) {
  if (stationCount >= MAX_STATIONS) {
    return false;
  }

  stations[stationCount].name = name;
  stations[stationCount].url = url;
  stationCount++;

  return saveStations();
}

bool ConfigManager::removeStation(int index) {
  if (index < 0 || index >= stationCount) {
    return false;
  }

  for (int i = index; i < stationCount - 1; i++) {
    stations[i] = stations[i + 1];
  }
  stationCount--;

  return saveStations();
}

bool ConfigManager::updateStation(int index, const String& name, const String& url) {
  if (index < 0 || index >= stationCount) {
    return false;
  }

  stations[index].name = name;
  stations[index].url = url;

  return saveStations();
}

void ConfigManager::clearStations() {
  stationCount = 0;
  saveStations();
}

String ConfigManager::getWifiSSID() {
  return wifiSSID;
}

String ConfigManager::getWifiPassword() {
  return wifiPassword;
}

void ConfigManager::setWifiCredentials(const String& ssid, const String& password) {
  wifiSSID = ssid;
  wifiPassword = password;
  saveSettings();
}

void ConfigManager::factoryReset() {
  createDefaultSettings();
  createDefaultStations();
}

bool ConfigManager::createDefaultSettings() {
  volume = 5;
  lastStation = {"", ""};
  return saveSettings();
}

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
  return true;
}
