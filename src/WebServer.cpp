/*
 * WebServer.cpp - Web-based Remote Control Implementation
 *
 * Copyright (c) 2025 felangga
 *
 */

#include "WebServer.h"
#include <ArduinoJson.h>
#include "ConfigManager.h"
#include "GlobalState.h"
#include "StationManager.h"

#if ENABLE_SERIAL_DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#endif

WebServer server(80);
static bool webServerInitialized = false;

void initWebServer() {
  // Only initialize once
  if (webServerInitialized) {
    return;
  }

  // Define routes
  server.on("/", handleRoot);
  server.on("/api/status", handleGetStatus);
  server.on("/api/volume", HTTP_POST, handleSetVolume);
  server.on("/api/station", HTTP_POST, handleSelectStation);
  server.on("/api/playpause", HTTP_POST, handlePlayPause);
  server.on("/api/next", HTTP_POST, handleNext);
  server.on("/api/previous", HTTP_POST, handlePrevious);
  server.on("/api/stations", handleGetStations);
  server.on("/api/stations/add", HTTP_POST, handleAddStation);
  server.on("/api/stations/edit", HTTP_POST, handleEditStation);
  server.on("/api/stations/delete", HTTP_POST, handleDeleteStation);
  server.onNotFound(handleNotFound);

  server.begin();
  webServerInitialized = true;
}

