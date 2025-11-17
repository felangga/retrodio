/*
 * MacUI.cpp - Classic Macintosh OS User Interface Library Implementation
 * 
 * Copyright (c) 2025 Felangga
 * 
 * This file contains the implementation of classic Mac OS UI drawing functions
 * using primitive graphics functions with LovyanGFX library.
 */

#include "MacUI.h"

/**
 * Draw the classic Mac OS menu bar
 */
void drawMenuBar(lgfx::LGFX_Device& lcd, const String& appName) {
  // Draw menu bar background
  lcd.fillRect(0, 0, screenWidth, 20, MAC_WHITE);
  lcd.drawFastHLine(0, 20, screenWidth, MAC_BLACK);

  lcd.fillCircle(15, 10, 6, MAC_BLACK);
  lcd.fillCircle(18, 7, 3, MAC_WHITE);

  // Draw menu items
  lcd.setTextColor(MAC_BLACK, MAC_WHITE);
  lcd.setTextSize(1);
  lcd.setCursor(30, 6);
  lcd.print(appName);
}

void drawClock(lgfx::LGFX_Device& lcd, const String& time) {
  lcd.fillRect(screenWidth - 80, 0, 80, 20, MAC_WHITE);
  lcd.setCursor(screenWidth - 80, 6);
  lcd.print(time);
}

/**
 * Draw a classic Mac OS window with title bar
 */
void drawWindow(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const String& title, bool active) {
  // Draw window shadow
  lcd.fillRect(x + 3, y + 3, w, h, MAC_DARK_GRAY);

  // Draw window background
  lcd.fillRect(x, y, w, h, MAC_WHITE);

  // Draw window border
  lcd.drawRect(x, y, w, h, MAC_BLACK);
  lcd.drawRect(x + 1, y + 1, w - 2, h - 2, MAC_BLACK);

  // Draw title bar (increased height for larger buttons)
  uint16_t titleColor = active ? MAC_BLACK : MAC_GRAY;
  lcd.fillRect(x + 2, y + 2, w - 4, 24, titleColor);

  // Draw title text (adjusted for larger title bar)
  lcd.setTextColor(MAC_WHITE, titleColor);
  lcd.setTextSize(1);
  int titleX = x + (w - title.length() * 6) / 2;
  lcd.setCursor(titleX, y + 10);  // Moved down to center in larger title bar
  lcd.print(title);

  lcd.fillRect(x + w - 24, y + 4, 18, 18, MAC_WHITE);
  lcd.drawRect(x + w - 24, y + 4, 18, 18, MAC_BLACK);

  // Draw minimize symbol (−)
  lcd.drawFastHLine(x + w - 18, y + 12, 8, MAC_BLACK);
}

/**
 * Draw a classic Mac OS window using MacWindow struct
 */
void drawWindow(lgfx::LGFX_Device& lcd, const MacWindow& window) {
  if (!window.visible) return;

  // Draw full window
  drawWindow(lcd, window.x, window.y, window.w, window.h, window.title, window.active);
  
  // Draw all child components (flexible system)
  drawWindowChildComponents(lcd, window);
}

/**
 * Draw a classic Mac OS button with rounded corners and ellipsis
 */
void drawButton(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const String& text, bool pressed) {
  // Choose colors based on pressed state - inverted when pressed
  uint16_t bgColor = pressed ? MAC_BLACK : MAC_WHITE;
  uint16_t textColor = pressed ? MAC_WHITE : MAC_BLACK;
  uint16_t borderColor = MAC_BLACK;

  // Draw button background with rounded corners effect
  lcd.fillRect(x + 1, y, w - 2, h, bgColor);
  lcd.fillRect(x, y + 1, w, h - 2, bgColor);

  // Draw rounded corners (simple 1-pixel corner cuts)
  lcd.drawPixel(x + 1, y + 1, bgColor);
  lcd.drawPixel(x + w - 2, y + 1, bgColor);
  lcd.drawPixel(x + 1, y + h - 2, bgColor);
  lcd.drawPixel(x + w - 2, y + h - 2, bgColor);

  // Draw border with rounded corners
  lcd.drawFastHLine(x + 2, y, w - 4, borderColor);          // top
  lcd.drawFastHLine(x + 2, y + h - 1, w - 4, borderColor);  // bottom
  lcd.drawFastVLine(x, y + 2, h - 4, borderColor);          // left
  lcd.drawFastVLine(x + w - 1, y + 2, h - 4, borderColor);  // right

  // Corner pixels for rounded effect
  lcd.drawPixel(x + 1, y + 1, borderColor);
  lcd.drawPixel(x + w - 2, y + 1, borderColor);
  lcd.drawPixel(x + 1, y + h - 2, borderColor);
  lcd.drawPixel(x + w - 2, y + h - 2, borderColor);

  // Draw button text (larger text for bigger buttons)
  lcd.setTextColor(textColor, bgColor);
  lcd.setTextSize(2);                            // Increased from 1 to 2 for better readability
  int textX = x + (w - text.length() * 12) / 2;  // Adjusted for larger text (12 pixels wide)
  int textY = y + (h - 16) / 2;                  // Adjusted for larger text (16 pixels high)
  lcd.setCursor(textX, textY);
  lcd.print(text);

  // Draw ellipsis (three dots) below text for classic Mac look
  if (h > 24) {                     // Only if button is tall enough (adjusted threshold for larger text)
    int ellipsisY = textY + 18;     // Adjusted spacing for larger text
    int ellipsisX = x + w / 2 - 6;  // Center the ellipsis
    for (int i = 0; i < 3; i++) {
      lcd.fillCircle(ellipsisX + i * 4, ellipsisY, 1, textColor);
    }
  }
}

