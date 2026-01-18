/*
 * WifiWindow.cpp - WiFi Window Implementation
 *
 * Copyright (c) 2025 felangga
 *
 */

#include "WifiWindow.h"
#include <WiFi.h>
#include "ConfigManager.h"
#include "GlobalState.h"
#include "RadioWindow.h"
#include "UIHelpers.h"
#include "WindowCallbacks.h"
#include "wt32_sc01_plus.h"

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

// WiFi Window component cache
struct WiFiWindowComponents {
  MacComponent* listComp;
  MacComponent* btnRefresh;
  MacComponent* btnConnect;
  MacComponent* lblTitle;
  MacComponent* lblPassword;
  MacComponent* txtPassword;
  MacComponent* btnPasswordOk;
  MacComponent* btnPasswordCancel;
};

static WiFiWindowComponents* wifiComponent = nullptr;

// Forward declarations
void onWifiItemClick(int index, void* itemData);
void onWifiConnectButtonClick();
void onWifiCancelButtonClick();
void onWifiRefreshButtonClick();
void onWifiPasswordSaveClick();
void onWifiPasswordCancelClick();
void initializeWifiComponentCache();

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

void initializeWifiComponentCache() {
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

  if (wifiComponent != nullptr) {
    delete wifiComponent;
  }

  wifiComponent = new WiFiWindowComponents();
  wifiComponent->listComp = findComponentById(wifiWindow, WIFI_LIST_COMPONENT);
  wifiComponent->btnRefresh = findComponentById(wifiWindow, BTN_WIFI_REFRESH);
  wifiComponent->btnConnect = findComponentById(wifiWindow, BTN_WIFI_CONNECT);
  wifiComponent->lblTitle = findComponentById(wifiWindow, LBL_WIFI_TITLE);
  wifiComponent->lblPassword = findComponentById(wifiWindow, LBL_WIFI_PASSWORD);
  wifiComponent->txtPassword = findComponentById(wifiWindow, INPUT_WIFI_PASSWORD);
  wifiComponent->btnPasswordOk = findComponentById(wifiWindow, BTN_WIFI_PASSWORD_OK);
  wifiComponent->btnPasswordCancel = findComponentById(wifiWindow, BTN_WIFI_PASSWORD_CANCEL);
}

String getRSSIDisplay(int rssi) {
  return String(rssi) + " dBm";
}

void scanWifiNetworks() {
  extern LGFX lcd;
  extern MacWindow wifiWindow;
  extern const int WIFI_LIST_COMPONENT;

  isScanning = true;

  cleanupWifiList();

  updateWifiListDisplay();

  showNotification("Scanning WiFi...");

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
      if (ssid.length() == 0)
        continue;

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

      // Format display text: "SSID -XX dBm"
      String displayText = tempSSIDs[i];
      if (displayText.length() > 18) {
        displayText = displayText.substring(0, 15) + "...";
      }
      displayText += "  " + getRSSIDisplay(tempRSSIs[i]);
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

  if (!wifiComponent || !wifiComponent->listComp || !wifiComponent->listComp->customData)
    return;

  MacListView* listViewData = (MacListView*)wifiComponent->listComp->customData;
  listViewData->items = wifiItems;
  listViewData->itemCount = wifiItemCount;
  listViewData->selectedIndex = -1;
  listViewData->scrollOffset = 0;
  listViewData->needsFullRedraw = true;

  if (wifiWindow.visible) {
    lcd.startWrite();
    drawComponent(lcd, *wifiComponent->listComp, wifiWindow.x, wifiWindow.y);
    lcd.endWrite();
  }
}

void onWifiItemClick(int index, void* itemData) {
  extern MacWindow wifiWindow;
  extern LGFX lcd;

  if (index < 0 || index >= wifiItemCount)
    return;
  if (wifiSSIDs[index].length() == 0)
    return;

  selectedWifiIndex = index;
  selectedSSID = wifiSSIDs[index];
  selectedIsSecure = wifiSecure[index];

  if (wifiComponent && wifiComponent->btnConnect) {
    wifiComponent->btnConnect->enabled = true;
    lcd.startWrite();
    drawComponent(lcd, *wifiComponent->btnConnect, wifiWindow.x, wifiWindow.y);
    lcd.endWrite();
  }
}

