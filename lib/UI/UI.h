/*
 * UI.h - Classic UI Library
 *
 * Copyright (c) 2025 felangga
 *
 * This header file contains declarations for drawing Classic UI elements
 * using primitive graphics functions with LovyanGFX library.
 */

#ifndef UI_H
#define UI_H

#include <LovyanGFX.hpp>
#include "Arduino.h"
#include "ChicagoFont.h"
#include "wt32_sc01_plus.h"

// Forward declarations for screen dimensions (defined in wt32_sc01_plus.h)
extern const uint32_t screenWidth;
extern const uint32_t screenHeight;

// Sprite buffer for double buffering (defined in implementation)
extern lgfx::LGFX_Sprite* componentSprite;

// ===== CLASSIC UI COLORS =====
#define UI_WHITE 0xFFFF
#define UI_BLACK 0x0000
#define UI_GRAY 0x8410
#define UI_LIGHT_GRAY 0xC618
#define UI_DARK_GRAY 0x4208
#define UI_BLUE 0x001F

// ===== CHICAGO FONT =====
#define CHICAGO14_FONT &Chicago14pt
#define CHICAGO11_FONT &Chicago11pt
#define CHICAGO9_FONT &Chicago9pt

enum FontType {
  FONT_CHICAGO_9PT,
  FONT_CHICAGO_11PT,
  FONT_CHICAGO_14PT,
  FONT_DEJAVU_12PT,
  FONT_TOM_THUMB,
  FONT_FREE_MONO_12PT,
  FONT_DEFAULT  // System default font (nullptr)
};

// Helper function to convert FontType enum to GFXfont pointer
// This is used internally by UI to convert the enum to the actual font pointer
const GFXfont* getFontFromType(FontType fontType);
// ===== WINDOW LAYOUT CONSTANTS =====
#define TITLE_BAR_HEIGHT 32
#define TITLE_BAR_BORDER 2
#define CONTENT_START_Y                  \
  (TITLE_BAR_BORDER + TITLE_BAR_HEIGHT + \
   8)  // Start Y position for window content (border + title bar + margin)

// ===== COMPONENT TYPES =====
enum ComponentType {
  COMPONENT_BUTTON,
  COMPONENT_LABEL,
  COMPONENT_TEXTBOX,
  COMPONENT_CHECKBOX,
  COMPONENT_SLIDER,
  COMPONENT_PROGRESS_BAR,
  COMPONENT_RUNNING_TEXT,
  COMPONENT_LISTVIEW,
  COMPONENT_INPUT_FIELD,
  COMPONENT_KEYBOARD,
  COMPONENT_IMAGE,
  COMPONENT_CUSTOM
};

// ===== SYMBOL TYPES FOR BUTTONS =====
enum SymbolType {
  SYMBOL_PLAY,
  SYMBOL_PAUSE,
  SYMBOL_STOP,
  SYMBOL_PREV,
  SYMBOL_NEXT,
  SYMBOL_VOL_UP,
  SYMBOL_VOL_DOWN,
  SYMBOL_LIST,
  SYMBOL_NONE
};

// ===== BASE COMPONENT STRUCT =====
struct UIComponent {
  ComponentType type;
  int x;
  int y;
  int w;
  int h;
  int id;
  int radius;
  bool visible;
  bool enabled;
  void (*onClick)(int componentId);                    // Generic callback with component ID
  void (*onValueChanged)(int componentId, int value);  // Callback for value changes (e.g., sliders)
  void* customData;                                    // Pointer to component-specific data
};

// ===== BUTTON STRUCT =====
#define BUTTON_DEFAULT_RADIUS 10

struct UIButton {
  String text;
  SymbolType symbol;  // symbol to draw instead of text (SYMBOL_NONE for text buttons)
  bool pressed;       // current pressed state
  int radius;
  FontType font;
};

// ===== LABEL STRUCT =====
struct UILabel {
  String text;
  uint16_t textColor;
  uint16_t backgroundColor;
  int textSize;
  bool centerAlign;
  FontType font;
};