void handleWebServer() {
  server.handleClient();
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Retrodio Remote</title>
  <style>
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
    }

    body {
      font-family: 'Chicago', 'Geneva', 'Monaco', monospace;
      background-color: #c0c0c0;
      background-image:
        linear-gradient(45deg, #999 25%, transparent 25%),
        linear-gradient(-45deg, #999 25%, transparent 25%),
        linear-gradient(45deg, transparent 75%, #999 75%),
        linear-gradient(-45deg, transparent 75%, #999 75%);
      background-size: 4px 4px;
      background-position: 0 0, 0 2px, 2px -2px, -2px 0px;
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
      padding: 20px;
    }

    .container {
      background: #fff;
      border: 2px solid #000;
      box-shadow: 4px 4px 0px rgba(0, 0, 0, 0.5);
      max-width: 500px;
      width: 100%;
    }

    .header {
      background: #000;
      color: #fff;
      padding: 8px;
      border-bottom: 2px solid #000;
      position: relative;
      display: flex;
      align-items: center;
      gap: 12px;
    }

    .header-info {
      flex: 1;
      min-width: 0;
    }

    .header h1 {
      font-size: 12px;
      font-weight: bold;
      margin-bottom: 4px;
      text-align: left;
      letter-spacing: 1px;
    }

    .header .station-name {
      font-size: 11px;
      margin-bottom: 2px;
      text-align: left;
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
    }

    .header .track-info {
      font-size: 10px;
      text-align: left;
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
    }

    .content {
      padding: 16px;
      background: #fff;
    }

    .controls {
      display: flex;
      gap: 4px;
      align-items: center;
    }

    .btn {
      background: #fff;
      color: #000;
      border: 2px solid #000;
      box-shadow: inset -1px -1px 0px #808080, inset 1px 1px 0px #dfdfdf;
      width: 50px;
      height: 50px;
      font-size: 18px;
      cursor: pointer;
      font-family: 'Chicago', monospace;
      font-weight: bold;
      border-radius: 50%;
    }

    .btn:hover {
      background: #f0f0f0;
    }

    .btn:active {
      box-shadow: inset 1px 1px 0px #808080, inset -1px -1px 0px #dfdfdf;
      background: #d0d0d0;
    }

    .btn.play-pause {
      width: 64px;
      height: 64px;
      font-size: 24px;
      border-radius: 50%;
    }

    .volume-control {
      margin-bottom: 16px;
      padding: 8px;
      border: 2px solid #000;
      background: #fff;
    }

    .volume-label {
      display: flex;
      justify-content: space-between;
      margin-bottom: 8px;
      color: #000;
      font-size: 11px;
      font-weight: bold;
    }

    .volume-slider {
      width: 100%;
      height: 20px;
      background: #fff;
      outline: none;
      -webkit-appearance: none;
      border: 2px solid #000;
      box-shadow: inset 1px 1px 0px #808080;
    }

    .volume-slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 16px;
      height: 16px;
      background: #000;
      cursor: pointer;
      border: 1px solid #fff;
    }

    .volume-slider::-moz-range-thumb {
      width: 16px;
      height: 16px;
      background: #000;
      cursor: pointer;
      border: 1px solid #fff;
    }

    .stations {
      max-height: 280px;
      overflow-y: auto;
      border: 2px solid #000;
      background: #fff;
    }

    .station-item {
      padding: 6px 8px;
      border-bottom: 1px solid #000;
      cursor: pointer;
      display: flex;
      align-items: center;
      gap: 8px;
      background: #fff;
      font-size: 11px;
    }

    .station-item:hover {
      background: #e0e0e0;
    }

    .station-item.active {
      background: #000;
      color: #fff;
    }

    .station-item.active .action-btn {
      background: #000;
      color: #fff;
      border-color: #fff;
    }

    .station-item .icon {
      font-size: 12px;
      width: 16px;
      text-align: center;
    }

    .station-item .name {
      flex: 1;
      font-weight: bold;
    }

    .station-item .actions {
      display: flex;
      gap: 4px;
    }

    .station-item .action-btn {
      background: #fff;
      border: 1px solid #000;
      font-size: 10px;
      cursor: pointer;
      padding: 2px 4px;
      font-family: 'Chicago', monospace;
    }

    .station-item .action-btn:hover {
      background: #d0d0d0;
    }

    .station-item .action-btn:active {
      background: #a0a0a0;
    }

    .add-station-btn {
      width: 100%;
      padding: 8px;
      background: #fff;
      color: #000;
      border: 2px solid #000;
      box-shadow: inset -1px -1px 0px #808080, inset 1px 1px 0px #dfdfdf;
      font-size: 11px;
      font-weight: bold;
      cursor: pointer;
      margin-bottom: 12px;
      font-family: 'Chicago', monospace;
    }

    .add-station-btn:hover {
      background: #f0f0f0;
    }

    .add-station-btn:active {
      box-shadow: inset 1px 1px 0px #808080, inset -1px -1px 0px #dfdfdf;
      background: #d0d0d0;
    }

    .status {
      text-align: center;
      padding: 12px;
      color: #000;
      font-size: 11px;
      font-style: italic;
    }

    .modal {
      display: none;
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background: rgba(192, 192, 192, 0.8);
      z-index: 1000;
      align-items: center;
      justify-content: center;
    }

    .modal.active {
      display: flex;
    }

    .modal-content {
      background: #fff;
      border: 2px solid #000;
      box-shadow: 4px 4px 0px rgba(0, 0, 0, 0.5);
      padding: 16px;
      max-width: 400px;
      width: 90%;
    }

    .modal-header {
      font-size: 12px;
      font-weight: bold;
      margin-bottom: 16px;
      color: #000;
      padding-bottom: 8px;
      border-bottom: 2px solid #000;
    }

    .form-group {
      margin-bottom: 12px;
    }

    .form-group label {
      display: block;
      margin-bottom: 4px;
      color: #000;
      font-size: 11px;
      font-weight: bold;
    }

    .form-group input {
      width: 100%;
      padding: 6px;
      border: 2px solid #000;
      box-shadow: inset 1px 1px 0px #808080;
      font-size: 11px;
      font-family: 'Chicago', monospace;
      background: #fff;
    }

    .form-group input:focus {
      outline: 2px solid #000;
      outline-offset: -4px;
    }

    .modal-buttons {
      display: flex;
      gap: 8px;
      justify-content: flex-end;
      margin-top: 16px;
      padding-top: 12px;
      border-top: 1px solid #000;
    }

    .modal-btn {
      padding: 6px 16px;
      border: 2px solid #000;
      font-size: 11px;
      font-weight: bold;
      cursor: pointer;
      font-family: 'Chicago', monospace;
      box-shadow: inset -1px -1px 0px #808080, inset 1px 1px 0px #dfdfdf;
      background: #fff;
    }

    .modal-btn.primary {
      background: #fff;
      color: #000;
    }

    .modal-btn.primary:hover {
      background: #f0f0f0;
    }

    .modal-btn.primary:active {
      box-shadow: inset 1px 1px 0px #808080, inset -1px -1px 0px #dfdfdf;
      background: #d0d0d0;
    }

    .modal-btn.secondary {
      background: #fff;
      color: #000;
    }

    .modal-btn.secondary:hover {
      background: #f0f0f0;
    }

    .modal-btn.secondary:active {
      box-shadow: inset 1px 1px 0px #808080, inset -1px -1px 0px #dfdfdf;
      background: #d0d0d0;
    }

    .playing-indicator {
      display: inline-block;
      width: 8px;
      height: 8px;
      background: #4caf50;
      border-radius: 50%;
      margin-right: 5px;
      animation: pulse 1.5s infinite;
    }

    @keyframes pulse {
      0%, 100% { opacity: 1; }
      50% { opacity: 0.3; }
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <div class="header-info">
        <h1>📻 Retrodio</h1>
        <div class="station-name" id="currentStationName">Loading...</div>
        <div class="track-info" id="currentTrackInfo">...</div>
      </div>
      <div class="controls">
        <button class="btn" onclick="previous()">⏮</button>
        <button class="btn play-pause" id="playPauseBtn" onclick="playPause()">▶</button>
        <button class="btn" onclick="next()">⏭</button>
      </div>
    </div>

    <div class="content">

      <div class="volume-control">
        <div class="volume-label">
          <span>🔊 Volume</span>
          <span id="volumeValue">50%</span>
        </div>
        <input type="range" min="0" max="21" value="10" class="volume-slider" id="volumeSlider" oninput="updateVolume(this.value)">
      </div>

      <button class="add-station-btn" onclick="showAddStationModal()">➕ Add Station</button>

      <div class="stations" id="stationsList">
        <div class="status">Loading stations...</div>
      </div>
    </div>
  </div>

  <div class="modal" id="stationModal">
    <div class="modal-content">
      <div class="modal-header" id="modalTitle">Add Station</div>
      <div class="form-group">
        <label for="inputStationName">Station Name</label>
        <input type="text" id="inputStationName" placeholder="e.g., BBC Radio 1">
      </div>
      <div class="form-group">
        <label for="inputStationUrl">Stream URL</label>
        <input type="text" id="inputStationUrl" placeholder="e.g., http://stream.example.com/radio">
      </div>
      <div class="modal-buttons">
        <button class="modal-btn secondary" onclick="closeModal()">Cancel</button>
        <button class="modal-btn primary" id="saveBtn" onclick="saveStation()">Save</button>
      </div>
    </div>
  </div>

  <script>
    let currentStationIndex = -1;
    let isPlaying = false;
    let editingStationIndex = -1;

    async function fetchStatus() {
      try {
        const response = await fetch('/api/status');
        const data = await response.json();

        document.getElementById('currentStationName').textContent = data.stationName || 'Retrodio';
        document.getElementById('currentTrackInfo').textContent = data.trackInfo || 'Standby';
        document.getElementById('volumeSlider').value = data.volume || 10;
        document.getElementById('volumeValue').textContent = Math.round((data.volume / 21) * 100) + '%';
        isPlaying = data.isPlaying;
        currentStationIndex = data.currentStationIndex;

        updatePlayPauseButton();
      } catch (error) {
        console.error('Error fetching status:', error);
      }
    }

    async function fetchStations() {
      try {
        const response = await fetch('/api/stations');
        const data = await response.json();

        const stationsList = document.getElementById('stationsList');
        if (data.stations && data.stations.length > 0) {
          stationsList.innerHTML = data.stations.map((station, index) => `
            <div class="station-item ${index === currentStationIndex ? 'active' : ''}">
              <span class="icon" onclick="selectStation(${index})">${index === currentStationIndex && isPlaying ? '▶' : '📻'}</span>
              <span class="name" onclick="selectStation(${index})">${station.name}</span>
              <div class="actions">
                <button class="action-btn" onclick="event.stopPropagation(); editStation(${index}, '${station.name.replace(/'/g, "\\'")}', '${station.url.replace(/'/g, "\\'")}')">✏️</button>
                <button class="action-btn" onclick="event.stopPropagation(); deleteStation(${index}, '${station.name.replace(/'/g, "\\'")}')">🗑️</button>
              </div>
            </div>
          `).join('');
        } else {
          stationsList.innerHTML = '<div class="status">No stations available</div>';
        }
      } catch (error) {
        console.error('Error fetching stations:', error);
      }
    }

    function updatePlayPauseButton() {
      const btn = document.getElementById('playPauseBtn');
      btn.textContent = isPlaying ? '⏸' : '▶';
    }

    async function updateVolume(value) {
      const percent = Math.round((value / 21) * 100);
      document.getElementById('volumeValue').textContent = percent + '%';

      try {
        await fetch('/api/volume', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ volume: parseInt(value) })
        });
      } catch (error) {
        console.error('Error updating volume:', error);
      }
    }

    async function selectStation(index) {
      try {
        await fetch('/api/station', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ index: index })
        });
        setTimeout(() => {
          fetchStatus();
          fetchStations();
        }, 100);
      } catch (error) {
        console.error('Error selecting station:', error);
      }
    }

    async function playPause() {
      try {
        await fetch('/api/playpause', { method: 'POST' });
        setTimeout(fetchStatus, 100);
      } catch (error) {
        console.error('Error toggling playback:', error);
      }
    }

    async function next() {
      try {
        await fetch('/api/next', { method: 'POST' });
        setTimeout(() => {
          fetchStatus();
          fetchStations();
        }, 100);
      } catch (error) {
        console.error('Error skipping to next:', error);
      }
    }

    async function previous() {
      try {
        await fetch('/api/previous', { method: 'POST' });
        setTimeout(() => {
          fetchStatus();
          fetchStations();
        }, 100);
      } catch (error) {
        console.error('Error skipping to previous:', error);
      }
    }

    function showAddStationModal() {
      editingStationIndex = -1;
      document.getElementById('modalTitle').textContent = 'Add Station';
      document.getElementById('inputStationName').value = '';
      document.getElementById('inputStationUrl').value = '';
      document.getElementById('stationModal').classList.add('active');
    }

    function editStation(index, name, url) {
      editingStationIndex = index;
      document.getElementById('modalTitle').textContent = 'Edit Station';
      document.getElementById('inputStationName').value = name;
      document.getElementById('inputStationUrl').value = url;
      document.getElementById('stationModal').classList.add('active');
    }

    function closeModal() {
      document.getElementById('stationModal').classList.remove('active');
      editingStationIndex = -1;
    }

    async function saveStation() {
      const name = document.getElementById('inputStationName').value.trim();
      const url = document.getElementById('inputStationUrl').value.trim();

      if (!name || !url) {
        alert('Please fill in both name and URL');
        return;
      }

      try {
        let response;
        if (editingStationIndex === -1) {
          // Add new station
          response = await fetch('/api/stations/add', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ name, url })
          });
        } else {
          // Edit existing station
          response = await fetch('/api/stations/edit', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ index: editingStationIndex, name, url })
          });
        }

        const result = await response.json();
        if (result.success) {
          closeModal();
          fetchStations();
          fetchStatus();
        } else {
          alert('Error: ' + (result.error || 'Failed to save station'));
        }
      } catch (error) {
        console.error('Error saving station:', error);
        alert('Error saving station');
      }
    }

    async function deleteStation(index, name) {
      if (!confirm(`Delete station "${name}"?`)) {
        return;
      }

      try {
        const response = await fetch('/api/stations/delete', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ index })
        });

        const result = await response.json();
        if (result.success) {
          fetchStations();
          fetchStatus();
        } else {
          alert('Error: ' + (result.error || 'Failed to delete station'));
        }
      } catch (error) {
        console.error('Error deleting station:', error);
        alert('Error deleting station');
      }
    }

    // Close modal when clicking outside
    document.getElementById('stationModal').addEventListener('click', (e) => {
      if (e.target.id === 'stationModal') {
        closeModal();
      }
    });

    // Initialize
    fetchStatus();
    fetchStations();

    // Update status every 2 seconds
    setInterval(() => {
      fetchStatus();
      fetchStations();
    }, 2000);
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleGetStatus() {
  extern String currentStationName;
  extern volatile bool isPlaying;
  extern int currentStationIndex;
  extern String lastTrackInfo;

  JsonDocument doc;
  doc["stationName"] = currentStationName;
  doc["trackInfo"] = lastTrackInfo;
  doc["volume"] = ConfigManager::getVolume();
  doc["isPlaying"] = isPlaying;
  doc["currentStationIndex"] = currentStationIndex;

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleSetVolume() {
  if (server.hasArg("plain")) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (!error && doc.containsKey("volume")) {
      int volume = doc["volume"];

      if (volume >= 0 && volume <= 21) {
        extern Audio audio;
        extern MacWindow radioWindow;
        extern const int CMP_VOLUME_SLIDER;

        // Set volume in audio system and save to config
        audio.setVolume(volume);
        ConfigManager::setVolume(volume);

        // Update the volume slider value and request redraw
        MacComponent* volumeSlider = findComponentById(radioWindow, CMP_VOLUME_SLIDER);
        if (volumeSlider != nullptr && volumeSlider->customData != nullptr) {
          MacSlider* slider = (MacSlider*)volumeSlider->customData;
          slider->currentValue = volume;

          // Set flag to request UI task to redraw the slider
          extern volatile bool needsVolumeSliderRedraw;
          needsVolumeSliderRedraw = true;
        }

        server.send(200, "application/json", "{\"success\":true}");
        return;
      }
    }
  }

  server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid volume\"}");
}

