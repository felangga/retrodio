/*
 * Radio.ino - Internet Radio Player with Classic Mac OS UI
 * 
 * Copyright (c) 2025 Felangga
 * 
 * This Arduino sketch implements an internet radio player with a classic
 * Macintosh OS-style user interface using the MacUI library.
 */

#include "Arduino.h"
#include "esp_task_wdt.h"
#include "WiFi.h"
#include "Audio.h"
#include "esp32-hal-psram.h"
#include "wt32_sc01_plus.h"
#include "MacUI.h"
#include "config.h"
#include <time.h>

// ===== GLOBAL OBJECTS =====

const int btnStationID = 7;
const int btnAddStationID = 8;
const int btnSaveStationID = 9;
const int btnCancelAddStationID = 10;

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

// Station window callbacks
void onStationWindowMinimize();
void onStationWindowClose();
void onStationWindowContentClick(int relativeX, int relativeY);
void onStationWindowMoved();

// Add Station window callbacks
void onAddStationWindowMinimize();
void onAddStationWindowClose();
void onAddStationWindowContentClick(int relativeX, int relativeY);
void onAddStationWindowMoved();

// Main radio window - declared early so callbacks can reference it
MacWindow radioWindow{ 20, 40, 420, 240, "Radio", true, false, true, onWindowMinimize, onWindowClose, onWindowContentClick, onWindowMoved, nullptr, 0, false, 0, 0 };
MacWindow stationWindow{ 20, 40, 420, 240, "Station List", true, false, true, onStationWindowMinimize, onStationWindowClose, onStationWindowContentClick, onStationWindowMoved, nullptr, 0, false, 0, 0 };
MacWindow addStationWindow{ 60, 40, 360, 160, "Add Station", true, false, true, onAddStationWindowMinimize, onAddStationWindowClose, onAddStationWindowContentClick, onAddStationWindowMoved, nullptr, 0, false, 0, 0 };

// Desktop icon for minimized radio window
DesktopIcon radioIcon{ 50, 60, "Radio Player", "window", false, false, &radioWindow, onRadioIconClick };

// Global keyboard component (overlays bottom half of screen)
MacComponent* globalKeyboard = nullptr;

// ===== FUNCTION PROTOTYPES =====

void connectToWiFi();
// void initializeAudio();
void updateClock();

// Multi-core task functions
void uiTask(void* parameter);
void audioTask(void* parameter);

// Window setup functions
void initializeRadioWindow();
void initializeStationWindow();
void initializeAddStationWindow();

// Component interaction functions
void onComponentClick(int componentId);
MacComponent* findComponentById(const MacWindow& window, int id);

// ===== BUTTON DECLARATIONS =====
// Forward declarations for button callbacks
void onPlay();
void onStop();
void onVolUp();
void onVolDown();
void onPrev();
void onNext();

// ===== COMPONENT INTERACTION HANDLERS =====

