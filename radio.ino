#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"
#include "esp32-hal-psram.h"
#include "wt32_sc01_plus.h"
#include "MacUI.h"
#include <time.h>

// ===== CONFIGURATION =====

// WiFi credentials (consider moving to separate config file for security)
const String WIFI_SSID = "WTM-KB32";
const String WIFI_PASSWORD = "kpbl3224";

// Radio stream URL
const String RADIO_URL = "http://202.65.114.229:9314/";

// Audio settings
#define DEFAULT_VOLUME 10  // Range: 0-21

// NTP config
static const char* NTP_SERVER = "pool.ntp.org";
static const long GMT_OFFSET_SEC = 0;  // adjust as needed
static const int DST_OFFSET_SEC = 0;   // adjust daylight saving

// ===== GLOBAL OBJECTS =====

static LGFX lcd;  // Display instance
Audio audio;      // Audio streaming instance

// Clock state
unsigned long lastClockUpdate = 0;
String lastClockText;

// Music player state
bool isPlaying = false;

// Forward declarations for callbacks
void onWindowMinimize();
void onWindowClose();
void onRadioIconClick();

// Main radio window - declared early so callbacks can reference it
MacWindow radioWindow{ 20, 40, 420, 240, "Internet Radio Player", true, false, true, onWindowMinimize, onWindowClose, false, 0, 0 };

// Desktop icon for minimized radio window
DesktopIcon radioIcon{ 50, 60, "Radio Player", "window", false, false, &radioWindow, onRadioIconClick };

// ===== FUNCTION PROTOTYPES =====

void connectToWiFi();
void initializeAudio();
void updateClock();

// ===== BUTTON DECLARATIONS =====
// Forward declarations for button callbacks
void onPlay();
void onStop();
void onVolUp();
void onVolDown();
void onPrev();
void onNext();

// Application buttons with callbacks - Music Player Style
// Redesigned layout: main playback controls in center, volume on sides
// Using symbols instead of text for a modern music player look

// Main playback controls - larger and centered
MacButton btnPrev{ 40, 120, 50, 50, "", SYMBOL_PREV, 4, false, onPrev };
MacButton btnPlay{ 100, 110, 60, 60, "", SYMBOL_PLAY, 1, false, onPlay };
MacButton btnNext{ 170, 120, 50, 50, "", SYMBOL_NEXT, 5, false, onNext };

// Volume controls - smaller, positioned on the right
MacButton btnVolUp{ 240, 110, 45, 35, "", SYMBOL_VOL_UP, 3, false, onVolUp };
MacButton btnVolDn{ 240, 150, 45, 35, "", SYMBOL_VOL_DOWN, 6, false, onVolDown };

// Stop button - separate and smaller
MacButton btnStop{ 120, 180, 40, 40, "", SYMBOL_STOP, 2, false, onStop };

// ===== BUTTON CALLBACKS =====

void onPlay() {
  if (isPlaying) {
    // Currently playing - pause/stop
    audio.stopSong();
    isPlaying = false;
    btnPlay.symbol = SYMBOL_PLAY;
    displayStatus(lcd, "Paused", 160);
  } else {
    // Currently stopped - start playing
    audio.connecttohost(RADIO_URL.c_str());
    isPlaying = true;
    btnPlay.symbol = SYMBOL_PAUSE;
    displayStatus(lcd, "Playing", 160);
  }
  // Force button redraw to show new symbol
  redrawButton(lcd, btnPlay);
}

void onStop() {
  audio.stopSong();
  isPlaying = false;
  btnPlay.symbol = SYMBOL_PLAY;
  displayStatus(lcd, "Stopped", 160);
  // Force button redraw to show play symbol
  redrawButton(lcd, btnPlay);
}