void handleSelectStation() {
  if (server.hasArg("plain")) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (!error && doc.containsKey("index")) {
      int index = doc["index"];
      int stationCount = ConfigManager::getStationCount();

      if (index >= 0 && index < stationCount) {
        switchToStation(index);
        server.send(200, "application/json", "{\"success\":true}");
        return;
      }
    }
  }

  server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid station index\"}");
}

void handlePlayPause() {
  extern Audio audio;
  extern QueueHandle_t audioCommandQueue;
  extern volatile bool isPlaying;
  extern String RadioURL;

  if (isPlaying) {
    AudioCommandMsg msg = {CMD_STOP, ""};
    xQueueSend(audioCommandQueue, &msg, portMAX_DELAY);
    isPlaying = false;

    extern MacWindow radioWindow;
    updateComponentSymbol(radioWindow, 1, SYMBOL_PLAY);
  } else {
    if (RadioURL.length() > 0) {
      AudioCommandMsg msg = {CMD_CONNECT, ""};
      strncpy(msg.url, RadioURL.c_str(), sizeof(msg.url) - 1);
      msg.url[sizeof(msg.url) - 1] = '\0';
      xQueueSend(audioCommandQueue, &msg, portMAX_DELAY);
      isPlaying = true;

      extern MacWindow radioWindow;
      updateComponentSymbol(radioWindow, 1, SYMBOL_PAUSE);
    }
  }

  server.send(200, "application/json", "{\"success\":true}");
}

