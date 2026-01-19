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

#if ENABLE_SERIAL_DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#endif

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
struct WiFiConnectionState {
  bool isConnecting;
  String ssid;
  String password;
  unsigned long startTime;
  static const unsigned long TIMEOUT = 10000;

  void reset() {
    isConnecting = false;
    ssid = "";
    password = "";
    startTime = 0;
  }

  void start(const String& networkSSID, const String& networkPassword) {
    isConnecting = true;
    ssid = networkSSID;
    password = networkPassword;
    startTime = millis();
  }
};

static WiFiConnectionState connectionState = {false, "", "", 0};

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
void reconnectWifi(bool& retFlag);
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
      createLabelComponent(10, 44, 280, 20, LBL_WIFI_PASSWORD, "Enter password for:");
  labelData = (MacLabel*)lblPassword->customData;
  labelData->font = FONT_CHICAGO_9PT;
  lblPassword->visible = false;
  addChildComponent(wifiWindow, lblPassword);

  MacComponent* txtPassword =
      createInputFieldComponent(10, 70, 280, 28, INPUT_WIFI_PASSWORD, "Password", 64);
  txtPassword->visible = false;
  addChildComponent(wifiWindow, txtPassword);

  MacComponent* btnPasswordOk =
      createButtonComponent(110, 110, 90, 28, BTN_WIFI_PASSWORD_OK, "Connect");
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

void triggerPlayAudioOnConnect() {
  // Send command to audio task to start streaming
  extern QueueHandle_t audioCommandQueue;
  extern String RadioURL;

  AudioCommandMsg msg = {CMD_CONNECT, ""};
  strncpy(msg.url, RadioURL.c_str(), sizeof(msg.url) - 1);
  xQueueSend(audioCommandQueue, &msg, portMAX_DELAY);
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
}

void hideWifiKeyboard() {
  extern MacComponent* wifiKeyboard;
  extern LGFX lcd;

  // Hide Keyboard
  if (wifiKeyboard && wifiKeyboard->customData) {
    MacKeyboard* kb = (MacKeyboard*)wifiKeyboard->customData;
    kb->visible = false;

    int keyboardHeight = screenHeight / 2;
    int keyboardY = screenHeight - keyboardHeight;
    drawCheckeredPatternArea(lcd, 0, keyboardY, screenWidth, keyboardHeight);
  }

  // Redraw Window
  extern MacWindow wifiWindow;
  drawWindow(lcd, wifiWindow);
  drawBottomBar(lcd, "", false);
}

void hideWifiPasswordEntry() {
  extern MacWindow wifiWindow;
  extern LGFX lcd;

  if (!wifiComponent)
    return;

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

void reconnectWifi() {
  String savedSSID = ConfigManager::getWifiSSID();
  String savedPassword = ConfigManager::getWifiPassword();

  if (savedSSID.length() > 0) {
    showNotification("Connecting to " + savedSSID + "...");
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

      triggerPlayAudioOnConnect();
    } else {
      hideNotification();
      showNotification("WiFi connection failed", 3000);
      return;
    }
  }
}

void onWifiCancelButtonClick() {
  extern MacWindow wifiWindow;
  extern MacWindow radioWindow;
  extern LGFX lcd;

  wifiWindow.visible = false;
  wifiWindow.active = false;

  cleanupWifiList();

  lcd.startWrite();
  drawCheckeredPatternArea(lcd, wifiWindow.x, wifiWindow.y, wifiWindow.w + 5, wifiWindow.h + 5);
  radioWindow.visible = true;
  radioWindow.active = true;
  drawWindow(lcd, radioWindow);
  lcd.endWrite();

  reconnectWifi();
}

void onWifiPasswordSaveClick() {
  if (!wifiComponent || !wifiComponent->txtPassword || !wifiComponent->txtPassword->customData)
    return;

  MacInputField* inputData = (MacInputField*)wifiComponent->txtPassword->customData;
  String password = inputData->text;

  if (password.length() < 8) {
    showNotification("Password at least 8 characters", 2000);
    return;
  }

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

  connectionState.start(selectedSSID, password);
  WiFi.disconnect();
  if (connectionState.password.length() > 0) {
    WiFi.begin(connectionState.ssid.c_str(), connectionState.password.c_str());
  } else {
    WiFi.begin(connectionState.ssid.c_str());
  }

  showNotification("Connecting to " + connectionState.ssid + "...");
}

bool isWifiConnecting() {
  return connectionState.isConnecting;
}

void cancelWifiConnection() {
  if (connectionState.isConnecting) {
    connectionState.reset();
    WiFi.disconnect();
    hideNotification();
  }
}

