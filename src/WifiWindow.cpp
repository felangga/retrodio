/*
 * WifiWindow.cpp - WiFi Window Implementation
 *
 * Copyright (c) 2025 felangga
 *
 * This file implements WiFi window for scanning and connecting to WiFi networks
 */

#include "WifiWindow.h"
#include "GlobalState.h"
#include "WindowCallbacks.h"
#include "UIHelpers.h"
#include "ConfigManager.h"
#include "RadioWindow.h"
#include "wt32_sc01_plus.h"
#include <WiFi.h>

// WiFi network list items
static MacListViewItem* wifiItems = nullptr;
static int wifiItemCount = 0;
static String* wifiSSIDs = nullptr;  // Store SSIDs separately for connection
static int* wifiRSSIs = nullptr;     // Store signal strengths
static bool* wifiSecure = nullptr;   // Store security status

// Currently selected network
static int selectedWifiIndex = -1;
static String selectedSSID = "";
static bool selectedIsSecure = false;

// Scanning state
static bool isScanning = false;

// Connection state
static bool isConnecting = false;
static String connectingSSID = "";
static String connectingPassword = "";
static unsigned long connectionStartTime = 0;
static const unsigned long CONNECTION_TIMEOUT = 10000;  // 10 seconds timeout

// Forward declarations
void onWifiItemClick(int index, void* itemData);
void onWifiConnectButtonClick();
void onWifiCancelButtonClick();
void onWifiRefreshButtonClick();
void onWifiPasswordSaveClick();
void onWifiPasswordCancelClick();

void cleanupWifiList() {
  if (wifiItems != nullptr) {
    delete[] wifiItems;
    wifiItems = nullptr;
  }
  if (wifiSSIDs != nullptr) {
    delete[] wifiSSIDs;
    wifiSSIDs = nullptr;
  }
  if (wifiRSSIs != nullptr) {
    delete[] wifiRSSIs;
    wifiRSSIs = nullptr;
  }
  if (wifiSecure != nullptr) {
    delete[] wifiSecure;
    wifiSecure = nullptr;
  }
  wifiItemCount = 0;
}

String getRSSIBars(int rssi) {
  if (rssi >= -50) return "[****]";
  if (rssi >= -60) return "[*** ]";
  if (rssi >= -70) return "[**  ]";
  if (rssi >= -80) return "[*   ]";
  return "[    ]";
}

void scanWifiNetworks() {
  extern LGFX lcd;
  extern MacWindow wifiWindow;
  extern const int WIFI_LIST_COMPONENT;

  isScanning = true;

  // Show scanning notification
  showNotification("Scanning WiFi...");

  // Clean up previous scan results
  cleanupWifiList();

  // Start WiFi scan
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  int n = WiFi.scanNetworks();

  hideNotification();
  isScanning = false;

  if (n <= 0) {
    wifiItemCount = 1;
    wifiItems = new MacListViewItem[1];
    wifiItems[0].text = "No networks found";
    wifiItems[0].data = nullptr;
    wifiSSIDs = new String[1];
    wifiSSIDs[0] = "";
    wifiRSSIs = new int[1];
    wifiRSSIs[0] = -100;
    wifiSecure = new bool[1];
    wifiSecure[0] = false;
  } else {
    // Filter duplicates and sort by signal strength
    wifiItemCount = 0;
    String* tempSSIDs = new String[n];
    int* tempRSSIs = new int[n];
    bool* tempSecure = new bool[n];

    for (int i = 0; i < n; i++) {
      String ssid = WiFi.SSID(i);
      if (ssid.length() == 0) continue;

      // Check for duplicate
      bool isDuplicate = false;
      for (int j = 0; j < wifiItemCount; j++) {
        if (tempSSIDs[j] == ssid) {
          // Keep the one with stronger signal
          if (WiFi.RSSI(i) > tempRSSIs[j]) {
            tempRSSIs[j] = WiFi.RSSI(i);
            tempSecure[j] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
          }
          isDuplicate = true;
          break;
        }
      }

      if (!isDuplicate) {
        tempSSIDs[wifiItemCount] = ssid;
        tempRSSIs[wifiItemCount] = WiFi.RSSI(i);
        tempSecure[wifiItemCount] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
        wifiItemCount++;
      }
    }

    // Sort by signal strength (bubble sort)
    for (int i = 0; i < wifiItemCount - 1; i++) {
      for (int j = 0; j < wifiItemCount - i - 1; j++) {
        if (tempRSSIs[j] < tempRSSIs[j + 1]) {
          // Swap
          String ts = tempSSIDs[j];
          tempSSIDs[j] = tempSSIDs[j + 1];
          tempSSIDs[j + 1] = ts;

          int tr = tempRSSIs[j];
          tempRSSIs[j] = tempRSSIs[j + 1];
          tempRSSIs[j + 1] = tr;

          bool tb = tempSecure[j];
          tempSecure[j] = tempSecure[j + 1];
          tempSecure[j + 1] = tb;
        }
      }
    }

    // Create final arrays
    wifiSSIDs = new String[wifiItemCount];
    wifiRSSIs = new int[wifiItemCount];
    wifiSecure = new bool[wifiItemCount];
    wifiItems = new MacListViewItem[wifiItemCount];

    for (int i = 0; i < wifiItemCount; i++) {
      wifiSSIDs[i] = tempSSIDs[i];
      wifiRSSIs[i] = tempRSSIs[i];
      wifiSecure[i] = tempSecure[i];

      // Format display text: "SSID [****] (secure)"
      String displayText = tempSSIDs[i];
      if (displayText.length() > 18) {
        displayText = displayText.substring(0, 15) + "...";
      }
      displayText += " " + getRSSIBars(tempRSSIs[i]);
      if (tempSecure[i]) {
        displayText += " *";
      }

      wifiItems[i].text = displayText;
      wifiItems[i].data = (void*)(intptr_t)i;
    }

    delete[] tempSSIDs;
    delete[] tempRSSIs;
    delete[] tempSecure;
  }

  // Clean up WiFi scan results
  WiFi.scanDelete();

  // Update the ListView component
  updateWifiListDisplay();
}