void handleNext() {
  int stationCount = ConfigManager::getStationCount();

  if (stationCount == 0) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"No stations\"}");
    return;
  }

  extern int currentStationIndex;
  int nextIndex;

  if (currentStationIndex < 0 || currentStationIndex >= stationCount - 1) {
    nextIndex = 0;
  } else {
    nextIndex = currentStationIndex + 1;
  }

  switchToStation(nextIndex);
  server.send(200, "application/json", "{\"success\":true}");
}

void handlePrevious() {
  int stationCount = ConfigManager::getStationCount();

  if (stationCount == 0) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"No stations\"}");
    return;
  }

  extern int currentStationIndex;
  int prevIndex;

  if (currentStationIndex <= 0) {
    prevIndex = stationCount - 1;
  } else {
    prevIndex = currentStationIndex - 1;
  }

  switchToStation(prevIndex);
  server.send(200, "application/json", "{\"success\":true}");
}

void handleGetStations() {
  JsonDocument doc;
  JsonArray stations = doc["stations"].to<JsonArray>();

  int stationCount = ConfigManager::getStationCount();
  for (int i = 0; i < stationCount; i++) {
    Station station = ConfigManager::getStation(i);
    JsonObject stationObj = stations.add<JsonObject>();
    stationObj["name"] = station.name;
    stationObj["url"] = station.url;
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleAddStation() {
  if (server.hasArg("plain")) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (!error && doc.containsKey("name") && doc.containsKey("url")) {
      String name = doc["name"].as<String>();
      String url = doc["url"].as<String>();

      if (name.length() > 0 && url.length() > 0) {
        if (ConfigManager::addStation(name, url)) {
          // Request UI task to reload station list
          extern volatile bool needsStationListReload;
          needsStationListReload = true;

          server.send(200, "application/json", "{\"success\":true}");
          return;
        } else {
          server.send(400, "application/json",
                      "{\"success\":false,\"error\":\"Failed to add station (maximum reached or "
                      "storage error)\"}");
          return;
        }
      }
    }
  }

  server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid station data\"}");
}

