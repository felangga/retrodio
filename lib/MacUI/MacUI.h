/*
 * MacUI.h - Classic Macintosh OS User Interface Library
 *
 * Copyright (c) 2025 felangga
 *
 * This header file contains declarations for drawing classic Mac OS UI elements
 * using primitive graphics functions with LovyanGFX library.
 */

#ifndef MAC_UI_H
#define MAC_UI_H

#include <LovyanGFX.hpp>
#include "Arduino.h"
#include "wt32_sc01_plus.h"
#include "ChicagoFont.h"

// Forward declarations for screen dimensions (defined in wt32_sc01_plus.h)
extern const uint32_t screenWidth;
extern const uint32_t screenHeight;

// Sprite buffer for double buffering (defined in implementation)
extern lgfx::LGFX_Sprite* componentSprite;

// ===== CLASSIC MAC OS UI COLORS =====
#define MAC_WHITE 0xFFFF
#define MAC_BLACK 0x0000
#define MAC_GRAY 0x8410
#define MAC_LIGHT_GRAY 0xC618
#define MAC_DARK_GRAY 0x4208
#define MAC_BLUE 0x001F

// ===== CHICAGO FONT =====
#define CHICAGO14_FONT &Chicago14pt
#define CHICAGO11_FONT &Chicago11pt
#define CHICAGO9_FONT  &Chicago9pt

// ===== FONT TYPE ENUMERATION =====
// Use these font types to specify fonts for UI components
// Example usage:
//   MacRunningText runningText;
//   runningText.font = FONT_CHICAGO_11PT;
//
//   MacLabel label;
//   label.font = FONT_CHICAGO_9PT;
//
//   FontType newFont = FONT_CHICAGO_14PT;
//   updateRunningTextProperties(component, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &newFont);
enum FontType {
  FONT_CHICAGO_9PT,
  FONT_CHICAGO_11PT,
  FONT_CHICAGO_14PT,
  FONT_DEFAULT  // System default font (nullptr)
};

// Helper function to convert FontType enum to GFXfont pointer
// This is used internally by MacUI to convert the enum to the actual font pointer
const GFXfont* getFontFromType(FontType fontType);
// ===== WINDOW LAYOUT CONSTANTS =====
#define TITLE_BAR_HEIGHT 32
#define TITLE_BAR_BORDER 2
#define CONTENT_START_Y (TITLE_BAR_BORDER + TITLE_BAR_HEIGHT + 8)  // Start Y position for window content (border + title bar + margin)

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
struct MacComponent {
  ComponentType type;
  int x;
  int y;
  int w;
  int h;
  int id;
  bool visible;
  bool enabled;
  void (*onClick)(int componentId);  // Generic callback with component ID
  void* customData;                  // Pointer to component-specific data
};

// ===== BUTTON STRUCT =====
struct MacButton {
  String text;
  SymbolType symbol;  // symbol to draw instead of text (SYMBOL_NONE for text buttons)
  bool pressed;       // current pressed state
  FontType font;
};

// ===== LABEL STRUCT =====
struct MacLabel {
  String text;
  uint16_t textColor;
  uint16_t backgroundColor;
  int textSize;
  bool centerAlign;
  FontType font;
};

// ===== TEXTBOX STRUCT =====
struct MacTextBox {
  String text;
  String placeholder;
  bool focused;
  int cursorPos;
  int maxLength;
  unsigned long lastCursorBlink;  // For cursor blinking animation
  bool cursorVisible;
};

// ===== INPUT FIELD STRUCT (Enhanced text input with keyboard support) =====
struct MacInputField {
  String text;
  String placeholder;
  bool focused;
  int cursorPos;
  int maxLength;
  unsigned long lastCursorBlink;
  bool cursorVisible;
  void (*onTextChanged)(int componentId, const String& text);  // Callback when text changes
};

// ===== KEYBOARD STRUCT =====
struct MacKeyboard {
  bool visible;
  int x;
  int y;
  int w;
  int h;
  int targetInputId;  // ID of the input field this keyboard is linked to
  bool shiftActive;   // Shift key state (for uppercase)
  int selectedKey;    // Currently highlighted key (-1 for none)
};

