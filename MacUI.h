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

// ===== BUTTON STRUCT =====
struct MacButton {
  int x;
  int y;
  int w;
  int h;
  String text;
  SymbolType symbol;  // symbol to draw instead of text (SYMBOL_NONE for text buttons)
  int id;        // application-specific identifier
  bool pressed;  // current pressed state
  void (*onClick)(); // callback function when pressed
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
  
  // Child components (buttons, etc.)
  MacButton** childButtons; // Array of pointers to buttons
  int childButtonCount;     // Number of child buttons
  
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

// Helper function to redraw window content after move (implemented in main application)
extern void redrawWindowContent(lgfx::LGFX_Device& lcd, const MacWindow& window);

// ===== BUTTON HELPERS =====
bool isInsideButton(const MacButton& btn, int tx, int ty);
void redrawButton(lgfx::LGFX_Device& lcd, MacButton& btn);
void interactiveButton(lgfx::LGFX_Device& lcd, MacButton& btn);

// ===== WINDOW HELPERS =====
void interactiveWindow(lgfx::LGFX_Device& lcd, MacWindow& window);
bool isInsideCloseButton(const MacWindow& window, int tx, int ty);
bool isInsideMinimizeButton(const MacWindow& window, int tx, int ty);
bool isInsideTitleBar(const MacWindow& window, int tx, int ty);

// ===== CHILD COMPONENT MANAGEMENT =====
void addChildButton(MacWindow& window, MacButton* button);
void removeChildButton(MacWindow& window, MacButton* button);
void clearChildButtons(MacWindow& window);
void drawWindowChildButtons(lgfx::LGFX_Device& lcd, const MacWindow& window);
MacButton* findButtonAt(const MacWindow& window, int x, int y);

// ===== DESKTOP ICON HELPERS =====
void interactiveDesktopIcon(lgfx::LGFX_Device& lcd, DesktopIcon& icon);
bool isInsideDesktopIcon(const DesktopIcon& icon, int tx, int ty);
void redrawDesktopArea(lgfx::LGFX_Device& lcd, int x, int y, int w, int h);

#endif // MAC_UI_H