// ===== WINDOW INTERACTION HELPERS =====

bool isInsideCloseButton(const MacWindow& window, int tx, int ty) {
  if (!window.visible) return false;
  int buttonHeight = 16;  // Now consistent 16 pixels for both normal and minimized
  int buttonY = window.y + 4;
  return tx >= window.x + 4 && tx < window.x + 20 && ty >= buttonY && ty < buttonY + buttonHeight;
}

bool isInsideMinimizeButton(const MacWindow& window, int tx, int ty) {
  if (!window.visible) return false;
  int buttonHeight = 18;  // Now consistent 16 pixels for both normal and minimized
  int buttonY = window.y + 4;
  return tx >= window.x + window.w - 24 && tx < window.x + window.w - 6 && ty >= buttonY && ty < buttonY + buttonHeight;
}

bool isInsideTitleBar(const MacWindow& window, int tx, int ty) {
  if (!window.visible) return false;
  int titleBarHeight = 24;

  // Check if in title bar but not in close or minimize buttons
  bool inTitleBar = tx >= window.x + 2 && tx < window.x + window.w - 2 && ty >= window.y + 2 && ty < window.y + 2 + titleBarHeight;
  bool inCloseButton = isInsideCloseButton(window, tx, ty);
  bool inMinimizeButton = isInsideMinimizeButton(window, tx, ty);

  return inTitleBar && !inCloseButton && !inMinimizeButton;
}

/**
 * Interactive window that handles touch for minimize/close buttons and dragging
 * Also manages all interactive components inside the window
 * Call this every loop for each window
 */
void interactiveWindow(lgfx::LGFX_Device& lcd, MacWindow& window) {
  if (!window.visible) return;

  static bool wasPressed = false;
  static unsigned long pressTime = 0;

  uint16_t tx, ty;
  bool touching = lcd.getTouch(&tx, &ty);

  if (touching) {
    lcd.drawPixel(tx, ty, MAC_BLACK);

    // Check if touch is inside window bounds
    bool insideWindow = tx >= window.x && tx < window.x + window.w && 
                       ty >= window.y && ty < window.y + window.h;

    if (insideWindow) {
      if (!wasPressed) {
        wasPressed = true;
        pressTime = millis();

        // Check if minimize button was pressed
        if (isInsideMinimizeButton(window, tx, ty)) {
          delay(100);                            // Brief visual feedback
          window.minimized = !window.minimized;  // Toggle minimize state

          if (window.minimized) {
            // Hide the window completely and clear its area
            window.visible = false;
            redrawDesktopArea(lcd, window.x, window.y, window.w, window.h);
          } else {
            // Restore the window
            window.visible = true;
          }

          if (window.onMinimize) {
            window.onMinimize();
          }

          // Redraw the window if being restored
          if (!window.minimized && window.visible) {
            drawWindow(lcd, window);
          }

          wasPressed = false;
          return;
        }

        // Check if title bar was pressed (for dragging)
        if (isInsideTitleBar(window, tx, ty)) {
          // Start dragging
          window.isDragging = true;
          window.dragOffsetX = tx - window.x;
          window.dragOffsetY = ty - window.y;
          return;
        }

        // Handle window content interactions (buttons, etc.)
        // This allows the window to manage all its child components
        if (window.onContentClick) {
          window.onContentClick(tx - window.x, ty - window.y); // Pass relative coordinates
        }
      }

      // Handle ongoing drag
      if (window.isDragging) {
        static unsigned long lastDragUpdate = 0;
        unsigned long now = millis();

        // Limit drag update frequency to reduce flicker
        if (now - lastDragUpdate < 30) {  // Max 33 FPS for smooth movement
          return;
        }
        lastDragUpdate = now;

        int newX = tx - window.dragOffsetX;
        int newY = ty - window.dragOffsetY;

        // Constrain window to screen bounds
        newX = max(0, min(newX, (int)screenWidth - window.w));
        newY = max(21, min(newY, (int)screenHeight - window.h));  // 21 to leave menu bar visible

        // Only update if position actually changed significantly (reduce micro-movements)
        if (abs(newX - window.x) > 2 || abs(newY - window.y) > 2) {
          // Store old position
          int oldX = window.x;
          int oldY = window.y;

          // Update position
          window.x = newX;
          window.y = newY;

          // Simple direct drawing approach
          redrawDesktopArea(lcd, oldX, oldY, window.w + 5, window.h + 5);
          drawWindow(lcd, window);
          
          // Notify that window moved (so buttons can update positions)
          if (window.onWindowMoved) {
            window.onWindowMoved();
          }
        }
      }

      // Reset press state after timeout
      if (millis() - pressTime > 500) {
        wasPressed = false;
      }
    }
  } else {
    // Touch released - stop dragging
    if (window.isDragging) {
      window.isDragging = false;
    }
    wasPressed = false;
  }
}