// ===== TEXTBOX STRUCT =====
struct UITextBox {
  String text;
  String placeholder;
  bool focused;
  int cursorPos;
  int maxLength;
  unsigned long lastCursorBlink;  // For cursor blinking animation
  bool cursorVisible;
};

// ===== INPUT FIELD STRUCT (Enhanced text input with keyboard support) =====
struct UIInputField {
  String text;
  String placeholder;
  bool focused;
  int cursorPos;
  int maxLength;
  unsigned long lastCursorBlink;
  bool cursorVisible;
  int scrollOffset;  // Horizontal scroll offset for long text
  FontType font;
  void (*onTextChanged)(int componentId, const String& text);
};

// ===== KEYBOARD STRUCT =====
struct UIKeyboard {
  bool visible;
  int x;
  int y;
  int w;
  int h;
  int targetInputId;                 // ID of the input field this keyboard is linked to
  bool shiftActive;                  // Shift key state (for uppercase)
  bool shiftLocked;                  // Shift lock state (caps lock mode via double-tap)
  unsigned long lastShiftPressTime;  // Timestamp of last shift press for double-tap detection
  int selectedKey;                   // Currently highlighted key (-1 for none)
  // Key repeat tracking
  bool isKeyPressed;            // Is a key currently being held
  unsigned long keyPressStart;  // When the key was first pressed
  unsigned long lastRepeat;     // Last time the key was repeated
  int pressedRow;               // Which row is being pressed (-1 for special keys)
  int pressedKeyIndex;          // Which key in the row is being pressed
  char lastPressedChar;         // The character of the last pressed key
  bool isBackspace;             // Is backspace being held
  bool isSpace;                 // Is space being held
};

// ===== CHECKBOX STRUCT =====
struct UICheckBox {
  String label;
  bool checked;
};

// ===== SLIDER STRUCT =====
struct UISlider {
  int minValue;
  int maxValue;
  int currentValue;
  bool vertical;
};

// ===== PROGRESS BAR STRUCT =====
struct UIProgressBar {
  int minValue;
  int maxValue;
  int currentValue;
  uint16_t fillColor;
  bool showPercentage;
};

// ===== RUNNING TEXT STRUCT =====
struct UIRunningText {
  String text;
  uint16_t textColor;
  uint16_t backgroundColor;
  int textSize;
  int scrollOffset;              // Current scroll position in pixels
  int scrollSpeed;               // Pixels to scroll per update (can be negative for right-to-left)
  unsigned long lastUpdate;      // Last update time in milliseconds
  int updateInterval;            // Update interval in milliseconds
  bool scrollEnabled;            // Enable/disable scrolling
  bool isPaused;                 // Pause state when text is fully visible
  unsigned long pauseStartTime;  // Time when pause started
  int pauseDuration;             // How long to pause in milliseconds (default: 2000ms)
  FontType font;
};

// ===== LISTVIEW STRUCT =====
struct UIListViewItem {
  String text;
  void* data;  // Custom data pointer for each item
};

struct UIListView {
  UIListViewItem* items;   // Array of list items
  int itemCount;           // Number of items
  int selectedIndex;       // Currently selected item (-1 if none)
  int scrollOffset;        // Vertical scroll offset in pixels
  int itemHeight;          // Height of each item in pixels
  int visibleItemCount;    // Number of items visible at once
  uint16_t textColor;
  uint16_t backgroundColor;
  uint16_t selectedColor;  // Background color for selected item
  int textSize;
  FontType font;
  void (*onItemClick)(int index, void* itemData);  // Callback when item is clicked
  // Touch/swipe tracking
  bool isTouching;               // Currently being touched
  int lastTouchY;                // Last touch Y position
  int touchStartY;               // Touch start Y position
  unsigned long touchStartTime;  // Touch start time for detecting taps vs swipes
  bool needsFullRedraw;          // Flag to indicate full redraw (frame + content) vs content-only
  int lastScrollbarThumbY;       // Last scrollbar thumb Y position to detect changes
};