void initializeWifiWindow() {
  extern MacWindow wifiWindow;
  extern const int WIFI_LIST_COMPONENT;
  extern const int BTN_WIFI_CONNECT;
  extern const int BTN_WIFI_REFRESH;
  extern const int LBL_WIFI_TITLE;
  extern const int INPUT_WIFI_PASSWORD;
  extern const int LBL_WIFI_PASSWORD;
  extern const int BTN_WIFI_PASSWORD_OK;
  extern const int BTN_WIFI_PASSWORD_CANCEL;

  clearChildComponents(wifiWindow);

  // Title label
  MacComponent* lblTitle =
      createLabelComponent(10, 42, 200, 20, LBL_WIFI_TITLE, "Select a network:");
  MacLabel* labelData = (MacLabel*)lblTitle->customData;
  labelData->font = FONT_CHICAGO_9PT;
  addChildComponent(wifiWindow, lblTitle);

  // WiFi networks list
  MacComponent* wifiList =
      createListViewComponent(10, 60, 280, 120, WIFI_LIST_COMPONENT, wifiItems, wifiItemCount, 24);
  if (wifiList && wifiList->customData) {
    MacListView* listViewData = (MacListView*)wifiList->customData;
    listViewData->onItemClick = onWifiItemClick;
    listViewData->font = FONT_CHICAGO_9PT;
  }
  addChildComponent(wifiWindow, wifiList);

  MacComponent* btnRefresh = createButtonComponent(10, 185, 90, 28, BTN_WIFI_REFRESH, "Refresh");
  btnRefresh->onClick = [](int componentId) { onWifiRefreshButtonClick(); };
  addChildComponent(wifiWindow, btnRefresh);

  MacComponent* btnConnect = createButtonComponent(200, 185, 90, 28, BTN_WIFI_CONNECT, "Connect");
  btnConnect->onClick = [](int componentId) { onWifiConnectButtonClick(); };
  btnConnect->enabled = false;
  addChildComponent(wifiWindow, btnConnect);

  MacComponent* lblPassword =
      createLabelComponent(10, 42, 280, 20, LBL_WIFI_PASSWORD, "Enter password for:");
  labelData = (MacLabel*)lblPassword->customData;
  labelData->font = FONT_CHICAGO_9PT;
  lblPassword->visible = false;
  addChildComponent(wifiWindow, lblPassword);

  MacComponent* txtPassword =
      createInputFieldComponent(10, 70, 280, 28, INPUT_WIFI_PASSWORD, "Password", 64);
  txtPassword->visible = false;
  addChildComponent(wifiWindow, txtPassword);

  MacComponent* btnPasswordOk =
      createButtonComponent(120, 110, 80, 28, BTN_WIFI_PASSWORD_OK, "Connect");
  btnPasswordOk->onClick = [](int componentId) { onWifiPasswordSaveClick(); };
  btnPasswordOk->visible = false;
  addChildComponent(wifiWindow, btnPasswordOk);

  MacComponent* btnPasswordCancel =
      createButtonComponent(210, 110, 80, 28, BTN_WIFI_PASSWORD_CANCEL, "Cancel");
  btnPasswordCancel->onClick = [](int componentId) { onWifiPasswordCancelClick(); };
  btnPasswordCancel->visible = false;
  addChildComponent(wifiWindow, btnPasswordCancel);

  selectedWifiIndex = -1;
  selectedSSID = "";
  selectedIsSecure = false;

  initializeWifiComponentCache();
}