/**
 * Draw checkered pattern background (classic Mac desktop)
 */
void drawCheckeredPattern(lgfx::LGFX_Device& lcd) {
  int patternSize = 3;
  for (int y = 21; y < screenHeight; y += patternSize) {
    for (int x = 0; x < screenWidth; x += patternSize) {
      if ((x / patternSize + y / patternSize) % 2 == 0) {
        lcd.fillRect(x, y, patternSize, patternSize, MAC_LIGHT_GRAY);
      }
    }
  }
}

/**
 * Draw a simple desktop icon
 */
void drawDesktopIcon(lgfx::LGFX_Device& lcd, int x, int y, const String& name, bool selected) {
  // Choose colors based on selection state
  uint16_t bgColor = selected ? MAC_BLACK : MAC_WHITE;
  uint16_t textColor = selected ? MAC_WHITE : MAC_BLACK;
  uint16_t iconColor = selected ? MAC_WHITE : MAC_LIGHT_GRAY;

  // Draw icon background
  lcd.fillRect(x, y, 32, 32, bgColor);
  lcd.drawRect(x, y, 32, 32, MAC_BLACK);

  // Draw a simple icon representation
  if (name == "HD") {
    lcd.fillRect(x + 4, y + 4, 24, 16, iconColor);
    lcd.drawRect(x + 4, y + 4, 24, 16, MAC_BLACK);
  } else if (name == "Trash") {
    lcd.fillRect(x + 8, y + 8, 16, 16, iconColor);
    lcd.drawRect(x + 8, y + 8, 16, 16, MAC_BLACK);
  } else if (name == "Radio") {
    lcd.fillCircle(x + 16, y + 16, 8, iconColor);
    lcd.drawCircle(x + 16, y + 16, 8, MAC_BLACK);
    // Add radio waves for better identification
    lcd.drawCircle(x + 16, y + 16, 12, MAC_BLACK);
    lcd.drawCircle(x + 16, y + 16, 15, MAC_BLACK);
  }

  // Draw icon label with selection-aware background
  if (selected) {
    lcd.fillRect(x - 2, y + 34, 36, 12, MAC_BLACK);
  }
  lcd.setTextColor(textColor, selected ? MAC_BLACK : MAC_WHITE);
  lcd.setTextSize(1);
  int labelX = x + (32 - name.length() * 6) / 2;
  lcd.setCursor(labelX, y + 36);
  lcd.print(name);
}

/**
 * Draw 3D frame (inset or outset)
 */
void draw3DFrame(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, bool inset) {
  if (inset) {
    // Inset frame (shadow on top/left, highlight on bottom/right)
    lcd.drawFastHLine(x, y, w, MAC_DARK_GRAY);
    lcd.drawFastVLine(x, y, h, MAC_DARK_GRAY);
    lcd.drawFastHLine(x, y + h - 1, w, MAC_WHITE);
    lcd.drawFastVLine(x + w - 1, y, h, MAC_WHITE);
  } else {
    // Outset frame (highlight on top/left, shadow on bottom/right)
    lcd.drawFastHLine(x, y, w, MAC_WHITE);
    lcd.drawFastVLine(x, y, h, MAC_WHITE);
    lcd.drawFastHLine(x, y + h - 1, w, MAC_DARK_GRAY);
    lcd.drawFastVLine(x + w - 1, y, h, MAC_DARK_GRAY);
  }
}

/**
 * Draw classic scroll bar
 */
void drawScrollBar(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, bool vertical) {
  // Draw scroll bar background
  lcd.fillRect(x, y, w, h, MAC_LIGHT_GRAY);

  if (vertical) {
    // Draw scroll arrows
    lcd.fillRect(x, y, w, 16, MAC_GRAY);
    lcd.fillRect(x, y + h - 16, w, 16, MAC_GRAY);

    // Draw scroll thumb
    int thumbY = y + 20;
    int thumbH = 20;
    lcd.fillRect(x + 1, thumbY, w - 2, thumbH, MAC_WHITE);
    draw3DFrame(lcd, x + 1, thumbY, w - 2, thumbH, false);
  } else {
    // Horizontal scroll bar
    lcd.fillRect(x, y, 16, h, MAC_GRAY);
    lcd.fillRect(x + w - 16, y, 16, h, MAC_GRAY);

    int thumbX = x + 20;
    int thumbW = 20;
    lcd.fillRect(thumbX, y + 1, thumbW, h - 2, MAC_WHITE);
    draw3DFrame(lcd, thumbX, y + 1, thumbW, h - 2, false);
  }
}

/**
 * Display status message on screen
 * @param message The message to display
 * @param y Y coordinate for the message (default: 160)
 */
void displayStatus(lgfx::LGFX_Device& lcd, const String& message, int y) {
  // Clear the status area in the radio window (adjusted for landscape)
  lcd.fillRect(45, y, 175, 15, MAC_WHITE);

  // Display new message in classic Mac style
  lcd.setTextColor(MAC_BLACK, MAC_WHITE);
  lcd.setTextSize(1);
  lcd.setCursor(45, y);
  lcd.println(message);
}

// ===== DESKTOP ICON IMPLEMENTATION =====

/**
 * Redraw desktop area with proper background pattern
 */
void redrawDesktopArea(lgfx::LGFX_Device& lcd, int x, int y, int w, int h) {
  drawCheckeredPattern(lcd);
}