// ===== WINDOW STRUCT =====
struct UIWindow {
  int x;
  int y;
  int w;
  int h;
  String title;
  bool active;
  bool minimized;
  bool visible;
  void (*onMinimize)();  // callback when minimize button is clicked
  void (*onClose)();     // callback when close button is clicked
  void (*onContentClick)(int relativeX,
                         int relativeY);  // callback for content clicks with relative coordinates
  void (*onWindowMoved)();                // callback when window position changes

  // Child components (flexible system)
  UIComponent** childComponents;  // Array of pointers to components
  int childComponentCount;        // Number of child components

  // Dragging state (internal use)
  bool isDragging;
  int dragOffsetX;
  int dragOffsetY;
};

// ===== DESKTOP ICON STRUCT =====
struct DesktopIcon {
  int x;
  int y;
  String name;
  String iconType;  // "window", "app", "file", etc.
  bool visible;
  bool selected;
  UIWindow* linkedWindow;  // pointer to associated window
  void (*onClick)();       // callback when icon is clicked
};

// ===== FUNCTION DECLARATIONS =====

// Main UI drawing functions
void drawMenuBar(lgfx::LGFX_Device& lcd, const String& appName);
void drawBottomBar(lgfx::LGFX_Device& lcd, const String& message = "",
                   bool highlight = false);  // Bottom notification bar
void drawCheckeredPattern(lgfx::LGFX_Device& lcd);
void drawCheckeredPatternArea(lgfx::LGFX_Device& lcd, int x, int y, int w, int h);
void drawClock(lgfx::LGFX_Device& lcd, const String& time);  // added clock drawing
void drawWifiSignal(lgfx::LGFX_Device& lcd, int rssi);       // WiFi signal strength indicator

// Window and dialog functions
void drawWindow(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const String& title,
                bool active = true);
void drawWindow(lgfx::LGFX_Device& lcd,
                const UIWindow& window);  // overloaded version for UIWindow struct
void drawButton(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, int radius, const String& text,
                bool pressed = false, FontType font = FONT_DEFAULT);
void drawSymbolButton(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, int radius,
                      SymbolType symbol, bool pressed = false);
void drawSymbol(lgfx::LGFX_Device& lcd, int x, int y, int size, SymbolType symbol,
                uint16_t color = UI_BLACK);
void draw3DFrame(lgfx::LGFX_Device& lcd, int x, int y, int w, int h);

// Desktop elements
void drawDesktopIcon(lgfx::LGFX_Device& lcd, int x, int y, const String& name,
                     bool selected = false);

// ===== WINDOW HELPERS =====
void interactiveWindow(lgfx::LGFX_Device& lcd, UIWindow& window);
bool isInsideCloseButton(const UIWindow& window, int tx, int ty);
bool isInsideMinimizeButton(const UIWindow& window, int tx, int ty);
bool isInsideTitleBar(const UIWindow& window, int tx, int ty);

// ===== FLEXIBLE COMPONENT MANAGEMENT =====
UIComponent* createComponent(ComponentType type, int x, int y, int w, int h, int id);
void addChildComponent(UIWindow& window, UIComponent* component);
void clearChildComponents(UIWindow& window);
void drawWindowChildComponents(lgfx::LGFX_Device& lcd, const UIWindow& window);
UIComponent* findComponentAt(const UIWindow& window, int x, int y);
void updateRunningTextComponents(lgfx::LGFX_Device& lcd, UIWindow& window);

// Component-specific drawing functions
void drawComponent(lgfx::LGFX_Device& lcd, const UIComponent& component, int windowX, int windowY);
void initComponentBuffer(lgfx::LGFX_Device* lcd, int maxWidth, int maxHeight);

// Individual component drawing functions (implemented in separate files)
void drawLabel(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const UILabel& label);
void drawTextBox(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const UITextBox& textbox);
void drawCheckBox(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const UICheckBox& checkbox);
void drawSlider(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const UISlider& slider);
void drawProgressBar(lgfx::LGFX_Device& lcd, int x, int y, int w, int h,
                     const UIProgressBar& progressBar);
void drawRunningText(lgfx::LGFX_Device& lcd, int x, int y, int w, int h,
                     UIRunningText& runningText);
void drawListView(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, UIListView& listView);
void drawInputField(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, UIInputField& inputField);
void drawKeyboard(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, UIKeyboard& keyboard);

