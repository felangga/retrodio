# Retrodio - Internet Radio Player

A retro-styled internet radio player for ESP32 with a classic Macintosh OS-inspired user interface.

![Platform](https://img.shields.io/badge/platform-ESP32--S3-blue)
![Framework](https://img.shields.io/badge/framework-Arduino-00979D)
![License](https://img.shields.io/badge/license-MIT-green)

## Overview

Retrodio is an internet radio streaming application designed for the WT32-SC01 Plus development board featuring an ESP32-S3 chip and a 3.5" capacitive touchscreen. It features a nostalgic classic Macintosh-style UI with draggable windows, custom UI components, and real-time system monitoring.

### Key Features

- 🎵 **Internet Radio Streaming**: Stream radio stations over WiFi with support for HTTP/HTTPS streams
- 🖼️ **Classic Mac UI**: Authentic retro Macintosh-style interface with draggable windows
- 📻 **Station Management**: Add, edit, and manage unlimited radio stations
- 💾 **Persistent Storage**: Stations and settings saved to flash memory
- 🔊 **I2S Audio Output**: High-quality audio streaming via I2S DAC
- ⚡ **Multi-Core Architecture**: Dual-core processing for smooth UI and audio
- 📊 **Real-time Monitoring**: CPU usage, clock, and stream metadata display
- 🖱️ **Touch Interface**: Fully touch-enabled with custom keyboard for text input

## Hardware Requirements

### Required Components

- **WT32-SC01 Plus Development Board**
  - ESP32-S3 (Dual-Core @ 240MHz)
  - 3.5" Capacitive Touch Display (480x320)
  - 16MB Flash Memory
  - Built-in ST7796 LCD Controller
  - FT5x06 Touch Controller

- **I2S DAC Module** (e.g., MAX98357A, PCM5102)
  - For audio output
  - Connected to GPIO pins 35, 36, 37

### Pin Configuration

```
I2S Audio:
- BCLK: GPIO 36
- LRC:  GPIO 35
- DOUT: GPIO 37

Display: Built-in (configured in wt32_sc01_plus.h)
Touch:   Built-in (I2C on GPIO 5, 6)
```

## Software Requirements

### Development Tools

Choose one of the following:

1. **PlatformIO** (Recommended)
   - Visual Studio Code + PlatformIO Extension
   - Or PlatformIO CLI

2. **Arduino IDE**
   - Arduino IDE 2.x or higher
   - ESP32 Board Support Package

### Dependencies

All dependencies are automatically managed by PlatformIO. For manual installation:

- ESP32-audioI2S (^3.0.12)
- LovyanGFX (^1.2.7)
- ArduinoJson (^7.4.2)
- LVGL (^8.3.3)

## Quick Start

### 1. Clone the Repository

```bash
git clone https://github.com/felangga/retrodio.git
cd retrodio
```

### 2. Configure WiFi

Edit `include/config.h`:

```cpp
#define WIFI_SSID "your_wifi_name"
#define WIFI_PASSWORD "your_wifi_password"
```

### 3. Build and Upload

**Using PlatformIO:**

```bash
pio run --target upload
pio device monitor
```

**Using Arduino IDE:**

1. Open `retrodio.ino`
2. Select Board: "ESP32S3 Dev Module"
3. Upload

### 4. First Run

On first boot, Retrodio will:
1. Initialize the file system
2. Create default station list
3. Connect to WiFi
4. Display the radio interface

## Project Structure

```
retrodio/
├── platformio.ini              # PlatformIO configuration
├── partition_16MB_ota_spiffs.csv  # Custom partition table
├── src/
│   ├── main.cpp               # Main application
│   └── ConfigManager.cpp      # Configuration management
├── include/
│   ├── ConfigManager.h
│   ├── config.h               # WiFi and app settings
│   └── wt32_sc01_plus.h       # Display hardware config
└── lib/
    └── MacUI/                 # Custom UI library
        ├── library.json
        └── src/
            ├── MacUI.h        # Main UI header
            ├── MacUI.cpp      # Core UI functions
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

## MacUI Library

The custom MacUI library provides classic Macintosh-style UI components:

### Components

- **Windows**: Draggable windows with title bars, minimize/close buttons
- **Buttons**: Standard and symbol buttons (play, pause, stop, next, prev, etc.)
- **Labels**: Text display with customizable colors and sizes
- **Running Text**: Auto-scrolling text with pause/resume functionality
- **List Views**: Scrollable lists with touch support and item callbacks
- **Input Fields**: Text input fields with cursor support
- **Checkboxes**: Toggle controls for settings
- **Sliders**: Value adjustment controls
- **Progress Bars**: Visual progress indicators
- **Keyboard**: On-screen keyboard for text input

### Usage Example

```cpp
// Create a button
MacComponent* btnPlay = createButtonComponent(
    80, 160,        // x, y position
    60, 60,         // width, height
    1,              // component ID
    "",             // text (empty for symbol)
    SYMBOL_PLAY     // symbol type
);
btnPlay->onClick = onComponentClick;
addChildComponent(radioWindow, btnPlay);
```

## Architecture

### Multi-Core Design

Retrodio leverages the ESP32-S3's dual-core architecture for optimal performance:

- **Core 0 (Audio Task)**
  - Audio streaming and decoding
  - Network buffer management
  - Stream metadata parsing
  - I2S audio output

- **Core 1 (UI Task)**
  - Display rendering
  - Touch input handling
  - UI component updates
  - System monitoring

### Thread-Safe Communication

- Mutex-protected metadata buffers
- Non-blocking semaphore operations
- Separate task stacks (16KB each)

## Features in Detail

### Real-time Monitoring

- **CPU Usage**: Live CPU usage for both cores
  - Core 0: Audio streaming task
  - Core 1: UI rendering task
  - Updates every second

- **Clock**: Real-time clock synchronized via NTP
  - Configurable timezone (GMT+7 default)
  - 24-hour format display

- **Stream Metadata**:
  - Station name from ICY headers
  - Current track/song info
  - Auto-scrolling for long text

### Station Management

- Add unlimited stations (up to 50)
- Edit station names and URLs
- Delete unwanted stations
- Persistent storage in flash
- Quick station switching with prev/next buttons

### Configuration System

Retrodio uses LittleFS for persistent configuration storage:

- **Settings** (`/settings.json`): Volume, last station
- **Stations** (`/stations.json`): Station list with names and URLs
- **Auto-save**: All changes saved automatically
- **Factory Reset**: Restore default settings and stations

See [README_CONFIG.md](README_CONFIG.md) for detailed configuration documentation.

## Usage

### Basic Controls

- **Play/Pause**: Start/pause current station
- **Stop**: Stop playback
- **Next/Prev**: Switch between stations
- **Station List**: Open station browser
- **Add Station**: Add new radio station

### Managing Stations

1. Click the "List" button to open station browser
2. Click "Add +" to add a new station
3. Enter station name and URL
4. Click "Save" to add the station
5. Click on any station to start playing

### Adding Custom Stations

Stations can be added via:

1. **UI**: Use the on-screen keyboard
2. **Code**: Edit `ConfigManager::factoryReset()`
3. **File**: Upload `stations.json` to SPIFFS

Example `stations.json`:

```json
{
  "stations": [
    {
      "name": "My Radio",
      "url": "http://stream.example.com:8000/radio"
    }
  ]
}
```

## Configuration

### WiFi Settings

Edit `include/config.h`:

```cpp
#define WIFI_SSID "YourNetwork"
#define WIFI_PASSWORD "YourPassword"

// NTP Configuration
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 25200    // GMT+7 (Indonesia)
#define DST_OFFSET_SEC 0        // No DST
```

### Audio Settings

```cpp
#define DEFAULT_VOLUME 5         // Initial volume (0-21)
```

### Display Orientation

Edit `src/main.cpp` setup():

```cpp
lcd.setRotation(lcd.getRotation() ^ 1);  // Toggle orientation
```

## Troubleshooting

### Build Issues

**Memory/Partition Errors:**
- Ensure using custom partition table: `partition_16MB_ota_spiffs.csv`
- Firmware requires ~1.6MB (won't fit in default 1.3MB partition)

**Library Not Found:**
```bash
pio pkg install --force
```

### WiFi Issues

**Connection Fails:**
- Check SSID and password in `config.h`
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Check router compatibility with ESP32

**Serial Debug:**
Enable debugging in `main.cpp`:
```cpp
#define ENABLE_SERIAL_DEBUG 1
```

### Audio Issues

**No Sound:**
- Check I2S DAC wiring (pins 35, 36, 37)
- Verify DAC power supply
- Test with different stream URL

**Choppy/Stuttering:**
- Check WiFi signal strength
- Try lower bitrate streams
- Increase audio buffer size in `initializeAudio()`

### Display Issues

**Touch Not Working:**
- Verify touch controller (FT5x06) configuration
- Check I2C pins (GPIO 5, 6)

**Wrong Orientation:**
- Adjust rotation in setup()

## Performance

### Memory Usage

- **Free Heap**: ~100-150KB during operation
- **Free PSRAM**: Available for buffering
- **Stack Usage**: 16KB per task

### CPU Usage

Typical CPU usage:
- **Core 0**: 25-45% (streaming), 5-10% (idle)
- **Core 1**: 15-30% (UI active)

## Development

### Building Custom UI Components

See MacUI library source for component examples:

```cpp
MacComponent* createCustomComponent(int x, int y, int w, int h, int id) {
  MacComponent* component = new MacComponent();
  component->type = COMPONENT_CUSTOM;
  component->x = x;
  component->y = y;
  component->w = w;
  component->h = h;
  component->id = id;
  component->visible = true;
  component->enabled = true;
  return component;
}
```

### Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## Documentation

- [PlatformIO Setup Guide](README_PLATFORMIO.md)
- [Configuration System](README_CONFIG.md)
- [PlatformIO Documentation](https://docs.platformio.org/)
- [ESP32-audioI2S Library](https://github.com/schreibfaul1/ESP32-audioI2S)
- [LovyanGFX Documentation](https://github.com/lovyan03/LovyanGFX)

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Credits

- **Author**: Felangga
- **UI Library**: Custom MacUI (Classic Macintosh-inspired)
- **Audio Library**: ESP32-audioI2S by schreibfaul1
- **Graphics Library**: LovyanGFX by lovyan03
- **Hardware**: WT32-SC01 Plus by Wireless-Tag

## Acknowledgments

- Classic Macintosh OS for UI inspiration
- ESP32 community for excellent libraries and support
- Arduino framework developers

## Support

For issues, questions, or contributions:
- GitHub Issues: [Create an issue](https://github.com/felangga/retrodio/issues)
- Documentation: See `README_*.md` files

## Changelog

### Version 1.0.0
- Initial release
- Classic Mac UI implementation
- Multi-core audio streaming
- Station management with persistent storage
- Real-time CPU monitoring
- Touch-enabled interface
- On-screen keyboard support

---

**Made with ❤️ for retro computing enthusiasts**