void updateWifiListDisplay() {
  extern MacWindow wifiWindow;
  extern LGFX lcd;
  extern const int WIFI_LIST_COMPONENT;

  MacComponent* listComp = findComponentById(wifiWindow, WIFI_LIST_COMPONENT);
  if (listComp && listComp->customData) {
    MacListView* listViewData = (MacListView*)listComp->customData;
    listViewData->items = wifiItems;
    listViewData->itemCount = wifiItemCount;
    listViewData->selectedIndex = -1;
    listViewData->scrollOffset = 0;
    listViewData->needsFullRedraw = true;

    if (wifiWindow.visible) {
      lcd.startWrite();
      drawComponent(lcd, *listComp, wifiWindow.x, wifiWindow.y);
      lcd.endWrite();
    }
  }
}

void onWifiItemClick(int index, void* itemData) {
  extern MacWindow wifiWindow;
  extern LGFX lcd;
  extern const int BTN_WIFI_CONNECT;

  if (index < 0 || index >= wifiItemCount) return;
  if (wifiSSIDs[index].length() == 0) return;  // "No networks found"

  selectedWifiIndex = index;
  selectedSSID = wifiSSIDs[index];
  selectedIsSecure = wifiSecure[index];

  // Enable connect button
  MacComponent* btnConnect = findComponentById(wifiWindow, BTN_WIFI_CONNECT);
  if (btnConnect) {
    btnConnect->enabled = true;
    lcd.startWrite();
    drawComponent(lcd, *btnConnect, wifiWindow.x, wifiWindow.y);
    lcd.endWrite();
  }
}