void onComponentClick(int componentId) {
  Serial.printf("Component %d clicked\n", componentId);

  switch (componentId) {
    case 100:  // Now Playing Label
      Serial.println("Now Playing label clicked");
      displayStatus(lcd, "Label clicked", 160);
      break;

    case 101:  // Volume Slider
      Serial.println("Volume slider clicked");
      displayStatus(lcd, "Volume slider clicked", 160);
      // Here you could implement slider dragging logic
      break;

    case 102:  // Buffer Progress Bar
      Serial.println("Buffer progress clicked");
      displayStatus(lcd, "Progress bar clicked", 160);
      break;

    case 103:  // Auto Play Checkbox
      Serial.println("Auto Play checkbox clicked");
      // Toggle checkbox state
      {
        MacComponent* component = findComponentById(radioWindow, componentId);
        if (component && component->customData) {
          MacCheckBox* checkbox = (MacCheckBox*)component->customData;
          checkbox->checked = !checkbox->checked;
          // Redraw the component
          drawComponent(lcd, *component, radioWindow.x, radioWindow.y);
          displayStatus(lcd, checkbox->checked ? "Auto Play ON" : "Auto Play OFF", 160);
        }
      }
      break;

    case 104:  // Show Visuals Checkbox
      Serial.println("Show Visuals checkbox clicked");
      // Toggle checkbox state
      {
        MacComponent* component = findComponentById(radioWindow, componentId);
        if (component && component->customData) {
          MacCheckBox* checkbox = (MacCheckBox*)component->customData;
          checkbox->checked = !checkbox->checked;
          // Redraw the component
          drawComponent(lcd, *component, radioWindow.x, radioWindow.y);
          displayStatus(lcd, checkbox->checked ? "Visuals ON" : "Visuals OFF", 160);
        }
      }
      break;

    // Music player control buttons
    case 1:  // Play Button
      onPlay();
      break;
    case 2:  // Stop Button
      onStop();
      break;
    case 3:  // Volume Up Button
      onVolUp();
      break;
    case 4:  // Previous Button
      onPrev();
      break;
    case 5:  // Next Button
      onNext();
      break;
    case 6:  // Volume Down Button
      onVolDown();
      break;
    case btnStationID:  // Station List Button
      // Hide radio window immediately to stop event processing
      radioWindow.visible = false;
      
      // Wait for touch release to prevent button redraw on new window
      {
        int tx, ty;
        delay(200);
        while(lcd.getTouch(&tx, &ty)) { delay(10); }
        delay(50); // Extra delay to ensure touch system is clear
      }
      
      // Redraw background
      drawCheckeredPattern(lcd);
      drawMenuBar(lcd, "Retrodio");
      // Show station window
      stationWindow.visible = true;
      stationWindow.minimized = false;
      drawWindow(lcd, stationWindow);
      break;
    
    case btnAddStationID:  // Add Station Button
      Serial.println("Add Station button pressed");
      displayStatus(lcd, "Opening Add Station", 160);
      
      // Hide station window immediately to stop event processing
      stationWindow.visible = false;
      
      // Wait for touch release to prevent button redraw on new window
      {
        int tx, ty;
        delay(200);
        while(lcd.getTouch(&tx, &ty)) { delay(10); }
        delay(50); // Extra delay to ensure touch system is clear
      }
      
      // Show add station window
      addStationWindow.visible = true;
      addStationWindow.minimized = false;
      drawCheckeredPattern(lcd);
      drawMenuBar(lcd, "Retrodio");
      drawWindow(lcd, addStationWindow);
      break;
    
    case btnSaveStationID:  // Save Station Button
      Serial.println("Save Station button pressed");
      
      // Get the input values from the input fields
      {
        MacComponent* nameInputComp = findComponentById(addStationWindow, 401);
        MacComponent* urlInputComp = findComponentById(addStationWindow, 403);
        
        if (nameInputComp && urlInputComp) {
          MacInputField* nameInput = (MacInputField*)nameInputComp->customData;
          MacInputField* urlInput = (MacInputField*)urlInputComp->customData;
          
          String stationName = nameInput->text;
          String stationURL = urlInput->text;
          
          // Validate inputs
          if (stationName.length() > 0 && stationURL.length() > 0) {
            Serial.printf("Saving station: %s - %s\n", stationName.c_str(), stationURL.c_str());
            displayStatus(lcd, "Station Saved: " + stationName, 160);
            
            // TODO: Add station to list (expand stationItems array)
            // For now, just show success message
            
            // Clear input fields
            nameInput->text = "";
            nameInput->cursorPos = 0;
            urlInput->text = "";
            urlInput->cursorPos = 0;
          } else {
            displayStatus(lcd, "Please fill all fields", 160);
            return; // Don't close window if validation fails
          }
        }
      }
      
      // Hide keyboard and close add station window
      if (globalKeyboard) {
        MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
        keyboard->visible = false;
      }
      
      addStationWindow.visible = false;
      stationWindow.visible = true;
      drawCheckeredPattern(lcd);
      drawMenuBar(lcd, "Retrodio");
      drawWindow(lcd, stationWindow);
      break;
    
    case btnCancelAddStationID:  // Cancel Add Station Button
      Serial.println("Cancel Add Station button pressed");
      
      // Hide keyboard and close add station window
      if (globalKeyboard) {
        MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
        keyboard->visible = false;
      }
      
      addStationWindow.visible = false;
      stationWindow.visible = true;
      drawCheckeredPattern(lcd);
      drawMenuBar(lcd, "Retrodio");
      drawWindow(lcd, stationWindow);
      break;

    default:
      Serial.printf("Unknown component ID: %d\n", componentId);
      break;
  }
}