void showWifiPasswordEntry() {
  extern MacWindow wifiWindow;
  extern LGFX lcd;

  if (!wifiComponent)
    return;

  if (wifiComponent->listComp)
    wifiComponent->listComp->visible = false;

  if (wifiComponent->btnRefresh)
    wifiComponent->btnRefresh->visible = false;

  if (wifiComponent->btnConnect)
    wifiComponent->btnConnect->visible = false;

  if (wifiComponent->lblTitle)
    wifiComponent->lblTitle->visible = false;

  if (wifiComponent->lblPassword && wifiComponent->lblPassword->customData) {
    MacLabel* labelData = (MacLabel*)wifiComponent->lblPassword->customData;
    labelData->text = "Password for: " + selectedSSID;
    wifiComponent->lblPassword->visible = true;
  }

  if (wifiComponent->txtPassword) {
    wifiComponent->txtPassword->visible = true;
    if (wifiComponent->txtPassword->customData) {
      MacInputField* inputData = (MacInputField*)wifiComponent->txtPassword->customData;
      inputData->text = "";
      inputData->cursorPos = 0;
      inputData->focused = true;
    }
  }

  if (wifiComponent->btnPasswordOk)
    wifiComponent->btnPasswordOk->visible = true;

  if (wifiComponent->btnPasswordCancel)
    wifiComponent->btnPasswordCancel->visible = true;

  lcd.startWrite();
  drawWindow(lcd, wifiWindow);
  lcd.endWrite();

  extern MacComponent* wifiKeyboard;
  extern const int WIFI_KEYBOARD_COMPONENT;
  if (!wifiKeyboard) {
    int keyboardHeight = screenHeight / 2;
    int keyboardY = screenHeight - keyboardHeight;
    wifiKeyboard = createKeyboardComponent(0, keyboardY, screenWidth, keyboardHeight,
                                           WIFI_KEYBOARD_COMPONENT, INPUT_WIFI_PASSWORD);
  }

  if (wifiKeyboard && wifiKeyboard->customData) {
    MacKeyboard* kb = (MacKeyboard*)wifiKeyboard->customData;
    kb->visible = true;
    kb->targetInputId = INPUT_WIFI_PASSWORD;
  }
}

void hideWifiPasswordEntry() {
  extern MacWindow wifiWindow;
  extern LGFX lcd;

  if (!wifiComponent)
    return;

  extern MacComponent* wifiKeyboard;
  if (wifiKeyboard && wifiKeyboard->customData) {
    MacKeyboard* kb = (MacKeyboard*)wifiKeyboard->customData;
    kb->visible = false;

    int keyboardHeight = screenHeight / 2;
    int keyboardY = screenHeight - keyboardHeight;
    drawCheckeredPatternArea(lcd, 0, keyboardY, screenWidth, keyboardHeight);
  }

  if (wifiComponent->lblPassword)
    wifiComponent->lblPassword->visible = false;

  if (wifiComponent->txtPassword) {
    wifiComponent->txtPassword->visible = false;
    if (wifiComponent->txtPassword->customData) {
      MacInputField* inputData = (MacInputField*)wifiComponent->txtPassword->customData;
      inputData->focused = false;
    }
  }

  if (wifiComponent->btnPasswordOk)
    wifiComponent->btnPasswordOk->visible = false;

  if (wifiComponent->btnPasswordCancel)
    wifiComponent->btnPasswordCancel->visible = false;

  if (wifiComponent->listComp)
    wifiComponent->listComp->visible = true;

  if (wifiComponent->btnRefresh)
    wifiComponent->btnRefresh->visible = true;

  if (wifiComponent->btnConnect)
    wifiComponent->btnConnect->visible = true;

  if (wifiComponent->lblTitle)
    wifiComponent->lblTitle->visible = true;

  lcd.startWrite();
  drawWindow(lcd, wifiWindow);
  lcd.endWrite();
}

void onWifiRefreshButtonClick() {
  scanWifiNetworks();
}

void onWifiConnectButtonClick() {
  if (selectedWifiIndex < 0 || selectedSSID.length() == 0)
    return;

  if (selectedIsSecure) {
    showWifiPasswordEntry();
  } else {
    connectToSelectedWifi("");
  }
}