void initializeWifiWindow() {
  extern MacWindow wifiWindow;
  extern const int WIFI_LIST_COMPONENT;
  extern const int BTN_WIFI_CONNECT;
  extern const int BTN_WIFI_CANCEL;
  extern const int BTN_WIFI_REFRESH;
  extern const int LBL_WIFI_TITLE;
  extern const int INPUT_WIFI_PASSWORD;
  extern const int LBL_WIFI_PASSWORD;
  extern const int BTN_WIFI_PASSWORD_OK;
  extern const int BTN_WIFI_PASSWORD_CANCEL;

  clearChildComponents(wifiWindow);

  // Title label
  MacComponent* lblTitle = createLabelComponent(10, 42, 200, 20, LBL_WIFI_TITLE, "Select a network:");
  MacLabel* labelData = (MacLabel*)lblTitle->customData;
  labelData->font = FONT_CHICAGO_9PT;
  addChildComponent(wifiWindow, lblTitle);

  // WiFi networks list
  MacComponent* wifiList = createListViewComponent(10, 60, 280, 120, WIFI_LIST_COMPONENT,
                                                   wifiItems, wifiItemCount, 24);
  if (wifiList && wifiList->customData) {
    MacListView* listViewData = (MacListView*)wifiList->customData;
    listViewData->onItemClick = onWifiItemClick;
    listViewData->font = FONT_CHICAGO_9PT;
  }
  addChildComponent(wifiWindow, wifiList);

  // Refresh button
  MacComponent* btnRefresh = createButtonComponent(10, 185, 80, 28, BTN_WIFI_REFRESH, "Refresh");
  btnRefresh->onClick = [](int componentId) { onWifiRefreshButtonClick(); };
  addChildComponent(wifiWindow, btnRefresh);

  // Connect button (initially disabled until network selected)
  MacComponent* btnConnect = createButtonComponent(120, 185, 80, 28, BTN_WIFI_CONNECT, "Connect");
  btnConnect->onClick = [](int componentId) { onWifiConnectButtonClick(); };
  btnConnect->enabled = false;
  addChildComponent(wifiWindow, btnConnect);

  // Cancel button
  MacComponent* btnCancel = createButtonComponent(210, 185, 80, 28, BTN_WIFI_CANCEL, "Cancel");
  btnCancel->onClick = [](int componentId) { onWifiCancelButtonClick(); };
  addChildComponent(wifiWindow, btnCancel);

  // Password entry components (initially hidden - will be shown when needed)
  MacComponent* lblPassword = createLabelComponent(10, 42, 280, 20, LBL_WIFI_PASSWORD, "Enter password for:");
  labelData = (MacLabel*)lblPassword->customData;
  labelData->font = FONT_CHICAGO_9PT;
  lblPassword->visible = false;
  addChildComponent(wifiWindow, lblPassword);

  MacComponent* txtPassword = createInputFieldComponent(10, 70, 280, 28, INPUT_WIFI_PASSWORD, "Password", 64);
  txtPassword->visible = false;
  addChildComponent(wifiWindow, txtPassword);

  MacComponent* btnPasswordOk = createButtonComponent(120, 110, 80, 28, BTN_WIFI_PASSWORD_OK, "Connect");
  btnPasswordOk->onClick = [](int componentId) { onWifiPasswordSaveClick(); };
  btnPasswordOk->visible = false;
  addChildComponent(wifiWindow, btnPasswordOk);

  MacComponent* btnPasswordCancel = createButtonComponent(210, 110, 80, 28, BTN_WIFI_PASSWORD_CANCEL, "Cancel");
  btnPasswordCancel->onClick = [](int componentId) { onWifiPasswordCancelClick(); };
  btnPasswordCancel->visible = false;
  addChildComponent(wifiWindow, btnPasswordCancel);

  // Reset state
  selectedWifiIndex = -1;
  selectedSSID = "";
  selectedIsSecure = false;
}

void showWifiPasswordEntry() {
  extern MacWindow wifiWindow;
  extern LGFX lcd;
  extern const int WIFI_LIST_COMPONENT;
  extern const int BTN_WIFI_CONNECT;
  extern const int BTN_WIFI_CANCEL;
  extern const int BTN_WIFI_REFRESH;
  extern const int LBL_WIFI_TITLE;
  extern const int INPUT_WIFI_PASSWORD;
  extern const int LBL_WIFI_PASSWORD;
  extern const int BTN_WIFI_PASSWORD_OK;
  extern const int BTN_WIFI_PASSWORD_CANCEL;

  // Hide list view components
  MacComponent* listComp = findComponentById(wifiWindow, WIFI_LIST_COMPONENT);
  if (listComp) listComp->visible = false;

  MacComponent* btnRefresh = findComponentById(wifiWindow, BTN_WIFI_REFRESH);
  if (btnRefresh) btnRefresh->visible = false;

  MacComponent* btnConnect = findComponentById(wifiWindow, BTN_WIFI_CONNECT);
  if (btnConnect) btnConnect->visible = false;

  MacComponent* btnCancel = findComponentById(wifiWindow, BTN_WIFI_CANCEL);
  if (btnCancel) btnCancel->visible = false;

  MacComponent* lblTitle = findComponentById(wifiWindow, LBL_WIFI_TITLE);
  if (lblTitle) lblTitle->visible = false;

  // Show password entry components
  MacComponent* lblPassword = findComponentById(wifiWindow, LBL_WIFI_PASSWORD);
  if (lblPassword && lblPassword->customData) {
    MacLabel* labelData = (MacLabel*)lblPassword->customData;
    labelData->text = "Password for: " + selectedSSID;
    lblPassword->visible = true;
  }

  MacComponent* txtPassword = findComponentById(wifiWindow, INPUT_WIFI_PASSWORD);
  if (txtPassword) {
    txtPassword->visible = true;
    if (txtPassword->customData) {
      MacInputField* inputData = (MacInputField*)txtPassword->customData;
      inputData->text = "";
      inputData->cursorPos = 0;
      inputData->focused = true;
    }
  }

  MacComponent* btnPasswordOk = findComponentById(wifiWindow, BTN_WIFI_PASSWORD_OK);
  if (btnPasswordOk) btnPasswordOk->visible = true;

  MacComponent* btnPasswordCancel = findComponentById(wifiWindow, BTN_WIFI_PASSWORD_CANCEL);
  if (btnPasswordCancel) btnPasswordCancel->visible = true;

  // Redraw window
  lcd.startWrite();
  drawWindow(lcd, wifiWindow);
  lcd.endWrite();

  // Show keyboard
  extern MacComponent* wifiKeyboard;
  if (wifiKeyboard && wifiKeyboard->customData) {
    MacKeyboard* kb = (MacKeyboard*)wifiKeyboard->customData;
    kb->visible = true;
    kb->targetInputId = INPUT_WIFI_PASSWORD;

    lcd.startWrite();
    drawComponent(lcd, *wifiKeyboard, wifiKeyboard->x, wifiKeyboard->y);
    lcd.endWrite();
  }
}