MacComponent* findComponentById(const MacWindow& window, int id) {
  if (window.childComponents == nullptr || window.childComponentCount == 0) {
    return nullptr;
  }

  for (int i = 0; i < window.childComponentCount; i++) {
    MacComponent* component = window.childComponents[i];
    if (component != nullptr && component->id == id) {
      return component;
    }
  }

  return nullptr;
}

void updateComponentSymbol(const MacWindow& window, int componentId, SymbolType newSymbol) {
  MacComponent* component = findComponentById(window, componentId);
  if (component != nullptr && component->type == COMPONENT_BUTTON && component->customData != nullptr) {
    MacButton* btnData = (MacButton*)component->customData;
    btnData->symbol = newSymbol;
    // Force redraw the component
    drawComponent(lcd, *component, window.x, window.y);
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
  //   updateComponentSymbol(radioWindow, 1, SYMBOL_PLAY);  // ID 1 is play button
  //   displayStatus(lcd, "Paused", 160);
  // } else {
  //   // Currently stopped - start playing
  //   audio.connecttohost(RADIO_URL.c_str());
  //   isPlaying = true;
  //   updateComponentSymbol(radioWindow, 1, SYMBOL_PAUSE);  // ID 1 is play button
  //   displayStatus(lcd, "Playing", 160);
  // }
  // Force button redraw to show new symbol
  updateComponentSymbol(radioWindow, 1, SYMBOL_PLAY);  // Just show play for now
}

void onStop() {
  Serial.println("Stop button pressed");
  displayStatus(lcd, "Stop pressed", 160);
  // audio.stopSong();
  // isPlaying = false;
  // updateComponentSymbol(radioWindow, 1, SYMBOL_PLAY);  // ID 1 is play button
  // displayStatus(lcd, "Stopped", 160);
  // Force button redraw to show play symbol
  updateComponentSymbol(radioWindow, 1, SYMBOL_PLAY);  // ID 1 is play button
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
  handleWindowMinimize(lcd, radioWindow, &radioIcon);
}

void onWindowClose() {
  handleWindowClose(lcd, radioWindow, &radioIcon);
}

void onRadioIconClick() {
  handleIconClick(lcd, radioWindow);
}

// Window content interaction callback
void onWindowContentClick(int relativeX, int relativeY) {
  handleWindowContentClick(lcd, radioWindow, relativeX, relativeY);
}

// Window moved callback
void onWindowMoved() {
  handleWindowMoved(lcd, radioWindow);
}

// Station window callbacks
void onStationWindowMinimize() {
  // Close station window and return to radio window
  stationWindow.visible = false;
  radioWindow.visible = true;
  drawCheckeredPattern(lcd);
  drawMenuBar(lcd, "Retrodio");
  drawWindow(lcd, radioWindow);
}

void onStationWindowClose() {
  // Close station window and return to radio window
  stationWindow.visible = false;
  radioWindow.visible = true;
  drawCheckeredPattern(lcd);
  drawMenuBar(lcd, "Retrodio");
  drawWindow(lcd, radioWindow);
}

void onStationWindowContentClick(int relativeX, int relativeY) {
  handleWindowContentClick(lcd, stationWindow, relativeX, relativeY);
}

void onStationWindowMoved() {
  handleWindowMoved(lcd, stationWindow);
}

// Add Station window callbacks
void onAddStationWindowMinimize() {
  // Hide keyboard if visible
  if (globalKeyboard) {
    MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
    keyboard->visible = false;
  }
  
  // Close add station window and return to station list
  addStationWindow.visible = false;
  stationWindow.visible = true;
  drawCheckeredPattern(lcd);
  drawMenuBar(lcd, "Retrodio");
  drawWindow(lcd, stationWindow);
}

void onAddStationWindowClose() {
  // Hide keyboard if visible
  if (globalKeyboard) {
    MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
    keyboard->visible = false;
  }
  
  // Close add station window and return to station list
  addStationWindow.visible = false;
  stationWindow.visible = true;
  drawCheckeredPattern(lcd);
  drawMenuBar(lcd, "Retrodio");
  drawWindow(lcd, stationWindow);
}

void onAddStationWindowContentClick(int relativeX, int relativeY) {
  // Find input components
  MacComponent* nameInputComp = nullptr;
  MacComponent* urlInputComp = nullptr;
  
  for (int i = 0; i < addStationWindow.childComponentCount; i++) {
    MacComponent* comp = addStationWindow.childComponents[i];
    if (comp->id == 401) {
      nameInputComp = comp;
    } else if (comp->id == 403) {
      urlInputComp = comp;
    }
  }
  
  if (globalKeyboard && nameInputComp && urlInputComp) {
    MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
    
    // Check if input fields were clicked to change focus
    if (relativeX >= nameInputComp->x && relativeX <= nameInputComp->x + nameInputComp->w &&
        relativeY >= nameInputComp->y && relativeY <= nameInputComp->y + nameInputComp->h) {
      // Switch to name input
      MacInputField* nameInput = (MacInputField*)nameInputComp->customData;
      MacInputField* urlInput = (MacInputField*)urlInputComp->customData;
      
      nameInput->focused = true;
      urlInput->focused = false;
      keyboard->targetInputId = 401;
      keyboard->visible = true;
      
      // Redraw both input fields and keyboard
      drawComponent(lcd, *nameInputComp, addStationWindow.x, addStationWindow.y);
      drawComponent(lcd, *urlInputComp, addStationWindow.x, addStationWindow.y);
      drawComponent(lcd, *globalKeyboard, 0, 0);
      
      // Wait for touch release to prevent immediate hide
      int tx, ty;
      delay(150);
      while(lcd.getTouch(&tx, &ty)) { delay(10); }
      return;
    }
    
    if (relativeX >= urlInputComp->x && relativeX <= urlInputComp->x + urlInputComp->w &&
        relativeY >= urlInputComp->y && relativeY <= urlInputComp->y + urlInputComp->h) {
      // Switch to URL input
      MacInputField* nameInput = (MacInputField*)nameInputComp->customData;
      MacInputField* urlInput = (MacInputField*)urlInputComp->customData;
      
      nameInput->focused = false;
      urlInput->focused = true;
      keyboard->targetInputId = 403;
      keyboard->visible = true;
      
      // Redraw both input fields and keyboard
      drawComponent(lcd, *nameInputComp, addStationWindow.x, addStationWindow.y);
      drawComponent(lcd, *urlInputComp, addStationWindow.x, addStationWindow.y);
      drawComponent(lcd, *globalKeyboard, 0, 0);
      
      // Wait for touch release to prevent immediate hide
      int tx, ty;
      delay(150);
      while(lcd.getTouch(&tx, &ty)) { delay(10); }
      return;
    }
    
    // If we reach here and keyboard is visible, user clicked something else (not input fields)
    // Hide the keyboard and unfocus all input fields
    if (keyboard->visible) {
      keyboard->visible = false;
      
      if (nameInputComp && nameInputComp->customData) {
        MacInputField* nameInput = (MacInputField*)nameInputComp->customData;
        nameInput->focused = false;
      }
      if (urlInputComp && urlInputComp->customData) {
        MacInputField* urlInput = (MacInputField*)urlInputComp->customData;
        urlInput->focused = false;
      }
      
      // Clear keyboard area
      int keyboardHeight = screenHeight / 2;
      int keyboardY = screenHeight - keyboardHeight;
      drawCheckeredPatternArea(lcd, 0, keyboardY, screenWidth, keyboardHeight);
      
      // Redraw the window and its components
      drawWindow(lcd, addStationWindow);
    }
  }
  
  // Handle other component clicks (buttons)
  handleWindowContentClick(lcd, addStationWindow, relativeX, relativeY);
}

void onAddStationWindowMoved() {
  handleWindowMoved(lcd, addStationWindow);
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
    tft.initDMA();
    tft.startWrite();
    
    Serial.println("LCD initialized");

    lcd.setRotation(lcd.getRotation() ^ 1);
    Serial.println("LCD rotation set");

    // Initialize sprite buffer for flicker-free component updates
    initComponentBuffer(&lcd, 420, 50);  // Max component size
    Serial.println("Sprite buffer initialized");

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
    uiTask,         // Task function
    "UI_Task",      // Task name
    10000,          // Stack size (bytes)
    NULL,           // Parameter
    1,              // Priority
    &uiTaskHandle,  // Task handle
    1               // Core (0 or 1)
  );

  // Create Audio task on Core 0 (background core)
  xTaskCreatePinnedToCore(
    audioTask,         // Task function
    "Audio_Task",      // Task name
    8000,              // Stack size (bytes)
    NULL,              // Parameter
    2,                 // Higher priority for audio
    &audioTaskHandle,  // Task handle
    0                  // Core (0 or 1)
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
  
  int touchX, touchY; // Touch coordinates
  bool keyboardWasVisible = false; // Track keyboard state

  while (true) {
    static unsigned long lastDebugPrint = 0;

    // Debug output every 10 seconds
    if (millis() - lastDebugPrint > 10000) {
      Serial.printf("UI Task running on Core %d, Free heap: %d bytes\n",
                    xPortGetCoreID(), ESP.getFreeHeap());
      lastDebugPrint = millis();
    }

    // Handle global keyboard input first if visible
    if (globalKeyboard) {
      MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
      if (keyboard->visible && lcd.getTouch(&touchX, &touchY)) {
        // Check if touch is within keyboard area
        int keyboardHeight = screenHeight / 2;
        int keyboardY = screenHeight - keyboardHeight;
        
        if (touchY >= keyboardY) {
          // Touch is within keyboard area - handle keyboard input
          MacComponent* activeInputComp = nullptr;
          if (addStationWindow.visible) {
            for (int i = 0; i < addStationWindow.childComponentCount; i++) {
              MacComponent* comp = addStationWindow.childComponents[i];
              if (comp->id == keyboard->targetInputId) {
                activeInputComp = comp;
                break;
              }
            }
          }
          
          if (activeInputComp && handleKeyboardTouch(lcd, globalKeyboard, activeInputComp, touchX, touchY)) {
            // Redraw the active input field after keyboard input
            drawComponent(lcd, *activeInputComp, addStationWindow.x, addStationWindow.y);
            delay(100); // Debounce
            while(lcd.getTouch(&touchX, &touchY)) { delay(10); } // Wait for release
            continue;
          }
        } else {
          // Touch is outside keyboard area - hide keyboard and unfocus inputs
          keyboard->visible = false;
          
          // Unfocus all input fields
          if (addStationWindow.visible) {
            for (int i = 0; i < addStationWindow.childComponentCount; i++) {
              MacComponent* comp = addStationWindow.childComponents[i];
              if (comp->type == COMPONENT_INPUT_FIELD && comp->customData) {
                MacInputField* inputField = (MacInputField*)comp->customData;
                inputField->focused = false;
                drawComponent(lcd, *comp, addStationWindow.x, addStationWindow.y);
              }
            }
          }
          
          // Redraw desktop area where keyboard was
          drawCheckeredPatternArea(lcd, 0, keyboardY, screenWidth, keyboardHeight);
          
          // Redraw any visible windows
          if (addStationWindow.visible) drawWindow(lcd, addStationWindow);
          if (stationWindow.visible) drawWindow(lcd, stationWindow);
          if (radioWindow.visible) drawWindow(lcd, radioWindow);
          
          delay(100); // Debounce
          while(lcd.getTouch(&touchX, &touchY)) { delay(10); } // Wait for release
        }
      }
    }
    
    // Handle window interaction (minimize/close buttons)
    // Only handle one window at a time - add station window takes priority, then station window, then radio window
    if (addStationWindow.visible) {
      interactiveWindow(lcd, addStationWindow);
    } else if (stationWindow.visible) {
      interactiveWindow(lcd, stationWindow);
    } else if (radioWindow.visible) {
      interactiveWindow(lcd, radioWindow);
    }

    // Handle desktop icon interaction
    if (radioIcon.visible) {
      interactiveDesktopIcon(lcd, radioIcon);
    }

    // Update running text components for animation
    if (radioWindow.visible && !stationWindow.visible && !addStationWindow.visible) {
      updateRunningTextComponents(lcd, radioWindow);
    }
    
    if (stationWindow.visible && !addStationWindow.visible) {
      updateRunningTextComponents(lcd, stationWindow);
    }
    
    if (addStationWindow.visible) {
      updateRunningTextComponents(lcd, addStationWindow);
      updateInputFieldComponents(lcd, addStationWindow);
    }
    
    // Handle global keyboard visibility changes
    if (globalKeyboard) {
      MacKeyboard* keyboard = (MacKeyboard*)globalKeyboard->customData;
      
      if (keyboard->visible && !keyboardWasVisible) {
        // Keyboard just became visible - draw it
        drawComponent(lcd, *globalKeyboard, 0, 0);
        keyboardWasVisible = true;
      } else if (!keyboard->visible && keyboardWasVisible) {
        // Keyboard just became hidden - clear the area
        int keyboardHeight = screenHeight / 2;
        int keyboardY = screenHeight - keyboardHeight;
        drawCheckeredPatternArea(lcd, 0, keyboardY, screenWidth, keyboardHeight);
        
        // Redraw any windows that overlap with keyboard area
        if (addStationWindow.visible && 
            (addStationWindow.y + addStationWindow.h > keyboardY)) {
          drawWindow(lcd, addStationWindow);
        }
        if (stationWindow.visible && 
            (stationWindow.y + stationWindow.h > keyboardY)) {
          drawWindow(lcd, stationWindow);
        }
        if (radioWindow.visible && 
            (radioWindow.y + radioWindow.h > keyboardY)) {
          drawWindow(lcd, radioWindow);
        }
        
        keyboardWasVisible = false;
      }
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

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

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

  if (audio.connecttohost(RADIO_URL)) {
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
  drawMenuBar(lcd, "Retrodio");

  // Initialize the radio window with its child components
  initializeRadioWindow();
  initializeStationWindow();
  initializeAddStationWindow();
  
  // Initialize global keyboard (bottom half of screen)
  if (globalKeyboard == nullptr) {
    int keyboardHeight = screenHeight / 2;
    int keyboardY = screenHeight - keyboardHeight;
    globalKeyboard = createKeyboardComponent(0, keyboardY, screenWidth, keyboardHeight, 404, 401);
    // Keyboard starts hidden
    MacKeyboard* kb = (MacKeyboard*)globalKeyboard->customData;
    kb->visible = false;
  }

  // Draw the window using the new window system
  // This will automatically draw all child buttons
  drawWindow(lcd, radioWindow);
  
  // Station and add station windows start hidden
  stationWindow.visible = false;
  addStationWindow.visible = false;

  // Only draw content if window is not minimized
  if (!radioWindow.minimized && radioWindow.visible) {
    redrawWindowContent(lcd, radioWindow);
  }
}

// Helper function to redraw window content (needed for dragging)
void redrawWindowContent(lgfx::LGFX_Device& lcd, const MacWindow& window) {
  if (!window.visible || window.minimized) return;

  // Draw now playing info area at the top
  // draw3DFrame(lcd, window.x + 10, window.y + 35, 200, 25, true);
  // lcd.setTextColor(MAC_BLACK, MAC_WHITE);
  // lcd.setTextSize(1);
  // lcd.setCursor(window.x + 15, window.y + 43);
  // if (isPlaying) {
  //   lcd.println("♪ Now Playing: Radio Stream");
  // } else {
  //   lcd.println("Radio Ready - Press Play");
  // }

  // Draw spectrum visualization next to the info
  // drawSpectrumVisualization(lcd, window.x + 220, window.y + 35, 80, 25, isPlaying);

  // Draw volume indicator
  // draw3DFrame(lcd, window.x + 310, window.y + 35, 90, 25, true);
  // lcd.setCursor(window.x + 315, window.y + 43);
  // lcd.printf("Volume: %d", 10);  // Fixed value instead of audio.getVolume()

  // // Draw a fake progress bar area (since it's streaming, we'll show activity)
  // draw3DFrame(lcd, window.x + 20, window.y + 160, 280, 8, true);
  // if (isPlaying) {
  //   // Show some activity in the progress bar
  //   static int progressPos = 0;
  //   progressPos = (progressPos + 5) % 260;
  //   lcd.fillRect(window.x + 30 + progressPos, window.y + 162, 20, 4, MAC_BLUE);
  // }

  // // Draw status area at the bottom
  // draw3DFrame(lcd, window.x + 20, window.y + 190, 280, 30, true);
  // lcd.setCursor(window.x + 25, window.y + 200);
  // if (isPlaying) {
  //   lcd.println("♪ Streaming... Internet Radio v1.0");
  // } else {
  //   lcd.println("Ready to play - Internet Radio v1.0");
  // }

}

// ===== DYNAMIC WINDOW SETUP =====

void initializeRadioWindow() {
  // Clear any existing child components
  clearChildComponents(radioWindow);

  // Add a label for the now playing area
  MacComponent* nowPlayingLabel = createLabelComponent(15, 40, 180, 20, 100, "Now Playing:", MAC_BLACK);
  nowPlayingLabel->onClick = onComponentClick;
  addChildComponent(radioWindow, nowPlayingLabel);

  // Main playback controls - larger and centered
  MacComponent* btnPrev = createButtonComponent(30, 165, 50, 50, 4, "", SYMBOL_PREV);
  btnPrev->onClick = onComponentClick;
  addChildComponent(radioWindow, btnPrev);

  MacComponent* btnPlay = createButtonComponent(80, 160, 60, 60, 1, "", SYMBOL_PLAY);
  btnPlay->onClick = onComponentClick;
  addChildComponent(radioWindow, btnPlay);

  MacComponent* btnStation = createButtonComponent(300, 165, 50, 50, btnStationID, "", SYMBOL_LIST);
  btnStation->onClick = onComponentClick;
  addChildComponent(radioWindow, btnStation);

  MacComponent* btnNext = createButtonComponent(140, 165, 50, 50, 5, "", SYMBOL_NEXT);
  btnNext->onClick = onComponentClick;
  addChildComponent(radioWindow, btnNext);

  MacComponent* txtRadioName = createRunningTextComponent(
    10, 40,           // x, y position
    400, 25,          // width, height
    200,              // component ID
    "Swaragama FM",
    2,                // scroll speed (2 pixels per update)
    MAC_BLACK,        // text color
    3                 // text size
  );
  txtRadioName->onClick = onComponentClick;
  addChildComponent(radioWindow, txtRadioName);

  MacComponent* txtRadioDetails = createRunningTextComponent(
    10, 70,           // x, y position
    400, 15,          // width, height
    201,              // component ID
    "Now Playing: Internet Radio Stream - Enjoy your favorite music and shows all day long!",
    2,                // scroll speed (2 pixels per update)
    MAC_BLACK,        // text color
    1                 // text size
  );
  txtRadioDetails->onClick = onComponentClick;
  addChildComponent(radioWindow, txtRadioDetails);

  // Volume controls - smaller, positioned on the right
  // MacComponent* btnVolUp = createButtonComponent(300, 70, 45, 35, 3, "", SYMBOL_VOL_UP);
  // btnVolUp->onClick = onComponentClick;
  // addChildComponent(radioWindow, btnVolUp);

  // MacComponent* btnVolDn = createButtonComponent(300, 110, 45, 35, 6, "", SYMBOL_VOL_DOWN);
  // btnVolDn->onClick = onComponentClick;
  // addChildComponent(radioWindow, btnVolDn);

  // // Stop button - separate and smaller
  // MacComponent* btnStop = createButtonComponent(90, 140, 40, 40, 2, "", SYMBOL_STOP);
  // btnStop->onClick = onComponentClick;
  // addChildComponent(radioWindow, btnStop);
}

// ===== STATION LIST DATA =====
// Define your radio stations here
MacListViewItem stationItems[] = {
  {"Swaragama FM - Jogja", nullptr},
  {"Prambors FM - Jakarta", nullptr},
  {"Gen FM - Semarang", nullptr},
  {"Trijaya FM - Jakarta", nullptr},
  {"Radio Elshinta - Jakarta", nullptr},
  {"Radio Sonora - Jakarta", nullptr},
  {"MNC Trijaya FM", nullptr},
  {"Hard Rock FM", nullptr},
  {"RRI Pro 1", nullptr},
  {"Ardan FM", nullptr}
};

const int stationItemCount = sizeof(stationItems) / sizeof(stationItems[0]);

void onStationItemClick(int index, void* itemData) {
  Serial.printf("Station selected: %d - %s\n", index, stationItems[index].text.c_str());
  // Here you can add logic to switch to the selected station
  // For example: switchToStation(index);
  
  // Close station window and show radio window
  stationWindow.visible = false;
  radioWindow.visible = true;
  drawCheckeredPattern(lcd);
  drawMenuBar(lcd, "Retrodio");
  drawWindow(lcd, radioWindow);
}

void initializeStationWindow() {
  // Clear any existing child components
  clearChildComponents(stationWindow);
  
  // Add "Add Station" button at the bottom
  MacComponent* btnAddStation = createButtonComponent(310, 35, 90, 30, btnAddStationID, "Add +");
  btnAddStation->onClick = onComponentClick;
  addChildComponent(stationWindow, btnAddStation);
  
  // Create the list view component (reduced width to accommodate button)
  MacComponent* stationList = createListViewComponent(
    10, 35,           // x, y position (below title bar)
    290, 195,         // width, height (reduced from 400)
    300,              // component ID
    stationItems,     // array of items
    stationItemCount, // number of items
    30                // item height
  );
  
  // Set the item click callback
  if (stationList && stationList->customData) {
    MacListView* listViewData = (MacListView*)stationList->customData;
    listViewData->onItemClick = onStationItemClick;
  }
  
  stationList->onClick = onComponentClick;
  addChildComponent(stationWindow, stationList);
}

void initializeAddStationWindow() {
  // Clear any existing child components
  clearChildComponents(addStationWindow);
  
  // Add labels for field names
  MacComponent* lblStationName = createLabelComponent(20, 40, 120, 20, 400, "Station Name:");
  addChildComponent(addStationWindow, lblStationName);
  
  // Add input field for station name
  MacComponent* txtStationName = createInputFieldComponent(150, 40, 190, 25, 401, "Enter station name", 50);
  txtStationName->onClick = onComponentClick;
  addChildComponent(addStationWindow, txtStationName);
  
  MacComponent* lblStationURL = createLabelComponent(20, 75, 120, 20, 402, "Station URL:");
  addChildComponent(addStationWindow, lblStationURL);
  
  // Add input field for station URL
  MacComponent* txtStationURL = createInputFieldComponent(150, 75, 190, 25, 403, "https://...", 200);
  txtStationURL->onClick = onComponentClick;
  addChildComponent(addStationWindow, txtStationURL);
  
  // Add Save and Cancel buttons
  MacComponent* btnSave = createButtonComponent(80, 115, 80, 30, btnSaveStationID, "Save");
  btnSave->onClick = onComponentClick;
  addChildComponent(addStationWindow, btnSave);
  
  MacComponent* btnCancel = createButtonComponent(180, 115, 80, 30, btnCancelAddStationID, "Cancel");
  btnCancel->onClick = onComponentClick;
  addChildComponent(addStationWindow, btnCancel);
}