/**
 * Check if touch is inside desktop icon bounds
 */
bool isInsideDesktopIcon(const DesktopIcon& icon, int tx, int ty) {
  if (!icon.visible) return false;
  return tx >= icon.x && tx < icon.x + 64 &&  // icon is 64x64 (32x32 + label)
         ty >= icon.y && ty < icon.y + 50;    // 32px icon + 18px label
}

/**
 * Interactive desktop icon that handles touch for icon clicks
 * Call this every loop for each desktop icon
 */
void interactiveDesktopIcon(lgfx::LGFX_Device& lcd, DesktopIcon& icon) {
  if (!icon.visible) return;

  static bool wasPressed = false;
  static unsigned long pressTime = 0;

  uint16_t tx, ty;
  bool touching = lcd.getTouch(&tx, &ty);

  if (touching) {
    // Check if touch is inside icon bounds
    bool insideIcon = isInsideDesktopIcon(icon, tx, ty);

    if (insideIcon) {
      if (!wasPressed) {
        wasPressed = true;
        pressTime = millis();

        // Select the icon
        icon.selected = true;
        drawDesktopIcon(lcd, icon.x, icon.y, icon.name, true);

        // Call the icon's callback after a brief delay for visual feedback
        delay(100);
        if (icon.onClick) {
          icon.onClick();
        }

        // Deselect the icon
        icon.selected = false;
        if (icon.visible) {  // Only redraw if still visible
          drawDesktopIcon(lcd, icon.x, icon.y, icon.name, false);
        }

        wasPressed = false;
      }

      // Reset press state after timeout
      if (millis() - pressTime > 500) {
        wasPressed = false;
      }
    }
  } else {
    wasPressed = false;
  }
}

/**
 * Draw a symbol (play, pause, stop, etc.) at the specified position
 */
void drawSymbol(lgfx::LGFX_Device& lcd, int x, int y, int size, SymbolType symbol, uint16_t color) {
  switch (symbol) {
    case SYMBOL_PLAY:
      for (int i = 0; i < size; i++) {
        int lineHeight = ((size - i) * size) / size;
        lcd.drawFastVLine(x + i, y + (size - lineHeight) / 2, lineHeight, color);
      }
      break;

    case SYMBOL_PAUSE:
      {
        int barWidth = size / 4;
        int spacing = size / 3;
        lcd.fillRect(x, y, barWidth, size, color);
        lcd.fillRect(x + barWidth + spacing, y, barWidth, size, color);
      }
      break;

    case SYMBOL_STOP:
      lcd.fillRect(x, y, size, size, color);
      break;

    case SYMBOL_PREV:
      {
        lcd.fillRect(x, y, 2, size, color); 
        int triSize = size - 4;
        for (int i = 0; i < triSize; i++) {
          int lineHeight = ((i + 1) * size) / triSize;
          lcd.drawFastVLine(x + 3 + i, y + (size - lineHeight) / 2, lineHeight, color);
        }
      }
      break;

    case SYMBOL_NEXT:
      {
        int triSize = size - 4;
        for (int i = 0; i < triSize; i++) {
          int lineHeight = ((triSize - i) * size) / triSize;
          lcd.drawFastVLine(x + i, y + (size - lineHeight) / 2, lineHeight, color);
        }
        lcd.fillRect(x + size - 2, y, 2, size, color);  
      }
      break;

    case SYMBOL_VOL_UP:
      lcd.fillRect(x, y + size / 3, size / 3, size / 3, color);
      lcd.fillRect(x + size / 3, y + size / 4, size / 6, size / 2, color);
      // Sound waves
      for (int i = 1; i <= 3; i++) {
        int waveX = x + size / 2 + i * size / 8;
        int waveY = y + size / 2 - i * size / 12;
        int waveH = i * size / 6;
        lcd.drawFastVLine(waveX, waveY, waveH, color);
      }
      break;

    case SYMBOL_VOL_DOWN:
      lcd.fillRect(x, y + size / 3, size / 3, size / 3, color);
      lcd.fillRect(x + size / 3, y + size / 4, size / 6, size / 2, color);
      lcd.fillRect(x + size / 2 + 2, y + size / 2 - 1, size / 4, 2, color);
      break;

    case SYMBOL_NONE:
    default:
      break;
  }
}

/**
 * Draw a button with a symbol instead of text
 */