void hideWifiPasswordEntry() {
  extern MacWindow wifiWindow;
  extern LGFX lcd;
  extern const int WIFI_LIST_COMPONENT;
  extern const int BTN_WIFI_CONNECT;
  extern const int BTN_WIFI_CANCEL;
  extern const int BTN_WIFI_REFRESH;
  extern const int LBL_WIFI_TITLE;
  extern const int INPUT_WIFI_PASSWORD;
  extern const int LBL_WIFI_PASSWORD;
  extern const int BTN_WIFI_PASSWORD_OK;
  extern const int BTN_WIFI_PASSWORD_CANCEL;

  // Hide keyboard
  extern MacComponent* wifiKeyboard;
  if (wifiKeyboard && wifiKeyboard->customData) {
    MacKeyboard* kb = (MacKeyboard*)wifiKeyboard->customData;
    kb->visible = false;

    // Clear keyboard area
    int keyboardHeight = screenHeight / 2;
    int keyboardY = screenHeight - keyboardHeight;
    drawCheckeredPatternArea(lcd, 0, keyboardY, screenWidth, keyboardHeight);
  }

  // Hide password entry components
  MacComponent* lblPassword = findComponentById(wifiWindow, LBL_WIFI_PASSWORD);
  if (lblPassword) lblPassword->visible = false;

  MacComponent* txtPassword = findComponentById(wifiWindow, INPUT_WIFI_PASSWORD);
  if (txtPassword) {
    txtPassword->visible = false;
    if (txtPassword->customData) {
      MacInputField* inputData = (MacInputField*)txtPassword->customData;
      inputData->focused = false;
    }
  }

  MacComponent* btnPasswordOk = findComponentById(wifiWindow, BTN_WIFI_PASSWORD_OK);
  if (btnPasswordOk) btnPasswordOk->visible = false;

  MacComponent* btnPasswordCancel = findComponentById(wifiWindow, BTN_WIFI_PASSWORD_CANCEL);
  if (btnPasswordCancel) btnPasswordCancel->visible = false;

  // Show list view components
  MacComponent* listComp = findComponentById(wifiWindow, WIFI_LIST_COMPONENT);
  if (listComp) listComp->visible = true;

  MacComponent* btnRefresh = findComponentById(wifiWindow, BTN_WIFI_REFRESH);
  if (btnRefresh) btnRefresh->visible = true;

  MacComponent* btnConnect = findComponentById(wifiWindow, BTN_WIFI_CONNECT);
  if (btnConnect) btnConnect->visible = true;

  MacComponent* btnCancel = findComponentById(wifiWindow, BTN_WIFI_CANCEL);
  if (btnCancel) btnCancel->visible = true;

  MacComponent* lblTitle = findComponentById(wifiWindow, LBL_WIFI_TITLE);
  if (lblTitle) lblTitle->visible = true;

  // Redraw window
  lcd.startWrite();
  drawWindow(lcd, wifiWindow);
  lcd.endWrite();
}

void onWifiRefreshButtonClick() {
  scanWifiNetworks();
}

void onWifiConnectButtonClick() {
  if (selectedWifiIndex < 0 || selectedSSID.length() == 0) return;

  if (selectedIsSecure) {
    // Need password - show password entry
    showWifiPasswordEntry();
  } else {
    // Open network - connect directly
    connectToSelectedWifi();
  }
}

