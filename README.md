# Retrodio

A retro-style internet radio for ESP32 with touchscreen interface.

![IMG_1735_VSCO](https://github.com/user-attachments/assets/26884c5d-ff6f-4797-ba58-dd58b5bbd2b2)

## Overview

Retrodio is an ESP32-based internet radio player featuring a graphical user interface for browsing and playing streaming radio stations. Built for the WT32-SC01 Plus development board with integrated touchscreen display.


## Features

- Stream internet radio stations over WiFi
- Touchscreen GUI for easy station selection
- Add, edit, and delete radio stations
- Volume control with visual feedback
- OTA (Over-The-Air) firmware updates
- Web server for remote management
- Persistent station storage

## Hardware Requirements

- **Board**: WT32-SC01 Plus (ESP32-S3, 16MB Flash, 2MB PSRAM)
- **Display**: 480x320 touchscreen (integrated on WT32-SC01 Plus)
- **Audio**: I2S DAC (connected via I2S interface)

### Web Remote
You can also remote this radio when you are connected to the same wifi network as the radio. 
Just go to `retrodio.local` on your browser (mobile/pc), and it will open the remote web.

<img width="1150" height="1160" alt="image" src="https://github.com/user-attachments/assets/00c14624-c153-469e-bd77-bf7db29ac6d2" />

## Schematic

Basically, you only need the WT32-SC01 and a speaker, since all the requirements are already built into the LCD itself, including the ESP32 and the amplifier. 
Just connect the LCD to the speaker, and powering it up! 

<img width="573" height="385" alt="image" src="https://github.com/user-attachments/assets/601328d5-5570-4e1f-a792-b899ae194c55" />

The speaker connector is beside the MicroSD Card slot.

## Software Dependencies

- [PlatformIO](https://platformio.org/)
- [LovyanGFX](https://github.com/lovyan03/LovyanGFX) - Graphics library
- [ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S) - Audio streaming
- [ArduinoJson](https://arduinojson.org/) - JSON parsing

## Building and Flashing

### Prerequisites

1. Install [PlatformIO](https://platformio.org/install)
2. Clone this repository

### Build

```bash
# Build for USB upload
pio run -e usb

# Build for OTA update
pio run -e ota
```

### Upload

```bash
# Upload via USB
pio run -e usb -t upload

# Upload via OTA (requires device on network)
pio run -e ota -t upload
```

### Monitor Serial Output

```bash
pio device monitor -b 115200
```

## Configuration

WiFi credentials and radio stations are configured through the touchscreen interface on first boot.

## Project Structure

```
src/
├── main.cpp              - Main application entry
├── RadioWindow.*         - Main radio interface
├── WifiWindow.*          - WiFi configuration
├── AddStationWindow.*    - Add/edit stations
├── StationManager.*      - Station list management
├── AudioHandlers.*       - Audio streaming handlers
├── NetworkHandlers.*     - WiFi/network management
├── WebServer.*           - Web interface
├── OTAHandler.*          - OTA update handling
└── GlobalState.h         - Shared application state
```

## License

This project is licensed under CC BY-NC-SA 4.0 (Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International).

**For personal/educational use only.** Commercial use requires permission. See [LICENSE](LICENSE) for details.

## Author

felangga - [GitHub](https://github.com/felangga/retrodio)

## Acknowledgments

- ESP32 community
- LovyanGFX graphics library
- ESP32-audioI2S audio library