void drawSymbolButton(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, SymbolType symbol, bool pressed) {
  // Inverted colors when pressed
  uint16_t bgColor = pressed ? MAC_BLACK : MAC_WHITE;
  uint16_t borderColor = MAC_BLACK;
  uint16_t symbolColor = pressed ? MAC_WHITE : MAC_BLACK;

  lcd.fillRect(x + 1, y + 1, w - 2, h - 2, bgColor);

  // Clear corner pixels for rounded effect
  lcd.drawPixel(x, y, MAC_WHITE);
  lcd.drawPixel(x + w - 1, y, MAC_WHITE);
  lcd.drawPixel(x, y + h - 1, MAC_WHITE);
  lcd.drawPixel(x + w - 1, y + h - 1, MAC_WHITE);
  lcd.drawPixel(x + 1, y + 1, bgColor);
  lcd.drawPixel(x + w - 2, y + 1, bgColor);
  lcd.drawPixel(x + 1, y + h - 2, bgColor);
  lcd.drawPixel(x + w - 2, y + h - 2, bgColor);

  // Draw border with rounded corners
  lcd.drawFastHLine(x + 2, y, w - 4, borderColor);          // top
  lcd.drawFastHLine(x + 2, y + h - 1, w - 4, borderColor);  // bottom
  lcd.drawFastVLine(x, y + 2, h - 4, borderColor);          // left
  lcd.drawFastVLine(x + w - 1, y + 2, h - 4, borderColor);  // right

  // Corner pixels for rounded effect
  lcd.drawPixel(x + 1, y + 1, borderColor);
  lcd.drawPixel(x + w - 2, y + 1, borderColor);
  lcd.drawPixel(x + 1, y + h - 2, borderColor);
  lcd.drawPixel(x + w - 2, y + h - 2, borderColor);

  // Draw the symbol centered in the button
  int symbolSize = min(w, h) - 32;  // Leave some padding
  int symbolX = x + (w - symbolSize) / 2;
  int symbolY = y + (h - symbolSize) / 2;

  drawSymbol(lcd, symbolX, symbolY, symbolSize, symbol, symbolColor);
}

// Music player visualization functions
void drawSpectrumVisualization(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, bool active) {
  // Draw a fake spectrum analyzer visualization
  draw3DFrame(lcd, x, y, w, h, true);

  if (active) {
    // Draw animated spectrum bars
    static unsigned long lastUpdate = 0;
    static int bars[10] = { 5, 8, 12, 6, 15, 9, 11, 4, 7, 13 };

    if (millis() - lastUpdate > 100) {  // Update every 100ms
      for (int i = 0; i < 10; i++) {
        bars[i] = random(3, h - 8);
      }
      lastUpdate = millis();
    }

    // Clear the visualization area
    lcd.fillRect(x + 2, y + 2, w - 4, h - 4, MAC_WHITE);

    // Draw spectrum bars
    int barWidth = (w - 4) / 10;
    for (int i = 0; i < 10; i++) {
      int barHeight = bars[i];
      int barX = x + 2 + i * barWidth;
      int barY = y + h - 2 - barHeight;

      // Color gradient from green to red based on height
      uint16_t color = MAC_BLACK;
      if (barHeight > h / 2) {
        color = 0xF800;  // Red for high values
      } else if (barHeight > h / 3) {
        color = 0xFFE0;  // Yellow for medium values
      } else {
        color = 0x07E0;  // Green for low values
      }

      lcd.fillRect(barX, barY, barWidth - 1, barHeight, color);
    }
  } else {
    // Clear the visualization area when not active
    lcd.fillRect(x + 2, y + 2, w - 4, h - 4, MAC_WHITE);
  }
}

// ===== FLEXIBLE COMPONENT MANAGEMENT =====

MacComponent* createComponent(ComponentType type, int x, int y, int w, int h, int id) {
  MacComponent* component = new MacComponent();
  component->type = type;
  component->x = x;
  component->y = y;
  component->w = w;
  component->h = h;
  component->id = id;
  component->visible = true;
  component->enabled = true;
  component->onClick = nullptr;
  component->customData = nullptr;
  return component;
}

void addChildComponent(MacWindow& window, MacComponent* component) {
  // Allocate or reallocate the child components array
  MacComponent** newArray = new MacComponent*[window.childComponentCount + 1];
  
  // Copy existing components to new array
  for (int i = 0; i < window.childComponentCount; i++) {
    newArray[i] = window.childComponents[i];
  }
  
  // Add new component
  newArray[window.childComponentCount] = component;
  
  // Clean up old array if it exists
  if (window.childComponents != nullptr) {
    delete[] window.childComponents;
  }
  
  // Update window
  window.childComponents = newArray;
  window.childComponentCount++;
}

void removeChildComponent(MacWindow& window, MacComponent* component) {
  if (window.childComponentCount == 0 || window.childComponents == nullptr) {
    return;
  }
  
  // Find the component index
  int removeIndex = -1;
  for (int i = 0; i < window.childComponentCount; i++) {
    if (window.childComponents[i] == component) {
      removeIndex = i;
      break;
    }
  }
  
  if (removeIndex == -1) {
    return; // Component not found
  }
  
  // Create new array with one less element
  if (window.childComponentCount == 1) {
    // Last component being removed
    delete[] window.childComponents;
    window.childComponents = nullptr;
    window.childComponentCount = 0;
  } else {
    MacComponent** newArray = new MacComponent*[window.childComponentCount - 1];
    
    // Copy components except the one being removed
    int newIndex = 0;
    for (int i = 0; i < window.childComponentCount; i++) {
      if (i != removeIndex) {
        newArray[newIndex++] = window.childComponents[i];
      }
    }
    
    delete[] window.childComponents;
    window.childComponents = newArray;
    window.childComponentCount--;
  }
}

void clearChildComponents(MacWindow& window) {
  if (window.childComponents != nullptr) {
    // Clean up each component's custom data
    for (int i = 0; i < window.childComponentCount; i++) {
      if (window.childComponents[i] && window.childComponents[i]->customData) {
        delete window.childComponents[i]->customData;
      }
      delete window.childComponents[i];
    }
    delete[] window.childComponents;
    window.childComponents = nullptr;
  }
  window.childComponentCount = 0;
}