void handleEditStation() {
  if (server.hasArg("plain")) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (!error && doc.containsKey("index") && doc.containsKey("name") && doc.containsKey("url")) {
      int index = doc["index"];
      String name = doc["name"].as<String>();
      String url = doc["url"].as<String>();

      int stationCount = ConfigManager::getStationCount();

      if (index >= 0 && index < stationCount && name.length() > 0 && url.length() > 0) {
        if (ConfigManager::updateStation(index, name, url)) {
          // Request UI task to reload station list
          extern volatile bool needsStationListReload;
          needsStationListReload = true;

          // If editing the current station, update the URL
          extern int currentStationIndex;
          extern String RadioURL;
          extern String currentStationName;

          if (index == currentStationIndex) {
            RadioURL = url;
            currentStationName = name;
          }

          server.send(200, "application/json", "{\"success\":true}");
          return;
        } else {
          server.send(400, "application/json",
                      "{\"success\":false,\"error\":\"Failed to update station\"}");
          return;
        }
      }
    }
  }

  server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid station data\"}");
}

void handleDeleteStation() {
  if (server.hasArg("plain")) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (!error && doc.containsKey("index")) {
      int index = doc["index"];
      int stationCount = ConfigManager::getStationCount();

      if (index >= 0 && index < stationCount) {
        extern int currentStationIndex;

        // Check if we're deleting the current station
        bool isDeletingCurrent = (index == currentStationIndex);

        if (ConfigManager::removeStation(index)) {
          // Request UI task to reload station list
          extern volatile bool needsStationListReload;
          needsStationListReload = true;

          // Adjust current station index if needed
          if (isDeletingCurrent) {
            extern volatile bool isPlaying;
            extern QueueHandle_t audioCommandQueue;

            // Stop playback if deleting current station
            if (isPlaying) {
              AudioCommandMsg msg = {CMD_STOP, ""};
              xQueueSend(audioCommandQueue, &msg, portMAX_DELAY);
              isPlaying = false;

              extern MacWindow radioWindow;
              updateComponentSymbol(radioWindow, 1, SYMBOL_PLAY);
            }

            currentStationIndex = -1;
          } else if (index < currentStationIndex) {
            // Shift index down if we deleted a station before the current one
            currentStationIndex--;
          }

          server.send(200, "application/json", "{\"success\":true}");
          return;
        } else {
          server.send(400, "application/json",
                      "{\"success\":false,\"error\":\"Failed to delete station\"}");
          return;
        }
      }
    }
  }

  server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid station index\"}");
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}