void onWifiCancelButtonClick() {
  extern MacWindow wifiWindow;
  extern MacWindow radioWindow;
  extern LGFX lcd;

  wifiWindow.visible = false;
  wifiWindow.active = false;

  cleanupWifiList();

  String savedSSID = ConfigManager::getWifiSSID();
  String savedPassword = ConfigManager::getWifiPassword();

  if (savedSSID.length() > 0) {
    showNotification("Connecting to WiFi...");
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    delay(100);

    if (savedPassword.length() > 0) {
      WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    } else {
      WiFi.begin(savedSSID.c_str());
    }

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 5000) {
      delay(100);
    }

    if (WiFi.status() == WL_CONNECTED) {
      hideNotification();
      showNotification("WiFi Connected!", 2000);
      drawWifiSignal(lcd, WiFi.RSSI());
    } else {
      hideNotification();
      showNotification("WiFi connection failed", 3000);
    }
  }

  lcd.startWrite();
  drawCheckeredPatternArea(lcd, wifiWindow.x, wifiWindow.y, wifiWindow.w + 5, wifiWindow.h + 5);
  radioWindow.visible = true;
  radioWindow.active = true;
  drawWindow(lcd, radioWindow);
  lcd.endWrite();
}

void onWifiPasswordSaveClick() {
  if (!wifiComponent || !wifiComponent->txtPassword || !wifiComponent->txtPassword->customData)
    return;

  MacInputField* inputData = (MacInputField*)wifiComponent->txtPassword->customData;
  String password = inputData->text;

  hideWifiPasswordEntry();

  connectToSelectedWifi(password);
}

void onWifiPasswordCancelClick() {
  hideWifiPasswordEntry();
}

void connectToSelectedWifi() {
  connectToSelectedWifi("");
}

void connectToSelectedWifi(const String& password) {
  if (selectedSSID.length() == 0)
    return;

  connectingSSID = selectedSSID;
  connectingPassword = password;
  isConnecting = true;
  connectionStartTime = millis();

  extern volatile bool isPlaying;
  extern QueueHandle_t audioCommandQueue;

  if (isPlaying && audioCommandQueue != nullptr) {
    AudioCommandMsg msg = {CMD_STOP, ""};
    if (xQueueSend(audioCommandQueue, &msg, pdMS_TO_TICKS(100)) == pdTRUE) {
      isPlaying = false;
      updateComponentSymbol(radioWindow, 1, SYMBOL_PLAY);
    }
  }

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
  if (!isConnecting)
    return;

  wl_status_t status = WiFi.status();

  if (status == WL_CONNECTED) {
    extern LGFX lcd;
    extern MacWindow wifiWindow;
    extern MacWindow radioWindow;

    isConnecting = false;
    hideNotification();
    showNotification("WiFi Connected!", 2000);

    ConfigManager::setWifiCredentials(connectingSSID, connectingPassword);

    wifiWindow.visible = false;
    wifiWindow.active = false;

    cleanupWifiList();
    connectingSSID = "";
    connectingPassword = "";

    lcd.startWrite();
    drawCheckeredPatternArea(lcd, wifiWindow.x, wifiWindow.y, wifiWindow.w + 5, wifiWindow.h + 5);
    radioWindow.visible = true;
    radioWindow.active = true;
    drawWindow(lcd, radioWindow);
    lcd.endWrite();

    drawWifiSignal(lcd, WiFi.RSSI());
    return;
  }

  // Check for timeout - stop trying and disconnect
  unsigned long now = millis();
  if (now - connectionStartTime >= CONNECTION_TIMEOUT) {
    extern LGFX lcd;
    extern MacWindow wifiWindow;
    extern MacWindow radioWindow;
    extern MacComponent* wifiKeyboard;

    isConnecting = false;
    connectingSSID = "";
    connectingPassword = "";
    WiFi.disconnect();
    hideNotification();

    if (wifiKeyboard && wifiKeyboard->customData) {
      MacKeyboard* kb = (MacKeyboard*)wifiKeyboard->customData;
      kb->visible = false;
    }

    wifiWindow.visible = false;
    wifiWindow.active = false;

    cleanupWifiList();

    int keyboardHeight = screenHeight / 2;
    int keyboardY = screenHeight - keyboardHeight;

    lcd.startWrite();
    drawCheckeredPatternArea(lcd, 0, keyboardY, screenWidth, keyboardHeight);
    drawCheckeredPatternArea(lcd, wifiWindow.x, wifiWindow.y, wifiWindow.w + 5, wifiWindow.h + 5);
    radioWindow.visible = true;
    radioWindow.active = true;
    drawWindow(lcd, radioWindow);
    lcd.endWrite();

    showNotification("Connection failed!", 3000);
  }
}