void drawWindowChildComponents(lgfx::LGFX_Device& lcd, const MacWindow& window) {
  if (window.childComponents == nullptr || window.childComponentCount == 0) {
    return;
  }
  
  for (int i = 0; i < window.childComponentCount; i++) {
    MacComponent* component = window.childComponents[i];
    if (component != nullptr && component->visible) {
      drawComponent(lcd, *component, window.x, window.y);
    }
  }
}

MacComponent* findComponentAt(const MacWindow& window, int x, int y) {
  if (window.childComponents == nullptr || window.childComponentCount == 0) {
    return nullptr;
  }
  
  // Convert absolute coordinates to window-relative coordinates
  int relativeX = x - window.x;
  int relativeY = y - window.y;
  
  for (int i = 0; i < window.childComponentCount; i++) {
    MacComponent* component = window.childComponents[i];
    if (component != nullptr && component->visible && component->enabled) {
      if (relativeX >= component->x && relativeX < component->x + component->w &&
          relativeY >= component->y && relativeY < component->y + component->h) {
        return component;
      }
    }
  }
  
  return nullptr;
}

// ===== COMPONENT DRAWING FUNCTIONS =====

void drawComponent(lgfx::LGFX_Device& lcd, const MacComponent& component, int windowX, int windowY) {
  if (!component.visible) return;
  
  int absoluteX = windowX + component.x;
  int absoluteY = windowY + component.y;
  
  switch (component.type) {
    case COMPONENT_BUTTON:
      // Draw button using component data
      if (component.customData != nullptr) {
        MacButton* btnData = (MacButton*)component.customData;
        if (btnData->symbol != SYMBOL_NONE) {
          drawSymbolButton(lcd, absoluteX, absoluteY, component.w, component.h, btnData->symbol, btnData->pressed);
        } else {
          drawButton(lcd, absoluteX, absoluteY, component.w, component.h, btnData->text, btnData->pressed);
        }
      }
      break;
      
    case COMPONENT_LABEL:
      if (component.customData != nullptr) {
        MacLabel* labelData = (MacLabel*)component.customData;
        drawLabel(lcd, absoluteX, absoluteY, component.w, component.h, *labelData);
      }
      break;
      
    case COMPONENT_TEXTBOX:
      if (component.customData != nullptr) {
        MacTextBox* textboxData = (MacTextBox*)component.customData;
        drawTextBox(lcd, absoluteX, absoluteY, component.w, component.h, *textboxData);
      }
      break;
      
    case COMPONENT_CHECKBOX:
      if (component.customData != nullptr) {
        MacCheckBox* checkboxData = (MacCheckBox*)component.customData;
        drawCheckBox(lcd, absoluteX, absoluteY, component.w, component.h, *checkboxData);
      }
      break;
      
    case COMPONENT_SLIDER:
      if (component.customData != nullptr) {
        MacSlider* sliderData = (MacSlider*)component.customData;
        drawSlider(lcd, absoluteX, absoluteY, component.w, component.h, *sliderData);
      }
      break;
      
    case COMPONENT_PROGRESS_BAR:
      if (component.customData != nullptr) {
        MacProgressBar* progressData = (MacProgressBar*)component.customData;
        drawProgressBar(lcd, absoluteX, absoluteY, component.w, component.h, *progressData);
      }
      break;
      
    case COMPONENT_CUSTOM:
      // Custom components can implement their own drawing logic
      // For now, just draw a placeholder rectangle
      lcd.drawRect(absoluteX, absoluteY, component.w, component.h, MAC_GRAY);
      break;
      
    default:
      // Unknown component type - draw placeholder
      lcd.drawRect(absoluteX, absoluteY, component.w, component.h, MAC_BLACK);
      break;
  }
}

void drawLabel(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const MacLabel& label) {
  // Draw background
  lcd.fillRect(x, y, w, h, label.backgroundColor);
  
  // Draw text
  lcd.setTextColor(label.textColor, label.backgroundColor);
  lcd.setTextSize(label.textSize);
  
  int textX, textY;
  if (label.centerAlign) {
    textX = x + (w - label.text.length() * 6 * label.textSize) / 2;
    textY = y + (h - 8 * label.textSize) / 2;
  } else {
    textX = x + 2;
    textY = y + (h - 8 * label.textSize) / 2;
  }
  
  lcd.setCursor(textX, textY);
  lcd.print(label.text);
}

void drawTextBox(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const MacTextBox& textbox) {
  // Draw background and border
  uint16_t borderColor = textbox.focused ? MAC_BLUE : MAC_BLACK;
  uint16_t bgColor = MAC_WHITE;
  
  lcd.fillRect(x, y, w, h, bgColor);
  lcd.drawRect(x, y, w, h, borderColor);
  draw3DFrame(lcd, x + 1, y + 1, w - 2, h - 2, true);
  
  // Draw text or placeholder
  lcd.setTextColor(MAC_BLACK, bgColor);
  lcd.setTextSize(1);
  lcd.setCursor(x + 4, y + (h - 8) / 2);
  
  if (textbox.text.length() > 0) {
    lcd.print(textbox.text);
    
    // Draw cursor if focused
    if (textbox.focused) {
      int cursorX = x + 4 + textbox.cursorPos * 6;
      lcd.drawFastVLine(cursorX, y + 2, h - 4, MAC_BLACK);
    }
  } else if (textbox.placeholder.length() > 0) {
    lcd.setTextColor(MAC_GRAY, bgColor);
    lcd.print(textbox.placeholder);
  }
}

