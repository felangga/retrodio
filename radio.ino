#include "Arduino.h"
#include "esp_task_wdt.h"
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

// Multi-core task handles
TaskHandle_t uiTaskHandle = NULL;
TaskHandle_t audioTaskHandle = NULL;

// Clock state
unsigned long lastClockUpdate = 0;
String lastClockText;

// Music player state
bool isPlaying = false;

// Forward declarations for callbacks
void onWindowMinimize();
void onWindowClose();
void onRadioIconClick();
void onWindowContentClick(int relativeX, int relativeY);
void onWindowMoved();

// Main radio window - declared early so callbacks can reference it
MacWindow radioWindow{ 20, 40, 420, 240, "Internet Radio Player", true, false, true, onWindowMinimize, onWindowClose, onWindowContentClick, onWindowMoved, nullptr, 0, false, 0, 0 };

// Desktop icon for minimized radio window
DesktopIcon radioIcon{ 50, 60, "Radio Player", "window", false, false, &radioWindow, onRadioIconClick };

// ===== FUNCTION PROTOTYPES =====

void connectToWiFi();
// void initializeAudio();
void updateClock();

// Multi-core task functions
void uiTask(void* parameter);
void audioTask(void* parameter);

// Window setup functions
void initializeRadioWindow();

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

// ===== GENERAL BUTTON HELPER FUNCTIONS =====

MacButton* findButtonById(const MacWindow& window, int id) {
  if (window.childButtons == nullptr || window.childButtonCount == 0) {
    return nullptr;
  }
  
  for (int i = 0; i < window.childButtonCount; i++) {
    MacButton* btn = window.childButtons[i];
    if (btn != nullptr && btn->id == id) {
      return btn;
    }
  }
  
  return nullptr;
}

void updateButtonSymbol(const MacWindow& window, int buttonId, SymbolType newSymbol) {
  MacButton* btn = findButtonById(window, buttonId);
  if (btn != nullptr) {
    btn->symbol = newSymbol;
    // Force redraw - the window system will handle absolute positioning
    if (btn->symbol != SYMBOL_NONE) {
      drawSymbolButton(lcd, window.x + btn->x, window.y + btn->y, btn->w, btn->h, btn->symbol, btn->pressed);
    } else {
      drawButton(lcd, window.x + btn->x, window.y + btn->y, btn->w, btn->h, btn->text, btn->pressed);
    }
  }
}

// ===== BUTTON CALLBACKS =====

void onPlay() {
  Serial.println("Play button pressed");
  displayStatus(lcd, "Play pressed", 160);
  // if (isPlaying) {
  //   // Currently playing - pause/stop
  //   audio.stopSong();
  //   isPlaying = false;
  //   updateButtonSymbol(radioWindow, 1, SYMBOL_PLAY);  // ID 1 is play button
  //   displayStatus(lcd, "Paused", 160);
  // } else {
  //   // Currently stopped - start playing
  //   audio.connecttohost(RADIO_URL.c_str());
  //   isPlaying = true;
  //   updateButtonSymbol(radioWindow, 1, SYMBOL_PAUSE);  // ID 1 is play button
  //   displayStatus(lcd, "Playing", 160);
  // }
  // Force button redraw to show new symbol
  updateButtonSymbol(radioWindow, 1, SYMBOL_PLAY);  // Just show play for now
}

void onStop() {
  Serial.println("Stop button pressed");
  displayStatus(lcd, "Stop pressed", 160);
  // audio.stopSong();
  // isPlaying = false;
  // updateButtonSymbol(radioWindow, 1, SYMBOL_PLAY);  // ID 1 is play button
  // displayStatus(lcd, "Stopped", 160);
  // Force button redraw to show play symbol
  updateButtonSymbol(radioWindow, 1, SYMBOL_PLAY);  // ID 1 is play button
}