// Component creation helpers
UIComponent* createButtonComponent(int x, int y, int w, int h, int id, const String& text,
                                    SymbolType symbol = SYMBOL_NONE);
UIComponent* createLabelComponent(int x, int y, int w, int h, int id, const String& text,
                                   uint16_t textColor = UI_BLACK);
UIComponent* createRunningTextComponent(int x, int y, int w, int h, int id, const String& text,
                                         int scrollSpeed = 2, uint16_t textColor = UI_BLACK,
                                         int textSize = 1, FontType font = FONT_DEFAULT);
UIComponent* createListViewComponent(int x, int y, int w, int h, int id, UIListViewItem* items,
                                      int itemCount, int itemHeight = 30);
UIComponent* createInputFieldComponent(int x, int y, int w, int h, int id,
                                        const String& placeholder = "", int maxLength = 50,
                                        const String& defaultText = "");
UIComponent* createKeyboardComponent(int x, int y, int w, int h, int id, int targetInputId);
UIComponent* createSliderComponent(int x, int y, int w, int h, int id, int minVal, int maxVal,
                                    int currentVal, bool vertical = false);
UIComponent* createCheckBoxComponent(int x, int y, int w, int h, int id, const String& label,
                                      bool checked);

// Helper to update running text properties
void updateRunningTextProperties(UIComponent* component, const String* newText = nullptr,
                                 int* newTextSize = nullptr, uint16_t* newTextColor = nullptr,
                                 uint16_t* newBgColor = nullptr, int* newScrollSpeed = nullptr,
                                 int* newPauseDuration = nullptr, FontType* newFont = nullptr);

// Helper to update input field components (cursor blinking)
void updateInputFieldComponents(lgfx::LGFX_Device& lcd, UIWindow& window);

// Helper to handle keyboard touch input
bool handleKeyboardTouch(lgfx::LGFX_Device& lcd, UIComponent* keyboardComponent,
                         UIComponent* inputComponent, int touchX, int touchY,
                         UIWindow* window = nullptr);

// Helper to handle keyboard key repeat (call this continuously in the UI loop)
void handleKeyboardRepeat(lgfx::LGFX_Device& lcd, UIComponent* keyboardComponent,
                          UIComponent* inputComponent, UIWindow* window = nullptr);

// ===== GENERIC WINDOW MANAGEMENT HELPERS =====
// Utility functions that can be called from user-defined callbacks
void handleWindowClose(lgfx::LGFX_Device& lcd, UIWindow& window,
                       DesktopIcon* associatedIcon = nullptr);
void handleWindowMinimize(lgfx::LGFX_Device& lcd, UIWindow& window,
                          DesktopIcon* associatedIcon = nullptr);
void handleIconClick(lgfx::LGFX_Device& lcd, UIWindow& window,
                     DesktopIcon* associatedIcon = nullptr);
void handleWindowContentClick(lgfx::LGFX_Device& lcd, UIWindow& window, int relativeX,
                              int relativeY);
void handleWindowMoved(lgfx::LGFX_Device& lcd, UIWindow& window);

// ===== DESKTOP ICON HELPERS =====
void interactiveDesktopIcon(lgfx::LGFX_Device& lcd, DesktopIcon& icon);
bool isInsideDesktopIcon(const DesktopIcon& icon, int tx, int ty);
void redrawDesktopArea(lgfx::LGFX_Device& lcd);

// ===== WINDOW MANAGER =====
// Multi-window management for proper layering and refresh
void registerWindow(UIWindow* window);
void unregisterWindow(UIWindow* window);
void redrawAllWindows(lgfx::LGFX_Device& lcd);
void redrawAllWindowsExcept(lgfx::LGFX_Device& lcd, UIWindow* exceptWindow);
UIWindow** getRegisteredWindows(int& windowCount);
void showWindowOnTop(lgfx::LGFX_Device& lcd, UIWindow& window);

// ===== DOUBLE BUFFERING =====
// Internal helper for flicker-free window dragging
void renderToSprite(UIWindow* draggedWindow);

#endif