void drawCheckBox(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const MacCheckBox& checkbox) {
  int boxSize = min(16, min(w, h));
  int boxX = x + 2;
  int boxY = y + (h - boxSize) / 2;
  
  // Draw checkbox box
  lcd.fillRect(boxX, boxY, boxSize, boxSize, MAC_WHITE);
  lcd.drawRect(boxX, boxY, boxSize, boxSize, MAC_BLACK);
  draw3DFrame(lcd, boxX + 1, boxY + 1, boxSize - 2, boxSize - 2, true);
  
  // Draw checkmark if checked
  if (checkbox.checked) {
    // Simple checkmark
    lcd.drawLine(boxX + 3, boxY + boxSize/2, boxX + boxSize/2, boxY + boxSize - 4, MAC_BLACK);
    lcd.drawLine(boxX + boxSize/2, boxY + boxSize - 4, boxX + boxSize - 3, boxY + 3, MAC_BLACK);
  }
  
  // Draw label
  lcd.setTextColor(MAC_BLACK, MAC_WHITE);
  lcd.setTextSize(1);
  lcd.setCursor(boxX + boxSize + 4, boxY + (boxSize - 8) / 2);
  lcd.print(checkbox.label);
}

void drawSlider(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const MacSlider& slider) {
  if (slider.vertical) {
    // Vertical slider
    int trackX = x + w / 2 - 2;
    int trackW = 4;
    int trackH = h - 20;
    int trackY = y + 10;
    
    // Draw track
    lcd.fillRect(trackX, trackY, trackW, trackH, MAC_LIGHT_GRAY);
    draw3DFrame(lcd, trackX, trackY, trackW, trackH, true);
    
    // Calculate thumb position
    int range = slider.maxValue - slider.minValue;
    int thumbY = trackY + trackH - ((slider.currentValue - slider.minValue) * trackH / range) - 5;
    
    // Draw thumb
    lcd.fillRect(trackX - 3, thumbY, trackW + 6, 10, MAC_WHITE);
    draw3DFrame(lcd, trackX - 3, thumbY, trackW + 6, 10, false);
  } else {
    // Horizontal slider
    int trackY = y + h / 2 - 2;
    int trackH = 4;
    int trackW = w - 20;
    int trackX = x + 10;
    
    // Draw track
    lcd.fillRect(trackX, trackY, trackW, trackH, MAC_LIGHT_GRAY);
    draw3DFrame(lcd, trackX, trackY, trackW, trackH, true);
    
    // Calculate thumb position
    int range = slider.maxValue - slider.minValue;
    int thumbX = trackX + ((slider.currentValue - slider.minValue) * trackW / range) - 5;
    
    // Draw thumb
    lcd.fillRect(thumbX, trackY - 3, 10, trackH + 6, MAC_WHITE);
    draw3DFrame(lcd, thumbX, trackY - 3, 10, trackH + 6, false);
  }
}

void drawProgressBar(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const MacProgressBar& progressBar) {
  // Draw background
  lcd.fillRect(x, y, w, h, MAC_WHITE);
  draw3DFrame(lcd, x, y, w, h, true);
  
  // Calculate fill width
  int range = progressBar.maxValue - progressBar.minValue;
  int fillW = ((progressBar.currentValue - progressBar.minValue) * (w - 4)) / range;
  
  // Draw fill
  if (fillW > 0) {
    lcd.fillRect(x + 2, y + 2, fillW, h - 4, progressBar.fillColor);
  }
  
  // Draw percentage text if enabled
  if (progressBar.showPercentage) {
    int percentage = ((progressBar.currentValue - progressBar.minValue) * 100) / range;
    String percentText = String(percentage) + "%";
    
    lcd.setTextColor(MAC_BLACK, MAC_WHITE);
    lcd.setTextSize(1);
    int textX = x + (w - percentText.length() * 6) / 2;
    int textY = y + (h - 8) / 2;
    lcd.setCursor(textX, textY);
    lcd.print(percentText);
  }
}

// ===== COMPONENT CREATION HELPERS =====

MacComponent* createButtonComponent(int x, int y, int w, int h, int id, const String& text, SymbolType symbol) {
  MacComponent* component = createComponent(COMPONENT_BUTTON, x, y, w, h, id);
  
  // Create button-specific data
  MacButton* buttonData = new MacButton();
  buttonData->text = text;
  buttonData->symbol = symbol;
  buttonData->pressed = false;
  
  component->customData = buttonData;
  return component;
}

MacComponent* createLabelComponent(int x, int y, int w, int h, int id, const String& text, uint16_t textColor) {
  MacComponent* component = createComponent(COMPONENT_LABEL, x, y, w, h, id);
  
  // Create label-specific data
  MacLabel* labelData = new MacLabel();
  labelData->text = text;
  labelData->textColor = textColor;
  labelData->backgroundColor = MAC_WHITE;
  labelData->textSize = 1;
  labelData->centerAlign = false;
  
  component->customData = labelData;
  return component;
}