void updateWifiConnectionStatus() {
  if (!connectionState.isConnecting)
    return;

  wl_status_t status = WiFi.status();

  if (status == WL_CONNECTED) {
    extern LGFX lcd;
    extern MacWindow wifiWindow;
    extern MacWindow radioWindow;

    hideNotification();
    showNotification("WiFi Connected!", 2000);

    ConfigManager::setWifiCredentials(connectionState.ssid, connectionState.password);

    wifiWindow.visible = false;
    wifiWindow.active = false;

    cleanupWifiList();
    connectionState.reset();

    lcd.startWrite();
    drawCheckeredPatternArea(lcd, wifiWindow.x, wifiWindow.y, wifiWindow.w + 5, wifiWindow.h + 5);
    radioWindow.visible = true;
    radioWindow.active = true;
    drawWindow(lcd, radioWindow);
    lcd.endWrite();

    drawWifiSignal(lcd, WiFi.RSSI());
    triggerPlayAudioOnConnect();

    // Initialize web server when WiFi connects (if not already initialized)
    extern void initWebServer();
    initWebServer();

    // Always show the IP address notification when WiFi connects
    String ipAddress = WiFi.localIP().toString();
    DEBUG_PRINT("Web server available at: http://");
    DEBUG_PRINTLN(ipAddress);
    showNotification("Web: " + ipAddress, 5000);

    return;
  }

  unsigned long now = millis();
  if (now - connectionState.startTime >= WiFiConnectionState::TIMEOUT) {
    connectionState.reset();
    WiFi.disconnect();

    showNotification("Connection failed!", 3000);
  }
}

// WiFi Window Callbacks
void onWifiWindowMinimize() {
  extern MacComponent* wifiKeyboard;

  if (wifiKeyboard) {
    MacKeyboard* keyboard = (MacKeyboard*)wifiKeyboard->customData;
    keyboard->visible = false;
  }

  wifiWindow.visible = false;
  wifiWindow.active = false;

  int keyboardHeight = screenHeight / 2;
  int keyboardY = screenHeight - keyboardHeight;
  drawCheckeredPatternArea(lcd, 0, keyboardY, screenWidth, keyboardHeight);

  drawCheckeredPatternArea(lcd, wifiWindow.x, wifiWindow.y, wifiWindow.w + 5, wifiWindow.h + 5);
  radioWindow.visible = true;
  radioWindow.active = true;
  drawWindow(lcd, radioWindow);

  reconnectWifi();
}

void onWifiWindowClose() {
  extern MacComponent* wifiKeyboard;

  if (wifiKeyboard) {
    MacKeyboard* keyboard = (MacKeyboard*)wifiKeyboard->customData;
    keyboard->visible = false;
  }

  wifiWindow.visible = false;
  wifiWindow.active = false;

  int keyboardHeight = screenHeight / 2;
  int keyboardY = screenHeight - keyboardHeight;
  drawCheckeredPatternArea(lcd, 0, keyboardY, screenWidth, keyboardHeight);

  drawCheckeredPatternArea(lcd, wifiWindow.x, wifiWindow.y, wifiWindow.w + 5, wifiWindow.h + 5);
  radioWindow.visible = true;
  radioWindow.active = true;
  drawWindow(lcd, radioWindow);

  reconnectWifi();
}

void onWifiWindowContentClick(int relativeX, int relativeY) {
  extern MacComponent* wifiKeyboard;
  extern const int INPUT_WIFI_PASSWORD;

  MacComponent* passwordInputComp = findComponentById(wifiWindow, INPUT_WIFI_PASSWORD);

  if (wifiKeyboard && passwordInputComp && passwordInputComp->visible) {
    MacKeyboard* keyboard = (MacKeyboard*)wifiKeyboard->customData;

    if (relativeX >= passwordInputComp->x &&
        relativeX <= passwordInputComp->x + passwordInputComp->w &&
        relativeY >= passwordInputComp->y &&
        relativeY <= passwordInputComp->y + passwordInputComp->h) {
      MacInputField* passwordInput = (MacInputField*)passwordInputComp->customData;
      passwordInput->focused = true;
      keyboard->targetInputId = INPUT_WIFI_PASSWORD;
      keyboard->visible = true;

      drawComponent(lcd, *passwordInputComp, wifiWindow.x, wifiWindow.y);
      drawComponent(lcd, *wifiKeyboard, 0, 0);

      int tx, ty;
      delay(150);
      while (lcd.getTouch(&tx, &ty)) {
        delay(10);
      }
      return;
    }
  }

  handleWindowContentClick(lcd, wifiWindow, relativeX, relativeY);
}

void onWifiWindowMoved() {
  handleWindowMoved(lcd, wifiWindow);
}