void onVolUp() {
  Serial.println("Volume Up pressed");
  displayStatus(lcd, "Vol Up pressed", 160);
  // audio.setVolume(min(21, audio.getVolume() + 1));
  // displayStatus(lcd, "Volume: " + String(audio.getVolume()), 160);
  // Update volume display in window
  if (!radioWindow.minimized && radioWindow.visible) {
    draw3DFrame(lcd, radioWindow.x + 310, radioWindow.y + 35, 90, 25, true);
    lcd.setTextColor(MAC_BLACK, MAC_WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(radioWindow.x + 315, radioWindow.y + 43);
    lcd.printf("Volume: %d", 10);  // audio.getVolume()
  }
}

void onPrev() {
  Serial.println("Previous button pressed");
  displayStatus(lcd, "Previous Station", 160);
  // Add your station switching logic here
  // For example: switchToStation(currentStation - 1);
}

void onNext() {
  Serial.println("Next button pressed");
  displayStatus(lcd, "Next Station", 160);
  // Add your station switching logic here
  // For example: switchToStation(currentStation + 1);
}

void onVolDown() {
  Serial.println("Volume Down pressed");
  displayStatus(lcd, "Vol Down pressed", 160);
  // audio.setVolume(max(0, audio.getVolume() - 1));
  // displayStatus(lcd, "Volume: " + String(audio.getVolume()), 160);
  // Update volume display in window
  if (!radioWindow.minimized && radioWindow.visible) {
    draw3DFrame(lcd, radioWindow.x + 310, radioWindow.y + 35, 90, 25, true);
    lcd.setTextColor(MAC_BLACK, MAC_WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(radioWindow.x + 315, radioWindow.y + 43);
    lcd.printf("Volume: %d", 10);  // audio.getVolume()
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
  radioIcon.visible = false;  // Hide icon when window is closed
  displayStatus(lcd, "Window closed", 300);
}

void onRadioIconClick() {
  // Restore window when icon is clicked
  radioWindow.minimized = false;
  radioWindow.visible = true;
  drawInterface(lcd);  // Redraw entire interface
}

// Window content interaction callback
void onWindowContentClick(int relativeX, int relativeY) {
  Serial.printf("Window content clicked at relative position: %d, %d\n", relativeX, relativeY);
  
  // Use the general child component system to find the clicked button
  MacButton* clickedButton = findButtonAt(radioWindow, radioWindow.x + relativeX, radioWindow.y + relativeY);
  
  if (clickedButton != nullptr && clickedButton->onClick != nullptr) {
    // Call the button's callback function
    clickedButton->onClick();
  }
}

// Window moved callback
void onWindowMoved() {
  Serial.println("Window moved - child buttons automatically positioned relative to window");
  
  // With the new system, buttons are positioned relative to the window
  // so no manual position updates are needed when the window moves
  // The drawWindowChildButtons function handles absolute positioning automatically
  
  // Just redraw the window content
  redrawWindowContent(lcd, radioWindow);
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
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting radio application...");

  try {
    lcd.init();
    Serial.println("LCD initialized");

    lcd.setRotation(lcd.getRotation() ^ 1);
    Serial.println("LCD rotation set");

    lcd.fillScreen(MAC_WHITE);
    Serial.println("Screen cleared");

    drawInterface(lcd);
    Serial.println("Interface drawn");
  } catch (...) {
    Serial.println("Error in setup!");
    while (1) delay(1000);
  }

  // Create UI task on Core 1 (default Arduino core)
  xTaskCreatePinnedToCore(
    uiTask,           // Task function
    "UI_Task",        // Task name
    10000,            // Stack size (bytes)
    NULL,             // Parameter
    1,                // Priority
    &uiTaskHandle,    // Task handle
    1                 // Core (0 or 1)
  );

  // Create Audio task on Core 0 (background core)
  xTaskCreatePinnedToCore(
    audioTask,        // Task function
    "Audio_Task",     // Task name
    8000,             // Stack size (bytes) 
    NULL,             // Parameter
    2,                // Higher priority for audio
    &audioTaskHandle, // Task handle
    0                 // Core (0 or 1)
  );

  Serial.println("Multi-core tasks created");
  Serial.println("Setup complete");
}

/**
 * Arduino main loop
 * Handles audio streaming
 */
void loop() {
  // Main loop is now empty - all work is done in tasks
  vTaskDelay(1000 / portTICK_PERIOD_MS);  // Just keep the main loop alive
}

// ===== MULTI-CORE TASKS =====

// UI Task - runs on Core 1 (default Arduino core)
void uiTask(void* parameter) {
  Serial.println("UI Task started on Core 1");
  
  while (true) {
    static unsigned long lastDebugPrint = 0;
    
    // Debug output every 10 seconds
    if (millis() - lastDebugPrint > 10000) {
      Serial.printf("UI Task running on Core %d, Free heap: %d bytes\n", 
                   xPortGetCoreID(), ESP.getFreeHeap());
      lastDebugPrint = millis();
    }

    // Handle window interaction (minimize/close buttons)
    if (radioWindow.visible) {
      interactiveWindow(lcd, radioWindow);
    }
    
    // Handle desktop icon interaction  
    if (radioIcon.visible) {
      interactiveDesktopIcon(lcd, radioIcon);
    }

    // Note: Button interactions are now handled inside the window container
    // via onWindowContentClick callback - no need for individual button checks here

   
    vTaskDelay(10 / portTICK_PERIOD_MS);  // 10ms delay for UI responsiveness
  }
}

// Audio Task - runs on Core 0 (background core)
void audioTask(void* parameter) {
  Serial.println("Audio Task started on Core 0");
  
  // Initialize audio on this core
  // audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  // audio.setVolume(DEFAULT_VOLUME);
  
  while (true) {
    static unsigned long lastDebugPrint = 0;
    
    // Debug output every 15 seconds
    if (millis() - lastDebugPrint > 15000) {
      Serial.printf("Audio Task running on Core %d\n", xPortGetCoreID());
      lastDebugPrint = millis();
    }

    // Handle audio processing
    // if (isPlaying) {
    //   audio.loop();  // Audio processing
    // }
    
    // Handle WiFi and network tasks
    // WiFi.maintain();
    
    // Update clock (time-based, not UI critical)
    // updateClock();

    vTaskDelay(1 / portTICK_PERIOD_MS);  // 1ms delay for audio precision
  }
}

// Clean up resources when done (call this if you need to free memory)
void cleanup() {
  // Additional cleanup can be added here if needed
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

  // Initialize the radio window with its child components
  initializeRadioWindow();

  // Draw the window using the new window system
  // This will automatically draw all child buttons
  drawWindow(lcd, radioWindow);

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
  lcd.printf("Volume: %d", 10);  // Fixed value instead of audio.getVolume()

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

  // Child buttons are automatically drawn by the window system
  // No need to manually position or redraw buttons here
}

// ===== DYNAMIC WINDOW SETUP =====

void initializeRadioWindow() {
  // Clear any existing child buttons
  clearChildButtons(radioWindow);
  
  // Add all buttons to the window dynamically
  addChildButton(radioWindow, &btnPrev);
  addChildButton(radioWindow, &btnPlay);
  addChildButton(radioWindow, &btnNext);
  addChildButton(radioWindow, &btnVolUp);
  addChildButton(radioWindow, &btnVolDn);
  addChildButton(radioWindow, &btnStop);
  
  // Update button positions relative to window content area (not absolute screen coordinates)
  // Main playback controls - centered
  btnPrev.x = 20;
  btnPrev.y = 80;
  btnPlay.x = 80;
  btnPlay.y = 70;
  btnNext.x = 150;
  btnNext.y = 80;

  // Volume controls - right side
  btnVolUp.x = 300;
  btnVolUp.y = 70;
  btnVolDn.x = 300;
  btnVolDn.y = 110;

  // Stop button - below play button
  btnStop.x = 90;
  btnStop.y = 140;
}
