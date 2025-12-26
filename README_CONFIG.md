# Configuration File System

## Overview

Retrodio uses LittleFS (Little File System) to store configuration persistently on the ESP32's flash memory. This allows settings and station lists to survive reboots and power cycles.

## Configuration Files

### 1. Settings File (`/settings.json`)

Stores application settings like volume and last played station.

**Format:**
```json
{
  "volume": 5,
  "lastStation": "Swaragama FM - Jogja"
}
```

**Fields:**
- `volume`: Audio volume level (0-21)
- `lastStation`: Name of the last played station

### 2. Stations File (`/stations.json`)

Stores the list of radio stations with their URLs.

**Format:**
```json
{
  "stations": [
    {
      "name": "Swaragama FM - Jogja",
      "url": "http://202.65.114.229:9314/"
    },
    {
      "name": "Geronimo FM - Jogja",
      "url": "https://ig.idstreamer.com:8090/live"
    }
  ]
}
```

**Fields:**
- `name`: Display name of the station
- `url`: Streaming URL (HTTP/HTTPS)

## ConfigManager API

### Initialization

```cpp
// Initialize file system and load configurations
if (!ConfigManager::begin()) {
  // Handle error
}
```

### Settings Management

```cpp
// Get/Set volume
int volume = ConfigManager::getVolume();
ConfigManager::setVolume(15);

// Get/Set last station
String lastStation = ConfigManager::getLastStation();
ConfigManager::setLastStation("Radio Name");
```

### Station Management

```cpp
// Get station count
int count = ConfigManager::getStationCount();

// Get station by index
Station station = ConfigManager::getStation(0);
// station.name -> "Swaragama FM - Jogja"
// station.url -> "http://202.65.114.229:9314/"

// Add new station
ConfigManager::addStation("New Radio", "http://stream.url");

// Remove station
ConfigManager::removeStation(2);  // Remove station at index 2

// Update existing station
ConfigManager::updateStation(0, "Updated Name", "http://new.url");

// Clear all stations
ConfigManager::clearStations();
```

### Utility Functions

```cpp
// Reset to factory defaults
ConfigManager::factoryReset();
```

## File System Management

### Viewing Files

You can view the configuration files using ESP32's LittleFS browser tools or upload tool.

### Manual Editing

You can manually create/edit configuration files and upload them via:
1. Arduino IDE → Tools → ESP32 Sketch Data Upload
2. Place JSON files in `data/` folder in your sketch directory

### Backup/Restore

To backup your configuration:
1. Read files from LittleFS
2. Save to your computer

To restore:
1. Upload JSON files back to LittleFS

## Storage Limits

- **Maximum Stations**: 50 (defined by `MAX_STATIONS`)
- **File System Size**: Depends on partition scheme
  - Minimal SPIFFS: 190KB
  - Default: 1.5MB
- **JSON Document Size**: Dynamically allocated

## Auto-Save Behavior

Settings are automatically saved when:
- Volume is changed (`onVolUp()`, `onVolDown()`)
- Station is selected for playback
- New station is added
- Station is removed or updated

## Factory Reset

The `factoryReset()` function:
1. Deletes all stations
2. Resets volume to default (5)
3. Clears last station
4. Creates default station list with 2 Indonesian stations

## Error Handling

ConfigManager handles errors gracefully:
- If file system initialization fails, it formats the partition
- If config files are missing, it creates defaults
- If JSON parsing fails, it recreates the file
- All operations return `bool` for success/failure checking

## Example Usage in Retrodio

```cpp
void setup() {
  // Initialize ConfigManager
  if (!ConfigManager::begin()) {
    displayStatus(lcd, "Config init failed!", 160);
  } else {
    // Load station list
    reloadStationList();

    // Restore last volume
    audio.setVolume(ConfigManager::getVolume());
  }
}

void onStationItemClick(int index) {
  // Get station from config
  Station station = ConfigManager::getStation(index);

  // Connect to stream
  audio.connecttohost(station.url.c_str());

  // Save as last played
  ConfigManager::setLastStation(station.name);
}
```

## Benefits

1. **Persistence**: Settings survive reboots
2. **User Customization**: Add unlimited stations (up to MAX_STATIONS)
3. **Easy Management**: Simple API for all operations
4. **Automatic Saving**: No manual save required
5. **Factory Reset**: Easy recovery from corruption
6. **JSON Format**: Human-readable and editable

## Technical Details

- **File System**: LittleFS (Little File System)
- **JSON Library**: ArduinoJson 7.x
- **Storage**: ESP32 Flash Memory
- **Format**: UTF-8 encoded JSON
- **Auto-format**: Formats partition on first use if needed