void onVolUp() {
  audio.setVolume(min(21, audio.getVolume() + 1));
  displayStatus(lcd, "Volume: " + String(audio.getVolume()), 160);
  // Update volume display in window
  if (!radioWindow.minimized && radioWindow.visible) {
    draw3DFrame(lcd, radioWindow.x + 310, radioWindow.y + 35, 90, 25, true);
    lcd.setTextColor(MAC_BLACK, MAC_WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(radioWindow.x + 315, radioWindow.y + 43);
    lcd.printf("Volume: %d", audio.getVolume());
  }
}

void onPrev() {
  displayStatus(lcd, "Previous Station", 160);
  // Add your station switching logic here
  // For example: switchToStation(currentStation - 1);
}

void onNext() {
  displayStatus(lcd, "Next Station", 160);
  // Add your station switching logic here  
  // For example: switchToStation(currentStation + 1);
}

void onVolDown() {
  audio.setVolume(max(0, audio.getVolume() - 1));
  displayStatus(lcd, "Volume: " + String(audio.getVolume()), 160);
  // Update volume display in window
  if (!radioWindow.minimized && radioWindow.visible) {
    draw3DFrame(lcd, radioWindow.x + 310, radioWindow.y + 35, 90, 25, true);
    lcd.setTextColor(MAC_BLACK, MAC_WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(radioWindow.x + 315, radioWindow.y + 43);
    lcd.printf("Volume: %d", audio.getVolume());
  }
}

void onOK() {
  displayStatus(lcd, "OK", 160);
}

// Window management callbacks
void onWindowMinimize() {
  if (radioWindow.minimized) {
    // Window was minimized - show desktop icon
    radioIcon.visible = true;
    drawDesktopIcon(lcd, radioIcon.x, radioIcon.y, radioIcon.name, false);
    displayStatus(lcd, "Window minimized to desktop", 300);
  } else {
    // Window was restored - hide desktop icon
    radioIcon.visible = false;
    // Clear the icon area
    redrawDesktopArea(lcd, radioIcon.x - 2, radioIcon.y, 36, 50);
    displayStatus(lcd, "Window restored", 300);
    
    // Update button positions and redraw content
    redrawWindowContent(lcd, radioWindow);
  }
}

void onWindowClose() {
  radioIcon.visible = false; // Hide icon when window is closed
  displayStatus(lcd, "Window closed", 300);
}

void onRadioIconClick() {
  // Restore window when icon is clicked
  radioWindow.minimized = false;
  radioWindow.visible = true;
  drawInterface(lcd); // Redraw entire interface
}


// ===== CALLBACK FUNCTIONS =====

/**
 * Audio information callback
 * Displays audio stream information on screen
 */
void my_audio_info(Audio::msg_t m) {
  // Display audio info in a designated area
  // lcd.fillRect(250, 40, 220, 60, MAC_WHITE);
  // lcd.drawRect(250, 40, 220, 60, MAC_BLACK);
  // lcd.setTextColor(MAC_BLACK, MAC_WHITE);
  // lcd.setTextSize(1);
  // lcd.setCursor(255, 45);
  // lcd.println("Audio Info:");
  // lcd.setCursor(255, 60);
  // lcd.printf("Event: %s", m.s);
  // lcd.setCursor(255, 75);
  // // Truncate long messages to fit
  // String msg = String(m.msg);
  // if (msg.length() > 30) {
  //   msg = msg.substring(0, 27) + "...";
  // }
  // lcd.printf("Msg: %s", msg.c_str());
}

// ===== MAIN FUNCTIONS =====

void setup() {
  lcd.init();

  lcd.setRotation(lcd.getRotation() ^ 1);
  lcd.fillScreen(MAC_WHITE);

  drawInterface(lcd);
  
  // Initialize double buffering for smooth window movement
  initializeDoubleBuffer();

  // connectToWiFi();

  // Configure time via NTP after WiFi connected
  // configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);
  // displayStatus(lcd, "NTP time requested...", 200);

  // Audio::audio_info_callback = my_audio_info;
  // initializeAudio();
}

/**
 * Arduino main loop
 * Handles audio streaming
 */
void loop() {
  // audio.loop();
  // updateClock();

  // Update music player visualization and progress bar when playing
  static unsigned long lastVisualizationUpdate = 0;
  static unsigned long lastProgressUpdate = 0;
  
  if (isPlaying && !radioWindow.minimized && radioWindow.visible) {
    // Update spectrum visualization
    if (millis() - lastVisualizationUpdate > 150) {
      drawSpectrumVisualization(lcd, radioWindow.x + 220, radioWindow.y + 35, 80, 25, true);
      lastVisualizationUpdate = millis();
    }
    
    // Update progress bar animation
    if (millis() - lastProgressUpdate > 100) {
      static int progressPos = 0;
      
      // Clear old progress indicator
      draw3DFrame(lcd, radioWindow.x + 20, radioWindow.y + 160, 280, 8, true);
      
      // Draw new progress indicator
      progressPos = (progressPos + 8) % 260;
      lcd.fillRect(radioWindow.x + 30 + progressPos, radioWindow.y + 162, 20, 4, MAC_BLUE);
      
      lastProgressUpdate = millis();
    }
  }

  // Handle window interaction (minimize/close buttons)
  if (radioWindow.visible) {
    interactiveWindow(lcd, radioWindow);
  }

  // Handle desktop icon interaction
  if (radioIcon.visible) {
    interactiveDesktopIcon(lcd, radioIcon);
  }

  // Only show buttons if window is not minimized and visible
  if (!radioWindow.minimized && radioWindow.visible) {
    interactiveButton(lcd, btnPlay);
    interactiveButton(lcd, btnStop);
    interactiveButton(lcd, btnVolUp);
    interactiveButton(lcd, btnPrev);
    interactiveButton(lcd, btnNext);
    interactiveButton(lcd, btnVolDn);
  }

  vTaskDelay(1);  // Small delay to prevent watchdog issues
}

// Clean up resources when done (call this if you need to free memory)
void cleanup() {
  cleanupDoubleBuffer();
}

// ===== HELPER FUNCTIONS =====

void updateClock() {
  unsigned long now = millis();
  if (now - lastClockUpdate < 1000) return;  // update once per second
  lastClockUpdate = now;

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return;  // keep previous
  }
  char buf[9];  // HH:MM:SS
  strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
  String current = String(buf);
  if (current == lastClockText) return;  // no change
  lastClockText = current;

  // Redraw clock area (menu bar right side)
  drawClock(lcd, current);
}

/**
 * Connect to WiFi network
 */
void connectToWiFi() {
  displayStatus(lcd, "Connecting to WiFi...", 160);

  WiFi.begin(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    displayStatus(lcd, "WiFi connecting... " + String(attempts + 1), 160);
    delay(1000);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    displayStatus(lcd, "WiFi Connected!", 160);

    lcd.setTextColor(MAC_BLACK, MAC_WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(275, 160);
    lcd.println("Connected:");
    lcd.setCursor(275, 175);
    lcd.println(WiFi.localIP().toString());
  } else {
    displayStatus(lcd, "WiFi Failed!", 160);
  }
}

/**
 * Initialize audio system and connect to radio stream
 */
void initializeAudio() {
  displayStatus(lcd, "Setting up audio...", 180);

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(DEFAULT_VOLUME);

  displayStatus(lcd, "Connecting to stream...", 180);

  if (audio.connecttohost(RADIO_URL.c_str())) {
    displayStatus(lcd, "Stream connected!", 180);

    lcd.fillRect(45, 180, 175, 15, MAC_WHITE);
    lcd.setTextColor(MAC_BLACK, MAC_WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(45, 180);
    lcd.println("\u266A Playing: Internet Radio");
    lcd.setCursor(45, 195);
    lcd.println("Status: Connected");
    lcd.setCursor(45, 210);
    lcd.println("Volume: " + String(DEFAULT_VOLUME));
  } else {
    displayStatus(lcd, "Stream failed!", 180);

    lcd.fillRect(45, 180, 175, 15, MAC_WHITE);
    lcd.setTextColor(MAC_BLACK, MAC_WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(45, 180);
    lcd.println("\u2717 Connection failed");
  }
}

void drawInterface(lgfx::LGFX_Device& lcd) {
  lcd.fillScreen(MAC_WHITE);
  drawCheckeredPattern(lcd);
  drawMenuBar(lcd);
  
  // Draw the window using the new window system
  drawWindow(lcd, radioWindow);

  // Update button positions relative to window position (music player layout)
  // Main playback controls - centered
  btnPrev.x = radioWindow.x + 20;
  btnPrev.y = radioWindow.y + 80;
  btnPlay.x = radioWindow.x + 80;
  btnPlay.y = radioWindow.y + 70;
  btnNext.x = radioWindow.x + 150;
  btnNext.y = radioWindow.y + 80;
  
  // Volume controls - right side
  btnVolUp.x = radioWindow.x + 300;
  btnVolUp.y = radioWindow.y + 70;
  btnVolDn.x = radioWindow.x + 300;
  btnVolDn.y = radioWindow.y + 110;
  
  // Stop button - below play button
  btnStop.x = radioWindow.x + 90;
  btnStop.y = radioWindow.y + 140;

  // Only draw content if window is not minimized
  if (!radioWindow.minimized && radioWindow.visible) {
    redrawWindowContent(lcd, radioWindow);
  }
}

// Helper function to redraw window content (needed for dragging)
void redrawWindowContent(lgfx::LGFX_Device& lcd, const MacWindow& window) {
  if (!window.visible || window.minimized) return;
  
  // Draw now playing info area at the top
  draw3DFrame(lcd, window.x + 10, window.y + 35, 200, 25, true);
  lcd.setTextColor(MAC_BLACK, MAC_WHITE);
  lcd.setTextSize(1);
  lcd.setCursor(window.x + 15, window.y + 43);
  if (isPlaying) {
    lcd.println("♪ Now Playing: Radio Stream");
  } else {
    lcd.println("Radio Ready - Press Play");
  }

  // Draw spectrum visualization next to the info
  drawSpectrumVisualization(lcd, window.x + 220, window.y + 35, 80, 25, isPlaying);

  // Draw volume indicator
  draw3DFrame(lcd, window.x + 310, window.y + 35, 90, 25, true);
  lcd.setCursor(window.x + 315, window.y + 43);
  lcd.printf("Volume: %d", audio.getVolume());

  // Draw a fake progress bar area (since it's streaming, we'll show activity)
  draw3DFrame(lcd, window.x + 20, window.y + 160, 280, 8, true);
  if (isPlaying) {
    // Show some activity in the progress bar
    static int progressPos = 0;
    progressPos = (progressPos + 5) % 260;
    lcd.fillRect(window.x + 30 + progressPos, window.y + 162, 20, 4, MAC_BLUE);
  }

  // Draw status area at the bottom
  draw3DFrame(lcd, window.x + 20, window.y + 190, 280, 30, true);
  lcd.setCursor(window.x + 25, window.y + 200);
  if (isPlaying) {
    lcd.println("♪ Streaming... Internet Radio v1.0");
  } else {
    lcd.println("Ready to play - Internet Radio v1.0");
  }

  // Update button positions relative to window (music player layout)
  btnPrev.x = window.x + 20;
  btnPrev.y = window.y + 80;
  btnPlay.x = window.x + 80;
  btnPlay.y = window.y + 70;
  btnNext.x = window.x + 150;
  btnNext.y = window.y + 80;
  
  btnVolUp.x = window.x + 300;
  btnVolUp.y = window.y + 70;
  btnVolDn.x = window.x + 300;
  btnVolDn.y = window.y + 110;
  
  btnStop.x = window.x + 90;
  btnStop.y = window.y + 140;

  // Redraw all buttons with new positions
  redrawButton(lcd, btnPrev);
  redrawButton(lcd, btnPlay);
  redrawButton(lcd, btnNext);
  redrawButton(lcd, btnVolUp);
  redrawButton(lcd, btnVolDn);
  redrawButton(lcd, btnStop);
}

// ===== DOUBLE BUFFERING FUNCTIONS =====

void initializeDoubleBuffer() {
  // Create a smaller sprite buffer optimized for window dragging
  if (windowBuffer == nullptr) {
    windowBuffer = new lgfx::LGFX_Sprite(&lcd);
    // Use a reasonably sized buffer - enough for window + margin
    int bufferWidth = min(480, radioWindow.w + 40);   // Max screen width or window + margin
    int bufferHeight = min(320, radioWindow.h + 40);  // Max screen height or window + margin
    
    if (windowBuffer->createSprite(bufferWidth, bufferHeight)) {
      // Successfully created buffer
      displayStatus(lcd, "Double buffer ready", 240);
    } else {
      // Failed to create buffer - try smaller size
      bufferWidth = radioWindow.w + 20;
      bufferHeight = radioWindow.h + 20;
      
      if (windowBuffer->createSprite(bufferWidth, bufferHeight)) {
        displayStatus(lcd, "Small buffer ready", 240);
      } else {
        // Fall back to direct drawing
        delete windowBuffer;
        windowBuffer = nullptr;
        displayStatus(lcd, "Direct draw mode", 240);
      }
    }
  }
}

void cleanupDoubleBuffer() {
  if (windowBuffer != nullptr) {
    windowBuffer->deleteSprite();
    delete windowBuffer;
    windowBuffer = nullptr;
  }
}

void renderBackgroundToBuffer(int x, int y, int w, int h) {
  if (windowBuffer == nullptr) return;
  
  // Clear the buffer
  windowBuffer->fillScreen(MAC_WHITE);
  
  // Draw checkered pattern in the buffer
  int patternSize = 8;
  int startX = (x / patternSize) * patternSize;
  int startY = max(21, (y / patternSize) * patternSize); // Don't overwrite menu bar
  
  for (int py = startY; py < y + h; py += patternSize) {
    for (int px = startX; px < x + w; px += patternSize) {
      if ((px / patternSize + py / patternSize) % 2 == 0) {
        // Calculate position in buffer coordinates
        int bufX = px - x;
        int bufY = py - y;
        
        // Only draw pattern within the specified area and buffer bounds
        if (bufX >= 0 && bufY >= 0 && bufX < w && bufY < h) {
          int rectW = min(patternSize, w - bufX);
          int rectH = min(patternSize, h - bufY);
          
          if (rectW > 0 && rectH > 0) {
            windowBuffer->fillRect(bufX, bufY, rectW, rectH, MAC_LIGHT_GRAY);
          }
        }
      }
    }
  }
}

void drawWindowSmooth(lgfx::LGFX_Device& lcd, MacWindow& window, int oldX, int oldY) {
  // Optimized approach: reduce flicker with minimal buffer usage
  if (windowBuffer != nullptr) {
    
    // For smooth movement, use a simple optimization:
    // 1. Clear old position
    // 2. Draw background for new position 
    // 3. Draw window at new position
    // All done with minimal intermediate steps
    
    // Clear old window area
    redrawDesktopArea(lcd, oldX, oldY, window.w + 5, window.h + 5);
    
    // Draw window at new position immediately
    drawWindow(lcd, window);
    
  } else {
    // Standard direct drawing
    redrawDesktopArea(lcd, oldX, oldY, window.w + 5, window.h + 5);
    drawWindow(lcd, window);
  }
  
  // Always update button positions after movement
  if (!window.minimized) {
    redrawWindowContent(lcd, window);
  }
}
