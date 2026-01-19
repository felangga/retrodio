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

WebServer server(80);

void initWebServer() {
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
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
      padding: 20px;
    }

    .container {
      background: white;
      border-radius: 20px;
      box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
      max-width: 500px;
      width: 100%;
      overflow: hidden;
    }

    .header {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      padding: 30px;
      text-align: center;
    }

    .header h1 {
      font-size: 28px;
      margin-bottom: 10px;
    }

    .header .station-name {
      font-size: 18px;
      opacity: 0.9;
      margin-bottom: 5px;
    }

    .header .track-info {
      font-size: 14px;
      opacity: 0.7;
    }

    .content {
      padding: 30px;
    }

    .controls {
      display: flex;
      justify-content: center;
      gap: 15px;
      margin-bottom: 30px;
    }

    .btn {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      border: none;
      border-radius: 50%;
      width: 60px;
      height: 60px;
      font-size: 24px;
      cursor: pointer;
      transition: all 0.3s;
      box-shadow: 0 4px 15px rgba(102, 126, 234, 0.4);
    }

    .btn:hover {
      transform: translateY(-2px);
      box-shadow: 0 6px 20px rgba(102, 126, 234, 0.6);
    }

    .btn:active {
      transform: translateY(0);
    }

    .btn.play-pause {
      width: 80px;
      height: 80px;
      font-size: 32px;
    }

    .volume-control {
      margin-bottom: 30px;
    }

    .volume-label {
      display: flex;
      justify-content: space-between;
      margin-bottom: 10px;
      color: #333;
      font-weight: 500;
    }

    .volume-slider {
      width: 100%;
      height: 8px;
      border-radius: 5px;
      background: #e0e0e0;
      outline: none;
      -webkit-appearance: none;
    }

    .volume-slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 24px;
      height: 24px;
      border-radius: 50%;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      cursor: pointer;
      box-shadow: 0 2px 10px rgba(102, 126, 234, 0.4);
    }

    .volume-slider::-moz-range-thumb {
      width: 24px;
      height: 24px;
      border-radius: 50%;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      cursor: pointer;
      border: none;
      box-shadow: 0 2px 10px rgba(102, 126, 234, 0.4);
    }

    .stations {
      max-height: 300px;
      overflow-y: auto;
    }

    .station-item {
      padding: 15px;
      border-bottom: 1px solid #f0f0f0;
      cursor: pointer;
      transition: background 0.2s;
      display: flex;
      align-items: center;
      gap: 10px;
    }

    .station-item:hover {
      background: #f8f8f8;
    }

    .station-item.active {
      background: linear-gradient(135deg, rgba(102, 126, 234, 0.1) 0%, rgba(118, 75, 162, 0.1) 100%);
      border-left: 4px solid #667eea;
    }

    .station-item .icon {
      font-size: 20px;
    }

    .station-item .name {
      flex: 1;
      font-weight: 500;
      color: #333;
    }

    .station-item .actions {
      display: flex;
      gap: 5px;
    }

    .station-item .action-btn {
      background: transparent;
      border: none;
      font-size: 18px;
      cursor: pointer;
      padding: 5px;
      opacity: 0.6;
      transition: opacity 0.2s;
    }

    .station-item .action-btn:hover {
      opacity: 1;
    }

    .add-station-btn {
      width: 100%;
      padding: 15px;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      border: none;
      border-radius: 10px;
      font-size: 16px;
      font-weight: 500;
      cursor: pointer;
      margin-bottom: 15px;
      transition: transform 0.2s;
    }

    .add-station-btn:hover {
      transform: translateY(-2px);
    }

    .status {
      text-align: center;
      padding: 10px;
      color: #666;
      font-size: 14px;
    }

    .modal {
      display: none;
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background: rgba(0, 0, 0, 0.5);
      z-index: 1000;
      align-items: center;
      justify-content: center;
    }

    .modal.active {
      display: flex;
    }

    .modal-content {
      background: white;
      border-radius: 15px;
      padding: 30px;
      max-width: 500px;
      width: 90%;
      box-shadow: 0 10px 40px rgba(0, 0, 0, 0.3);
    }

    .modal-header {
      font-size: 24px;
      font-weight: 600;
      margin-bottom: 20px;
      color: #333;
    }

    .form-group {
      margin-bottom: 20px;
    }

    .form-group label {
      display: block;
      margin-bottom: 8px;
      color: #555;
      font-weight: 500;
    }

    .form-group input {
      width: 100%;
      padding: 12px;
      border: 2px solid #e0e0e0;
      border-radius: 8px;
      font-size: 14px;
      transition: border-color 0.2s;
    }

    .form-group input:focus {
      outline: none;
      border-color: #667eea;
    }

    .modal-buttons {
      display: flex;
      gap: 10px;
      justify-content: flex-end;
      margin-top: 25px;
    }

    .modal-btn {
      padding: 12px 24px;
      border: none;
      border-radius: 8px;
      font-size: 14px;
      font-weight: 500;
      cursor: pointer;
      transition: all 0.2s;
    }

    .modal-btn.primary {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
    }

    .modal-btn.primary:hover {
      transform: translateY(-2px);
      box-shadow: 0 4px 15px rgba(102, 126, 234, 0.4);
    }

    .modal-btn.secondary {
      background: #f0f0f0;
      color: #555;
    }

    .modal-btn.secondary:hover {
      background: #e0e0e0;
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
      <h1>📻 Retrodio</h1>
      <div class="station-name" id="currentStationName">Loading...</div>
      <div class="track-info" id="currentTrackInfo">...</div>
    </div>

    <div class="content">
      <div class="controls">
        <button class="btn" onclick="previous()">⏮</button>
        <button class="btn play-pause" id="playPauseBtn" onclick="playPause()">▶</button>
        <button class="btn" onclick="next()">⏭</button>
      </div>

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

        audio.setVolume(volume);
        ConfigManager::setVolume(volume);

        // Update the volume slider component on the radio window
        MacComponent* volumeSlider = findComponentById(radioWindow, CMP_VOLUME_SLIDER);
        if (volumeSlider && volumeSlider->customData) {
          MacSlider* slider = (MacSlider*)volumeSlider->customData;
          slider->currentValue = volume;

          // Redraw the slider if the radio window is visible
          if (radioWindow.visible && !radioWindow.minimized) {
            extern LGFX lcd;
            drawComponent(lcd, *volumeSlider, radioWindow.x, radioWindow.y);
          }
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
          // Reload station list on the device
          reloadStationList();

          server.send(200, "application/json", "{\"success\":true}");
          return;
        } else {
          server.send(400, "application/json", "{\"success\":false,\"error\":\"Failed to add station (maximum reached or storage error)\"}");
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
          // Reload station list on the device
          reloadStationList();

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
          server.send(400, "application/json", "{\"success\":false,\"error\":\"Failed to update station\"}");
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
          // Reload station list on the device
          reloadStationList();

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
          server.send(400, "application/json", "{\"success\":false,\"error\":\"Failed to delete station\"}");
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