MacComponent* createTextBoxComponent(int x, int y, int w, int h, int id, const String& placeholder) {
  MacComponent* component = createComponent(COMPONENT_TEXTBOX, x, y, w, h, id);
  
  // Create textbox-specific data
  MacTextBox* textboxData = new MacTextBox();
  textboxData->text = "";
  textboxData->placeholder = placeholder;
  textboxData->focused = false;
  textboxData->cursorPos = 0;
  textboxData->maxLength = 50;
  
  component->customData = textboxData;
  return component;
}

MacComponent* createCheckBoxComponent(int x, int y, int w, int h, int id, const String& label, bool checked) {
  MacComponent* component = createComponent(COMPONENT_CHECKBOX, x, y, w, h, id);
  
  // Create checkbox-specific data
  MacCheckBox* checkboxData = new MacCheckBox();
  checkboxData->label = label;
  checkboxData->checked = checked;
  
  component->customData = checkboxData;
  return component;
}

MacComponent* createSliderComponent(int x, int y, int w, int h, int id, int minVal, int maxVal, int currentVal, bool vertical) {
  MacComponent* component = createComponent(COMPONENT_SLIDER, x, y, w, h, id);
  
  // Create slider-specific data
  MacSlider* sliderData = new MacSlider();
  sliderData->minValue = minVal;
  sliderData->maxValue = maxVal;
  sliderData->currentValue = currentVal;
  sliderData->vertical = vertical;
  
  component->customData = sliderData;
  return component;
}

MacComponent* createProgressBarComponent(int x, int y, int w, int h, int id, int minVal, int maxVal, int currentVal) {
  MacComponent* component = createComponent(COMPONENT_PROGRESS_BAR, x, y, w, h, id);
  
  // Create progress bar-specific data
  MacProgressBar* progressData = new MacProgressBar();
  progressData->minValue = minVal;
  progressData->maxValue = maxVal;
  progressData->currentValue = currentVal;
  progressData->fillColor = MAC_BLUE;
  progressData->showPercentage = true;
  
  component->customData = progressData;
  return component;
}

// ===== GENERIC WINDOW MANAGEMENT HELPERS =====
// These are utility functions that can be called from user-defined callbacks

void handleWindowMinimize(lgfx::LGFX_Device& lcd, MacWindow& window, DesktopIcon* associatedIcon) {
  if (window.minimized) {
    // Window was minimized - show desktop icon if provided
    if (associatedIcon) {
      associatedIcon->visible = true;
      drawDesktopIcon(lcd, associatedIcon->x, associatedIcon->y, associatedIcon->name, false);
    }
    displayStatus(lcd, "Window minimized to desktop", 300);
  } else {
    // Window was restored - hide desktop icon if provided
    if (associatedIcon) {
      associatedIcon->visible = false;
      // Clear the icon area
      redrawDesktopArea(lcd, associatedIcon->x - 2, associatedIcon->y, 36, 50);
    }
    displayStatus(lcd, "Window restored", 300);
    
    // Redraw the window content
    drawWindow(lcd, window);
  }
}

void handleWindowClose(lgfx::LGFX_Device& lcd, MacWindow& window, DesktopIcon* associatedIcon) {
  if (associatedIcon) {
    associatedIcon->visible = false;  // Hide icon when window is closed
  }
  displayStatus(lcd, "Window closed", 300);
}

void handleIconClick(lgfx::LGFX_Device& lcd, MacWindow& window) {
  // Restore window when icon is clicked
  window.minimized = false;
  window.visible = true;
  // Trigger a full interface redraw to show the restored window
  drawWindow(lcd, window);
}

void handleWindowContentClick(lgfx::LGFX_Device& lcd, MacWindow& window, int relativeX, int relativeY) {
  Serial.printf("Window content clicked at relative position: %d, %d\n", relativeX, relativeY);
  
  // Check for components at the clicked position
  MacComponent* clickedComponent = findComponentAt(window, window.x + relativeX, window.y + relativeY);
  
  if (clickedComponent != nullptr) {
    // Handle button press visual feedback
    if (clickedComponent->type == COMPONENT_BUTTON && clickedComponent->customData != nullptr) {
      MacButton* btnData = (MacButton*)clickedComponent->customData;
      
      // Set pressed state and redraw
      btnData->pressed = true;
      drawComponent(lcd, *clickedComponent, window.x, window.y);
      
      // Brief delay for visual feedback
      delay(100);
      
      // Call the callback if it exists
      if (clickedComponent->onClick != nullptr) {
        clickedComponent->onClick(clickedComponent->id);
      }
      
      // Release pressed state and redraw
      btnData->pressed = false;
      drawComponent(lcd, *clickedComponent, window.x, window.y);
    } else if (clickedComponent->onClick != nullptr) {
      // Non-button components just call the callback
      clickedComponent->onClick(clickedComponent->id);
    }
  }
}

void handleWindowMoved(lgfx::LGFX_Device& lcd, MacWindow& window) {
  Serial.println("Window moved - child components automatically positioned relative to window");
  
  // With the component system, all components are positioned relative to the window
  // so no manual position updates are needed when the window moves
  // The drawWindowChildComponents function handles absolute positioning automatically
  
  // Just redraw the window content
  drawWindow(lcd, window);
}

// ===== EXISTING CODE CONTINUES =====