// ===== CHECKBOX STRUCT =====
struct MacCheckBox {
  String label;
  bool checked;
};

// ===== SLIDER STRUCT =====
struct MacSlider {
  int minValue;
  int maxValue;
  int currentValue;
  bool vertical;
};

// ===== PROGRESS BAR STRUCT =====
struct MacProgressBar {
  int minValue;
  int maxValue;
  int currentValue;
  uint16_t fillColor;
  bool showPercentage;
};

// ===== RUNNING TEXT STRUCT =====
struct MacRunningText {
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
struct MacListViewItem {
  String text;
  void* data;  // Custom data pointer for each item
};

struct MacListView {
  MacListViewItem* items;  // Array of list items
  int itemCount;           // Number of items
  int selectedIndex;       // Currently selected item (-1 if none)
  int scrollOffset;        // Vertical scroll offset in pixels
  int itemHeight;          // Height of each item in pixels
  int visibleItemCount;    // Number of items visible at once
  uint16_t textColor;
  uint16_t backgroundColor;
  uint16_t selectedColor;                          // Background color for selected item
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
struct MacWindow {
  int x;
  int y;
  int w;
  int h;
  String title;
  bool active;
  bool minimized;
  bool visible;
  void (*onMinimize)();                   // callback when minimize button is clicked
  void (*onClose)();                      // callback when close button is clicked
  void (*onContentClick)(int relativeX,
                         int relativeY);  // callback for content clicks with relative coordinates
  void (*onWindowMoved)();                // callback when window position changes

