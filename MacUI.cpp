/*
 * MacUI.cpp - Classic Macintosh OS User Interface Library Implementation
 * 
 * This file contains the implementation of classic Mac OS UI drawing functions
 * using primitive graphics functions with LovyanGFX library.
 */

#include "MacUI.h"

// Screen dimensions are now properly defined in wt32_sc01_plus.h using inline
// This allows the constants to be shared across multiple files without conflicts

/**
 * Draw the classic Mac OS menu bar
 */
void drawMenuBar(lgfx::LGFX_Device& lcd) {
  // Draw menu bar background
  lcd.fillRect(0, 0, screenWidth, 20, MAC_WHITE);
  lcd.drawFastHLine(0, 20, screenWidth, MAC_BLACK);

  lcd.fillCircle(15, 10, 6, MAC_BLACK);
  lcd.fillCircle(18, 7, 3, MAC_WHITE);

  // Draw menu items
  lcd.setTextColor(MAC_BLACK, MAC_WHITE);
  lcd.setTextSize(1);
  lcd.setCursor(30, 6);
  lcd.print("RetroRadio");
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
}

/**
 * Draw a classic Mac OS button with rounded corners and ellipsis
 */
void drawButton(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const String& text, bool pressed) {
  // Choose colors based on pressed state
  uint16_t bgColor = pressed ? MAC_DARK_GRAY : MAC_WHITE;
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

// ===== BUTTON HELPERS IMPLEMENTATION =====
bool isInsideButton(const MacButton& btn, int tx, int ty) {
  return tx >= btn.x && tx < (btn.x + btn.w) && ty >= btn.y && ty < (btn.y + btn.h);
}

void redrawButton(lgfx::LGFX_Device& lcd, MacButton& btn) {
  if (btn.symbol != SYMBOL_NONE) {
    drawSymbolButton(lcd, btn.x, btn.y, btn.w, btn.h, btn.symbol, btn.pressed);
  } else {
    drawButton(lcd, btn.x, btn.y, btn.w, btn.h, btn.text, btn.pressed);
  }
}

/**
 * Transform touch coordinates from portrait to landscape if needed
 */
void transformTouchCoordinates(uint16_t& x, uint16_t& y) {
  uint16_t temp_x = x;

  x = y;
  y = temp_x;

  // Constrain to screen bounds
  x = constrain(x, 0, screenWidth - 1);
  y = constrain(y, 0, screenHeight - 1);
}

/**
 * Interactive button that handles both drawing and touch
 * Call this every loop for each button
 */
void interactiveButton(lgfx::LGFX_Device& lcd, MacButton& btn) {
  uint16_t tx, ty;
  bool touching = lcd.getTouch(&tx, &ty);  // Use lcd parameter for touch

  if (touching) {
    // Apply coordinate transformation if needed
    transformTouchCoordinates(tx, ty);
  }

  bool inside = touching && tx >= btn.x && tx < (btn.x + btn.w) && ty >= btn.y && ty < (btn.y + btn.h);
  bool wasPressed = btn.pressed;

  btn.pressed = inside;

  // Trigger callback on press transition (when button goes from not pressed to pressed)
  if (inside && !wasPressed && btn.onClick) {
    btn.onClick();
  }

  // Only redraw if button state changed to avoid flickering
  if (btn.pressed != wasPressed) {
    drawButton(lcd, btn.x, btn.y, btn.w, btn.h, btn.text, btn.pressed);
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
 * Call this every loop for each window
 */
void interactiveWindow(lgfx::LGFX_Device& lcd, MacWindow& window) {
  if (!window.visible) return;

  static bool wasPressed = false;
  static unsigned long pressTime = 0;

  uint16_t tx, ty;
  bool touching = lcd.getTouch(&tx, &ty);

  if (touching) {
    // transformTouchCoordinates(tx, ty);
    lcd.drawPixel(tx, ty, MAC_BLACK);

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

      // Check if close button was pressed
      if (isInsideCloseButton(window, tx, ty)) {
        delay(100);  // Brief visual feedback
        window.visible = false;

        if (window.onClose) {
          window.onClose();
        }

        // Clear the window area
        redrawDesktopArea(lcd, window.x, window.y, window.w, window.h);

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
      }
    }

    // Reset press state after timeout
    if (millis() - pressTime > 500) {
      wasPressed = false;
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
  // Clear the area first
  lcd.fillRect(x, y, w, h, MAC_WHITE);

  // Redraw checkered pattern in this area
  int patternSize = 8;
  int startX = (x / patternSize) * patternSize;
  int startY = max(21, (y / patternSize) * patternSize);  // Don't overwrite menu bar

  for (int py = startY; py < y + h; py += patternSize) {
    for (int px = startX; px < x + w; px += patternSize) {
      if ((px / patternSize + py / patternSize) % 2 == 0) {
        // Only draw pattern within the specified area
        int rectX = max(px, x);
        int rectY = max(py, y);
        int rectW = min(px + patternSize, x + w) - rectX;
        int rectH = min(py + patternSize, y + h) - rectY;

        if (rectW > 0 && rectH > 0) {
          lcd.fillRect(rectX, rectY, rectW, rectH, MAC_LIGHT_GRAY);
        }
      }
    }
  }
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
 * Interactive desktop icon that handles touch
 */
void interactiveDesktopIcon(lgfx::LGFX_Device& lcd, DesktopIcon& icon) {
  if (!icon.visible) return;

  static bool wasPressed = false;
  static unsigned long pressTime = 0;
  static DesktopIcon* lastIcon = nullptr;

  uint16_t tx, ty;
  bool touching = lcd.getTouch(&tx, &ty);

  if (touching) {
    transformTouchCoordinates(tx, ty);

    bool inside = isInsideDesktopIcon(icon, tx, ty);

    if (inside && !wasPressed && lastIcon != &icon) {
      wasPressed = true;
      pressTime = millis();
      lastIcon = &icon;

      // Visual feedback - invert icon selection
      icon.selected = !icon.selected;
      drawDesktopIcon(lcd, icon.x, icon.y, icon.name, icon.selected);

      delay(150);  // Brief feedback delay

      // Execute click callback
      if (icon.onClick) {
        icon.onClick();
      }

      // Reset selection state
      icon.selected = false;
      drawDesktopIcon(lcd, icon.x, icon.y, icon.name, icon.selected);

      wasPressed = false;
      return;
    }

    // Reset press state after timeout
    if (millis() - pressTime > 500) {
      wasPressed = false;
      lastIcon = nullptr;
    }
  } else {
    wasPressed = false;
    lastIcon = nullptr;
  }
}

/**
 * Draw a symbol (play, pause, stop, etc.) at the specified position
 */
void drawSymbol(lgfx::LGFX_Device& lcd, int x, int y, int size, SymbolType symbol, uint16_t color) {
  switch (symbol) {
    case SYMBOL_PLAY:
      // Draw triangle pointing right
      for (int i = 0; i < size; i++) {
        int lineHeight = (i * 2 * size) / size;
        if (lineHeight > size) lineHeight = 2 * size - lineHeight;
        lcd.drawFastVLine(x + i, y + (size - lineHeight) / 2, lineHeight, color);
      }
      break;

    case SYMBOL_PAUSE:
      // Draw two vertical bars
      {
        int barWidth = size / 4;
        int spacing = size / 3;
        lcd.fillRect(x, y, barWidth, size, color);
        lcd.fillRect(x + barWidth + spacing, y, barWidth, size, color);
      }
      break;

    case SYMBOL_STOP:
      // Draw filled square
      lcd.fillRect(x, y, size, size, color);
      break;

    case SYMBOL_PREV:
      // Draw triangle pointing left with bar
      lcd.fillRect(x, y, 2, size, color);  // Left bar
      for (int i = 0; i < size - 4; i++) {
        int lineHeight = ((size - 4 - i) * 2 * (size - 4)) / (size - 4);
        if (lineHeight > size - 4) lineHeight = 2 * (size - 4) - lineHeight;
        lcd.drawFastVLine(x + 3 + i, y + (size - lineHeight) / 2, lineHeight, color);
      }
      break;

    case SYMBOL_NEXT:
      // Draw triangle pointing right with bar
      for (int i = 0; i < size - 4; i++) {
        int lineHeight = (i * 2 * (size - 4)) / (size - 4);
        if (lineHeight > size - 4) lineHeight = 2 * (size - 4) - lineHeight;
        lcd.drawFastVLine(x + i, y + (size - lineHeight) / 2, lineHeight, color);
      }
      lcd.fillRect(x + size - 2, y, 2, size, color);  // Right bar
      break;

    case SYMBOL_VOL_UP:
      // Draw speaker with sound waves
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
      // Draw speaker only (no waves)
      lcd.fillRect(x, y + size / 3, size / 3, size / 3, color);
      lcd.fillRect(x + size / 3, y + size / 4, size / 6, size / 2, color);
      // Draw minus sign
      lcd.fillRect(x + size / 2 + 2, y + size / 2 - 1, size / 4, 2, color);
      break;

    case SYMBOL_NONE:
    default:
      // Do nothing for text buttons
      break;
  }
}

/**
 * Draw a button with a symbol instead of text
 */
void drawSymbolButton(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, SymbolType symbol, bool pressed) {
  // Colors
  uint16_t bgColor = pressed ? MAC_GRAY : MAC_WHITE;
  uint16_t borderColor = MAC_BLACK;
  uint16_t symbolColor = MAC_BLACK;

  // Fill button area
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
  int symbolSize = min(w, h) - 16;  // Leave some padding
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