void onWifiCancelButtonClick() {
  extern MacWindow wifiWindow;
  extern MacWindow radioWindow;
  extern LGFX lcd;

  wifiWindow.visible = false;
  wifiWindow.active = false;

  // Clean up
  cleanupWifiList();

  // Redraw and restore radio window
  lcd.startWrite();
  drawCheckeredPatternArea(lcd, wifiWindow.x, wifiWindow.y, wifiWindow.w + 5, wifiWindow.h + 5);
  radioWindow.visible = true;
  radioWindow.active = true;
  drawWindow(lcd, radioWindow);
  lcd.endWrite();
}

void onWifiPasswordSaveClick() {
  extern MacWindow wifiWindow;
  extern const int INPUT_WIFI_PASSWORD;

  MacComponent* txtPassword = findComponentById(wifiWindow, INPUT_WIFI_PASSWORD);
  if (txtPassword && txtPassword->customData) {
    MacInputField* inputData = (MacInputField*)txtPassword->customData;
    String password = inputData->text;

    // Hide password entry first
    hideWifiPasswordEntry();

    // Connect with password
    connectToSelectedWifi(password);
  }
}

void onWifiPasswordCancelClick() {
  hideWifiPasswordEntry();
}

void connectToSelectedWifi() {
  connectToSelectedWifi("");
}

void connectToSelectedWifi(const String& password) {
  if (selectedSSID.length() == 0) return;

  // Store connection details
  connectingSSID = selectedSSID;
  connectingPassword = password;
  isConnecting = true;
  connectionStartTime = millis();

  // Stop radio if playing before attempting connection
  extern volatile bool isPlaying;
  extern QueueHandle_t audioCommandQueue;

  if (isPlaying && audioCommandQueue != nullptr) {
    AudioCommandMsg msg = {CMD_STOP, ""};
    if (xQueueSend(audioCommandQueue, &msg, pdMS_TO_TICKS(100)) == pdTRUE) {
      isPlaying = false;
      updateComponentSymbol(radioWindow, 1, SYMBOL_PLAY);
    }
  }

  // Start connection attempt
  WiFi.disconnect();
  if (connectingPassword.length() > 0) {
    WiFi.begin(connectingSSID.c_str(), connectingPassword.c_str());
  } else {
    WiFi.begin(connectingSSID.c_str());
  }

  showNotification("Connecting...");
}

bool isWifiConnecting() {
  return isConnecting;
}

void cancelWifiConnection() {
  if (isConnecting) {
    isConnecting = false;
    connectingSSID = "";
    connectingPassword = "";
    WiFi.disconnect();
    hideNotification();
  }
}

void updateWifiConnectionStatus() {
  if (!isConnecting) return;

  // Check if connected
  if (WiFi.status() == WL_CONNECTED) {
    extern LGFX lcd;
    extern MacWindow wifiWindow;
    extern MacWindow radioWindow;

    isConnecting = false;
    hideNotification();
    showNotification("WiFi Connected!", 2000);

    // Save credentials
    ConfigManager::setWifiCredentials(connectingSSID, connectingPassword);

    // Close WiFi window
    wifiWindow.visible = false;
    wifiWindow.active = false;

    // Clean up
    cleanupWifiList();
    connectingSSID = "";
    connectingPassword = "";

    // Redraw and restore radio window
    lcd.startWrite();
    drawCheckeredPatternArea(lcd, wifiWindow.x, wifiWindow.y, wifiWindow.w + 5, wifiWindow.h + 5);
    radioWindow.visible = true;
    radioWindow.active = true;
    drawWindow(lcd, radioWindow);
    lcd.endWrite();

    // Update WiFi signal display
    drawWifiSignal(lcd, WiFi.RSSI());
    return;
  }

  // Check for timeout - stop trying and disconnect
  unsigned long now = millis();
  if (now - connectionStartTime >= CONNECTION_TIMEOUT) {
    extern LGFX lcd;
    extern MacWindow wifiWindow;
    extern MacWindow radioWindow;

    isConnecting = false;
    connectingSSID = "";
    connectingPassword = "";
    WiFi.disconnect();
    hideNotification();

    // Close WiFi window
    wifiWindow.visible = false;
    wifiWindow.active = false;

    // Clean up
    cleanupWifiList();

    // Redraw and restore radio window
    lcd.startWrite();
    drawCheckeredPatternArea(lcd, wifiWindow.x, wifiWindow.y, wifiWindow.w + 5, wifiWindow.h + 5);
    radioWindow.visible = true;
    radioWindow.active = true;
    drawWindow(lcd, radioWindow);
    lcd.endWrite();

    showNotification("Connection failed!", 3000);
  }
}