  // Child components (flexible system)
  MacComponent** childComponents;  // Array of pointers to components
  int childComponentCount;         // Number of child components

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
  MacWindow* linkedWindow;  // pointer to associated window
  void (*onClick)();        // callback when icon is clicked
};

// ===== FUNCTION DECLARATIONS =====

// Main UI drawing functions
void drawMenuBar(lgfx::LGFX_Device& lcd, const String& appName);
void drawCheckeredPattern(lgfx::LGFX_Device& lcd);
void drawCheckeredPatternArea(lgfx::LGFX_Device& lcd, int x, int y, int w, int h);
void drawClock(lgfx::LGFX_Device& lcd, const String& time);  // added clock drawing

// Window and dialog functions
void drawWindow(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const String& title,
                bool active = true);
void drawWindow(lgfx::LGFX_Device& lcd,
                const MacWindow& window);  // overloaded version for MacWindow struct
void drawButton(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const String& text,
                bool pressed = false);
void drawSymbolButton(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, SymbolType symbol,
                      bool pressed = false);
void drawSymbol(lgfx::LGFX_Device& lcd, int x, int y, int size, SymbolType symbol,
                uint16_t color = MAC_BLACK);
void draw3DFrame(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, bool inset = false);

// Desktop elements
void drawDesktopIcon(lgfx::LGFX_Device& lcd, int x, int y, const String& name,
                     bool selected = false);

// ===== WINDOW HELPERS =====
void interactiveWindow(lgfx::LGFX_Device& lcd, MacWindow& window);
bool isInsideCloseButton(const MacWindow& window, int tx, int ty);
bool isInsideMinimizeButton(const MacWindow& window, int tx, int ty);
bool isInsideTitleBar(const MacWindow& window, int tx, int ty);

// ===== FLEXIBLE COMPONENT MANAGEMENT =====
MacComponent* createComponent(ComponentType type, int x, int y, int w, int h, int id);
void addChildComponent(MacWindow& window, MacComponent* component);
void clearChildComponents(MacWindow& window);
void drawWindowChildComponents(lgfx::LGFX_Device& lcd, const MacWindow& window);
MacComponent* findComponentAt(const MacWindow& window, int x, int y);
void updateRunningTextComponents(lgfx::LGFX_Device& lcd, MacWindow& window);

// Component-specific drawing functions
void drawComponent(lgfx::LGFX_Device& lcd, const MacComponent& component, int windowX, int windowY);
void initComponentBuffer(lgfx::LGFX_Device* lcd, int maxWidth, int maxHeight);

// Individual component drawing functions (implemented in separate files)
void drawLabel(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const MacLabel& label);
void drawTextBox(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const MacTextBox& textbox);
void drawCheckBox(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const MacCheckBox& checkbox);
void drawSlider(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const MacSlider& slider);
void drawProgressBar(lgfx::LGFX_Device& lcd, int x, int y, int w, int h,
                     const MacProgressBar& progressBar);
void drawRunningText(lgfx::LGFX_Device& lcd, int x, int y, int w, int h,
                     MacRunningText& runningText);
void drawListView(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, MacListView& listView);
void drawInputField(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, MacInputField& inputField);
void drawKeyboard(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, MacKeyboard& keyboard);

// Component creation helpers
MacComponent* createButtonComponent(int x, int y, int w, int h, int id, const String& text,
                                    SymbolType symbol = SYMBOL_NONE);
MacComponent* createLabelComponent(int x, int y, int w, int h, int id, const String& text,
                                   uint16_t textColor = MAC_BLACK);
MacComponent* createRunningTextComponent(int x, int y, int w, int h, int id, const String& text,
                                         int scrollSpeed = 2, uint16_t textColor = MAC_BLACK,
                                         int textSize = 1);
MacComponent* createListViewComponent(int x, int y, int w, int h, int id, MacListViewItem* items,
                                      int itemCount, int itemHeight = 30);
MacComponent* createInputFieldComponent(int x, int y, int w, int h, int id,
                                        const String& placeholder = "", int maxLength = 50);
MacComponent* createKeyboardComponent(int x, int y, int w, int h, int id, int targetInputId);

// Helper to update running text properties
void updateRunningTextProperties(MacComponent* component, const String* newText = nullptr,
                                 int* newTextSize = nullptr, uint16_t* newTextColor = nullptr,
                                 uint16_t* newBgColor = nullptr, int* newScrollSpeed = nullptr,
                                 int* newPauseDuration = nullptr, FontType* newFont = nullptr);

// Helper to update input field components (cursor blinking)
void updateInputFieldComponents(lgfx::LGFX_Device& lcd, MacWindow& window);

// Helper to handle keyboard touch input
bool handleKeyboardTouch(lgfx::LGFX_Device& lcd, MacComponent* keyboardComponent,
                         MacComponent* inputComponent, int touchX, int touchY);

// ===== GENERIC WINDOW MANAGEMENT HELPERS =====
// Utility functions that can be called from user-defined callbacks
void handleWindowClose(lgfx::LGFX_Device& lcd, MacWindow& window,
                       DesktopIcon* associatedIcon = nullptr);
void handleWindowMinimize(lgfx::LGFX_Device& lcd, MacWindow& window,
                          DesktopIcon* associatedIcon = nullptr);
void handleIconClick(lgfx::LGFX_Device& lcd, MacWindow& window);
void handleWindowContentClick(lgfx::LGFX_Device& lcd, MacWindow& window, int relativeX,
                              int relativeY);
void handleWindowMoved(lgfx::LGFX_Device& lcd, MacWindow& window);

// ===== DESKTOP ICON HELPERS =====
void interactiveDesktopIcon(lgfx::LGFX_Device& lcd, DesktopIcon& icon);
bool isInsideDesktopIcon(const DesktopIcon& icon, int tx, int ty);
void redrawDesktopArea(lgfx::LGFX_Device& lcd);

// ===== WINDOW MANAGER =====
// Multi-window management for proper layering and refresh
void registerWindow(MacWindow* window);
void unregisterWindow(MacWindow* window);
void redrawAllWindows(lgfx::LGFX_Device& lcd);
void redrawAllWindowsExcept(lgfx::LGFX_Device& lcd, MacWindow* exceptWindow);
void showWindowOnTop(lgfx::LGFX_Device& lcd, MacWindow& window);

// ===== DOUBLE BUFFERING =====
// Internal helper for flicker-free window dragging
void renderToSprite(MacWindow* draggedWindow);

#endif
