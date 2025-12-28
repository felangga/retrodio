# PlatformIO Setup Guide

This project has been migrated to support both Arduino IDE and PlatformIO. This document explains how to build and upload using PlatformIO.

## Prerequisites

You need to have PlatformIO installed. Choose one of the following methods:

### Option 1: VSCode Extension (Recommended)

1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Install the PlatformIO IDE extension from VSCode marketplace
3. Restart VSCode

### Option 2: PlatformIO Core CLI

```bash
# Install using pip
pip install -U platformio

# Or using Homebrew (macOS)
brew install platformio
```

## Project Structure

```
retrodio/
├── platformio.ini              # PlatformIO configuration
├── partition_16MB_ota_spiffs.csv
├── src/
│   ├── main.cpp               # Main application (was retrodio.ino)
│   └── ConfigManager.cpp      # Configuration management
├── include/
│   ├── ConfigManager.h
│   ├── config.h
│   └── wt32_sc01_plus.h       # Display hardware config
└── lib/
    └── MacUI/                 # MacUI library (custom UI components)
        ├── library.json       # Library metadata
        └── src/
            ├── MacUI.h        # Main header
            ├── MacUI.cpp      # Core UI functions
            ├── wt32_sc01_plus.h  # Hardware config (copy)
            ├── MacButton.cpp
            ├── MacCheckBox.cpp
            ├── MacInputField.cpp
            ├── MacKeyboard.cpp
            ├── MacLabel.cpp
            ├── MacListView.cpp
            ├── MacProgressBar.cpp
            ├── MacRunningText.cpp
            ├── MacSlider.cpp
            └── MacTextBox.cpp
```

## Building the Project

### Using VSCode with PlatformIO Extension

1. Open the project folder in VSCode
2. PlatformIO will automatically detect the `platformio.ini` file
3. Click the Build icon (checkmark) in the status bar, or use:
   - Menu: PlatformIO > Build
   - Shortcut: `Cmd+Alt+B` (macOS) / `Ctrl+Alt+B` (Windows/Linux)

### Using PlatformIO CLI

```bash
# Navigate to project directory
cd /path/to/retrodio

# Build the project
pio run

# Upload to device
pio run --target upload

# Open serial monitor
pio device monitor

# Build, upload, and monitor in one command
pio run --target upload && pio device monitor
```

## Configuration Details

### Board Configuration

The `platformio.ini` file is configured for ESP32-S3 with:

- **Platform**: Espressif32 (ESP32)
- **Board**: ESP32-S3 DevKit C-1
- **CPU Frequency**: 240 MHz
- **Flash Mode**: QIO at 80 MHz
- **Partition Scheme**: Custom 3MB APP partition
- **Upload Speed**: 921600 baud

### Partition Scheme

The custom partition table allocates:

- **NVS**: 20 KB (Non-volatile storage)
- **OTA Data**: 8 KB (OTA update metadata)
- **APP**: 3 MB (Application firmware - HUGE APP)
- **SPIFFS**: 960 KB (File system for station data)

This is critical because the firmware is ~1.6 MB and won't fit in the default 1.3 MB partition.

### Library Dependencies

The following libraries are automatically installed by PlatformIO:

1. **ESP32-audioI2S** - Audio streaming library
2. **LovyanGFX** (^1.2.7) - Graphics library for displays
3. **ArduinoJson** (^7.4.2) - JSON parsing for configuration
4. **LVGL** (^8.3.3) - Graphics library
5. **MacUI** (local) - Custom classic Macintosh-style UI library

## Troubleshooting

### Serial Port Not Found

```bash
# List available serial ports
pio device list

# Update platformio.ini with your port
# Add this line under [env:esp32s3]:
upload_port = /dev/tty.usbmodem1101  # macOS/Linux
# or
upload_port = COM3                    # Windows
```

### Build Fails with Memory Errors

Ensure you're using the custom partition table:
```ini
board_build.partitions = partition_16MB_ota_spiffs.csv
```

### Library Not Found

```bash
# Force library dependency reinstall
pio pkg install --force

# Update all libraries
pio pkg update
```

### Upload Fails

1. Make sure the ESP32-S3 is connected
2. Press and hold the BOOT button while uploading
3. Check upload_port is correct in platformio.ini

## Monitoring Serial Output

```bash
# Using PlatformIO
pio device monitor

# Or specify baud rate
pio device monitor -b 115200
```

In VSCode, click the Serial Monitor icon (plug) in the status bar.

## Switching Between Arduino IDE and PlatformIO

Both build systems can coexist:

- **Arduino IDE**: Uses `retrodio.ino` in the root directory
- **PlatformIO**: Uses `src/main.cpp` (which is a copy of retrodio.ino)

If you modify files, make sure to sync changes between both locations, or choose one build system to avoid confusion.

## Advanced Configuration

### Changing Board or Partition

Edit `platformio.ini`:

```ini
# For different ESP32-S3 boards
board = esp32-s3-devkitm-1

# For different partition schemes
board_build.partitions = default.csv  # Use Arduino default
```

### Adding Build Flags

```ini
build_flags =
    -DCORE_DEBUG_LEVEL=5  # Enable verbose debug
    -DCONFIG_ARDUHAL_LOG_COLORS=1
```

### Custom Upload Settings

```ini
upload_speed = 460800     # Slower but more reliable
upload_port = /dev/ttyUSB0
```

## Features

### MacUI Library

The custom MacUI library provides classic Macintosh-style UI components:

- **Windows**: Draggable windows with title bars, minimize/close buttons
- **Buttons**: Standard and symbol buttons (play, pause, stop, etc.)
- **Labels**: Text display with customizable colors and sizes
- **Running Text**: Auto-scrolling text with pause/resume
- **List Views**: Scrollable lists with touch support
- **Input Fields**: Text input with cursor support
- **Checkboxes**: Toggle controls
- **Sliders**: Value adjustment controls
- **Progress Bars**: Visual progress indicators
- **Keyboard**: On-screen keyboard for text input

### Real-time Monitoring

- **CPU Usage**: Displays real-time CPU usage for both ESP32 cores
  - Core 0: Audio streaming task
  - Core 1: UI rendering task
- **Clock**: Real-time clock display synchronized via NTP
- **Station Metadata**: Shows current station name and track info

### Multi-Core Architecture

- **Core 0**: Handles audio streaming and decoding
- **Core 1**: Manages UI rendering and user interaction
- **Thread-Safe**: Mutex-protected communication between cores

## Next Steps

1. Build the project: `pio run`
2. Upload to device: `pio run --target upload`
3. Monitor serial output: `pio device monitor`

For more information, visit [PlatformIO Documentation](https://docs.platformio.org/).
