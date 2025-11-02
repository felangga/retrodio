/*
 * MacUI.h - Classic Macintosh OS User Interface Library
 * 
 * This header file contains declarations for drawing classic Mac OS UI elements
 * using primitive graphics functions with LovyanGFX library.
 */

#ifndef MAC_UI_H
#define MAC_UI_H

#include <LovyanGFX.hpp>
#include "Arduino.h"
#include "wt32_sc01_plus.h"

// Forward declarations for screen dimensions (defined in wt32_sc01_plus.h)
extern const uint32_t screenWidth;
extern const uint32_t screenHeight;

// ===== CLASSIC MAC OS UI COLORS =====
#define MAC_WHITE     0xFFFF
#define MAC_BLACK     0x0000
#define MAC_GRAY      0x8410
#define MAC_LIGHT_GRAY 0xC618
#define MAC_DARK_GRAY 0x4208
#define MAC_BLUE      0x001F

// ===== COMPONENT TYPES =====
enum ComponentType {
  COMPONENT_BUTTON,
  COMPONENT_LABEL,
  COMPONENT_TEXTBOX,
  COMPONENT_CHECKBOX,
  COMPONENT_SLIDER,
  COMPONENT_PROGRESS_BAR,
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
  void (*onClick)(int componentId); // Generic callback with component ID
  void* customData; // Pointer to component-specific data
};

// ===== BUTTON STRUCT =====
struct MacButton {
  String text;
  SymbolType symbol;  // symbol to draw instead of text (SYMBOL_NONE for text buttons)
  bool pressed;  // current pressed state
};

// ===== LABEL STRUCT =====
struct MacLabel {
  String text;
  uint16_t textColor;
  uint16_t backgroundColor;
  int textSize;
  bool centerAlign;
};

// ===== TEXTBOX STRUCT =====
struct MacTextBox {
  String text;
  String placeholder;
  bool focused;
  int cursorPos;
  int maxLength;
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
  void (*onMinimize)(); // callback when minimize button is clicked
  void (*onClose)();    // callback when close button is clicked
  void (*onContentClick)(int relativeX, int relativeY); // callback for content clicks with relative coordinates
  void (*onWindowMoved)(); // callback when window position changes
  
  // Child components (flexible system)
  MacComponent** childComponents; // Array of pointers to components
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
  String iconType; // "window", "app", "file", etc.
  bool visible;
  bool selected;
  MacWindow* linkedWindow; // pointer to associated window
  void (*onClick)(); // callback when icon is clicked
};

// ===== FUNCTION DECLARATIONS =====

// Main UI drawing functions
void drawMenuBar(lgfx::LGFX_Device& lcd);
void drawCheckeredPattern(lgfx::LGFX_Device& lcd);
void drawClock(lgfx::LGFX_Device& lcd, const String& time); // added clock drawing

// Window and dialog functions
void drawWindow(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const String& title, bool active = true);
void drawWindow(lgfx::LGFX_Device& lcd, const MacWindow& window); // overloaded version for MacWindow struct
void drawButton(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const String& text, bool pressed = false);
void drawSymbolButton(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, SymbolType symbol, bool pressed = false);
void drawSymbol(lgfx::LGFX_Device& lcd, int x, int y, int size, SymbolType symbol, uint16_t color = MAC_BLACK);
void draw3DFrame(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, bool inset = false);
void drawScrollBar(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, bool vertical = true);

// Desktop elements
void drawDesktopIcon(lgfx::LGFX_Device& lcd, int x, int y, const String& name, bool selected = false);

// Music player elements
void drawSpectrumVisualization(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, bool active);

// Utility functions
void displayStatus(lgfx::LGFX_Device& lcd, const String& message, int y = 160);

// ===== WINDOW HELPERS =====
void interactiveWindow(lgfx::LGFX_Device& lcd, MacWindow& window);
bool isInsideCloseButton(const MacWindow& window, int tx, int ty);
bool isInsideMinimizeButton(const MacWindow& window, int tx, int ty);
bool isInsideTitleBar(const MacWindow& window, int tx, int ty);

// ===== FLEXIBLE COMPONENT MANAGEMENT =====
MacComponent* createComponent(ComponentType type, int x, int y, int w, int h, int id);
void addChildComponent(MacWindow& window, MacComponent* component);
void removeChildComponent(MacWindow& window, MacComponent* component);
void clearChildComponents(MacWindow& window);
void drawWindowChildComponents(lgfx::LGFX_Device& lcd, const MacWindow& window);
MacComponent* findComponentAt(const MacWindow& window, int x, int y);

// Component-specific drawing functions
void drawComponent(lgfx::LGFX_Device& lcd, const MacComponent& component, int windowX, int windowY);
void drawLabel(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const MacLabel& label);
void drawTextBox(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const MacTextBox& textbox);
void drawCheckBox(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const MacCheckBox& checkbox);
void drawSlider(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const MacSlider& slider);
void drawProgressBar(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const MacProgressBar& progressBar);

// Component creation helpers
MacComponent* createButtonComponent(int x, int y, int w, int h, int id, const String& text, SymbolType symbol = SYMBOL_NONE);
MacComponent* createLabelComponent(int x, int y, int w, int h, int id, const String& text, uint16_t textColor = MAC_BLACK);
MacComponent* createTextBoxComponent(int x, int y, int w, int h, int id, const String& placeholder = "");
MacComponent* createCheckBoxComponent(int x, int y, int w, int h, int id, const String& label, bool checked = false);
MacComponent* createSliderComponent(int x, int y, int w, int h, int id, int minVal, int maxVal, int currentVal, bool vertical = false);
MacComponent* createProgressBarComponent(int x, int y, int w, int h, int id, int minVal, int maxVal, int currentVal);

// ===== GENERIC WINDOW MANAGEMENT HELPERS =====
// Utility functions that can be called from user-defined callbacks
void handleWindowMinimize(lgfx::LGFX_Device& lcd, MacWindow& window, DesktopIcon* associatedIcon = nullptr);
void handleWindowClose(lgfx::LGFX_Device& lcd, MacWindow& window, DesktopIcon* associatedIcon = nullptr);
void handleIconClick(lgfx::LGFX_Device& lcd, MacWindow& window);
void handleWindowContentClick(lgfx::LGFX_Device& lcd, MacWindow& window, int relativeX, int relativeY);
void handleWindowMoved(lgfx::LGFX_Device& lcd, MacWindow& window);

// ===== DESKTOP ICON HELPERS =====
void interactiveDesktopIcon(lgfx::LGFX_Device& lcd, DesktopIcon& icon);
bool isInsideDesktopIcon(const DesktopIcon& icon, int tx, int ty);
void redrawDesktopArea(lgfx::LGFX_Device& lcd, int x, int y, int w, int h);

#endif // MAC_UI_H
