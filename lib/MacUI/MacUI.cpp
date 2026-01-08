/*
 * MacUI.cpp - Classic Macintosh OS User Interface Library Implementation
 *
 * Copyright (c) 2025 felangga
 *
 * This file contains the implementation of classic Mac OS UI drawing functions
 * using primitive graphics functions with LovyanGFX library.
 */

#include "MacUI.h"
// Sprite buffer for double buffering components
lgfx::LGFX_Sprite* componentSprite = nullptr;
lgfx::LGFX_Sprite* windowSprite = nullptr;

// Window manager - track all registered windows
static MacWindow** registeredWindows = nullptr;
static int registeredWindowCount = 0;
static const int MAX_WINDOWS = 10;

/**
 * Initialize sprite buffer for double buffering
 * Call this once during setup after lcd.init()
 */
void initComponentBuffer(lgfx::LGFX_Device* lcd, int maxWidth, int maxHeight) {
  if (componentSprite == nullptr) {
    componentSprite = new lgfx::LGFX_Sprite(lcd);
    componentSprite->setColorDepth(16);
    componentSprite->createSprite(maxWidth, maxHeight);
    componentSprite->fillSprite(MAC_WHITE);  // Initialize with white background
  }

  // Create a larger sprite for entire window (if memory allows)
  if (windowSprite == nullptr) {
    windowSprite = new lgfx::LGFX_Sprite(lcd);
    windowSprite->setColorDepth(16);
    // Try to create sprite for entire window, fall back if memory insufficient
    if (!windowSprite->createSprite(430, 250)) {
      delete windowSprite;
      windowSprite = nullptr;
    } else {
      windowSprite->fillSprite(MAC_WHITE);  // Initialize with white background
    }
  }
}

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
  lcd.setTextColor(MAC_BLACK, MAC_WHITE);
  lcd.fillRect(screenWidth - 60, 0, 80, 20, MAC_WHITE);
  lcd.setCursor(screenWidth - 60, 6);
  lcd.print(time);
}

/**
 * Draw a classic Mac OS window with title bar
 */
void drawWindow(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const String& title,
                bool active) {
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
  if (!window.visible)
    return;

  // Draw full window
  drawWindow(lcd, window.x, window.y, window.w, window.h, window.title, window.active);

  // Skip drawing child components during drag to reduce flicker
  // They'll be redrawn once the drag ends
  if (!window.isDragging) {
    // Draw all child components (flexible system)
    drawWindowChildComponents(lcd, window);
  }
}

// ===== WINDOW INTERACTION HELPERS =====

bool isInsideCloseButton(const MacWindow& window, int tx, int ty) {
  if (!window.visible)
    return false;
  int buttonHeight = 16;  // Now consistent 16 pixels for both normal and minimized
  int buttonY = window.y + 4;
  return tx >= window.x + 4 && tx < window.x + 20 && ty >= buttonY && ty < buttonY + buttonHeight;
}

bool isInsideMinimizeButton(const MacWindow& window, int tx, int ty) {
  if (!window.visible)
    return false;
  int buttonHeight = 18;  // Now consistent 16 pixels for both normal and minimized
  int buttonY = window.y + 4;
  return tx >= window.x + window.w - 24 && tx < window.x + window.w - 6 && ty >= buttonY &&
         ty < buttonY + buttonHeight;
}

bool isInsideTitleBar(const MacWindow& window, int tx, int ty) {
  if (!window.visible)
    return false;
  int titleBarHeight = 24;

  // Check if in title bar but not in close or minimize buttons
  bool inTitleBar = tx >= window.x + 2 && tx < window.x + window.w - 2 && ty >= window.y + 2 &&
                    ty < window.y + 2 + titleBarHeight;
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
  if (!window.visible)
    return;

  static bool wasPressed = false;
  static unsigned long pressTime = 0;
  static int lastTouchX = 0;  // Track last touch coordinates for release callbacks
  static int lastTouchY = 0;

  uint16_t tx, ty;
  bool touching = lcd.getTouch(&tx, &ty);

  if (touching) {
    // Check if touch is inside window bounds
    bool insideWindow =
        tx >= window.x && tx < window.x + window.w && ty >= window.y && ty < window.y + window.h;

    if (insideWindow) {
      // If window is not active, only allow title bar interactions (minimize/close/drag)
      // Block all content interactions for inactive windows
      if (!window.active) {
        if (!wasPressed) {
          wasPressed = true;
          // Only allow minimize button and title bar interactions
          if (isInsideMinimizeButton(window, tx, ty) || isInsideTitleBar(window, tx, ty)) {
            // Let it fall through to normal handling
          } else {
            // Block all other interactions for inactive windows
            return;
          }
        }
      }

      // Always update last touch coordinates while touching
      lastTouchX = tx;
      lastTouchY = ty;

      if (!wasPressed) {
        wasPressed = true;
        pressTime = millis();

        // Check if minimize button was pressed
        if (isInsideMinimizeButton(window, tx, ty)) {
          delay(100);                            // Brief visual feedback
          window.minimized = !window.minimized;  // Toggle minimize state

          if (window.minimized) {
            // Hide the window completely and clear its area (including shadow)
            window.visible = false;
            lcd.startWrite();
            drawCheckeredPatternArea(lcd, window.x, window.y, window.w + 5, window.h + 5);
            lcd.endWrite();
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
          window.isDragging = true;
          window.dragOffsetX = tx - window.x;
          window.dragOffsetY = ty - window.y;
          return;
        }

        // Check if touch is on a ListView component for swipe scrolling (only if window is visible)
        MacComponent* touchedComponent = findComponentAt(window, tx, ty);
        if (window.visible && touchedComponent && touchedComponent->type == COMPONENT_LISTVIEW &&
            touchedComponent->customData) {
          MacListView* listViewData = (MacListView*)touchedComponent->customData;
          listViewData->isTouching = true;
          listViewData->touchStartY = ty;
          listViewData->lastTouchY = ty;
          listViewData->touchStartTime = millis();
        }
      } else if (window.visible) {
        // Handle ongoing swipe for ListView (only if window is visible)
        static unsigned long lastScrollUpdate = 0;
        unsigned long now = millis();

        for (int i = 0; i < window.childComponentCount; i++) {
          MacComponent* component = window.childComponents[i];
          if (component && component->type == COMPONENT_LISTVIEW && component->customData) {
            MacListView* listViewData = (MacListView*)component->customData;
            if (listViewData->isTouching) {
              int deltaY = ty - listViewData->lastTouchY;

              // Update scroll offset (inverted for natural scrolling)
              int oldOffset = listViewData->scrollOffset;
              listViewData->scrollOffset -= deltaY;

              // Constrain scroll offset
              int visibleHeight = component->h - 4;
              int maxScroll =
                  max(0, listViewData->itemCount * listViewData->itemHeight - visibleHeight);
              listViewData->scrollOffset = max(0, min(listViewData->scrollOffset, maxScroll));

              // Throttle redraw to reduce flickering (max 30 FPS)
              if (oldOffset != listViewData->scrollOffset && (now - lastScrollUpdate > 33)) {
                lcd.startWrite();
                drawComponent(lcd, *component, window.x, window.y);
                lcd.endWrite();
                lastScrollUpdate = now;
              }

              listViewData->lastTouchY = ty;
            }
          }
        }
      }

      // Handle ongoing drag
      if (window.isDragging) {
        static unsigned long lastDragUpdate = 0;
        static int lastDrawnX = window.x;
        static int lastDrawnY = window.y;
        unsigned long now = millis();

        // Limit drag update frequency to reduce flicker
        if (now - lastDragUpdate < 50) {  // Slower updates (20 FPS) for smoother appearance
          return;
        }

        int newX = tx - window.dragOffsetX;
        int newY = ty - window.dragOffsetY;

        // Constrain window to screen bounds
        newX = max(0, min(newX, (int)screenWidth - window.w));
        newY = max(21, min(newY, (int)screenHeight - window.h));  // 21 to leave menu bar visible

        // Only update if position actually changed significantly (reduce micro-movements)
        if (abs(newX - lastDrawnX) > 8 || abs(newY - lastDrawnY) > 8) {
          lastDragUpdate = now;

          // Store old position
          int oldX = window.x;
          int oldY = window.y;

          // Update position
          window.x = newX;
          window.y = newY;
          lastDrawnX = newX;
          lastDrawnY = newY;

          // Draw dotted rectangle outline during drag for classic Mac look
          // Use double buffering if sprite is available to eliminate flicker
          if (windowSprite != nullptr) {
            // Render everything to the sprite buffer first
            renderToSprite(&window);

            // Draw bolder dotted rectangle at new position to sprite (adjusted for sprite coords)
            int spriteY = window.y - 21;
            for (int i = 0; i < window.w; i += 4) {
              windowSprite->fillRect(window.x + i, spriteY, 2, 2, MAC_BLACK);
              windowSprite->fillRect(window.x + i, spriteY + window.h - 2, 2, 2, MAC_BLACK);
            }
            for (int i = 0; i < window.h; i += 4) {
              windowSprite->fillRect(window.x, spriteY + i, 2, 2, MAC_BLACK);
              windowSprite->fillRect(window.x + window.w - 2, spriteY + i, 2, 2, MAC_BLACK);
            }

            // Push the entire sprite to screen at once (eliminates flicker)
            lcd.startWrite();
            windowSprite->pushSprite(&lcd, 0, 21);
            lcd.endWrite();
          } else {
            // Fallback: direct rendering (with flicker)
            lcd.startWrite();
            drawCheckeredPatternArea(lcd, oldX, oldY, window.w + 5, window.h + 5);
            redrawAllWindowsExcept(lcd, &window);

            for (int i = 0; i < window.w; i += 4) {
              lcd.fillRect(window.x + i, window.y, 2, 2, MAC_BLACK);
              lcd.fillRect(window.x + i, window.y + window.h - 2, 2, 2, MAC_BLACK);
            }
            for (int i = 0; i < window.h; i += 4) {
              lcd.fillRect(window.x, window.y + i, 2, 2, MAC_BLACK);
              lcd.fillRect(window.x + window.w - 2, window.y + i, 2, 2, MAC_BLACK);
            }
            lcd.endWrite();
          }

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
    if (wasPressed && window.onContentClick && !window.isDragging) {
      window.onContentClick(lastTouchX - window.x, lastTouchY - window.y);
    }

    if (window.isDragging) {
      window.isDragging = false;

      lcd.startWrite();
      // Clear the dotted outline with background pattern
      drawCheckeredPatternArea(lcd, window.x, window.y, window.w + 5, window.h + 5);
      // Redraw all other windows to refresh any obscured content
      redrawAllWindowsExcept(lcd, &window);
      // Draw the dragged window at its final position with all components (on top)
      drawWindow(lcd, window);
      lcd.endWrite();
    }

    // Reset ListView touch state
    for (int i = 0; i < window.childComponentCount; i++) {
      MacComponent* component = window.childComponents[i];
      if (component && component->type == COMPONENT_LISTVIEW && component->customData) {
        MacListView* listViewData = (MacListView*)component->customData;
        listViewData->isTouching = false;
      }
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
 * Draw checkered pattern in a specific area (optimized for window dragging)
 */
void drawCheckeredPatternArea(lgfx::LGFX_Device& lcd, int startX, int startY, int w, int h) {
  int patternSize = 3;

  // Constrain to valid screen area
  startY = max(21, startY);  // Don't draw over menu bar
  int endX = min((int)screenWidth, startX + w);
  int endY = min((int)screenHeight, startY + h);

  // Align to pattern grid for seamless appearance
  int gridStartX = (startX / patternSize) * patternSize;
  int gridStartY = (startY / patternSize) * patternSize;

  for (int y = gridStartY; y < endY; y += patternSize) {
    for (int x = gridStartX; x < endX; x += patternSize) {
      if ((x / patternSize + y / patternSize) % 2 == 0) {
        lcd.fillRect(x, y, patternSize, patternSize, MAC_LIGHT_GRAY);
      } else {
        lcd.fillRect(x, y, patternSize, patternSize, MAC_WHITE);
      }
    }
  }
}

/**
 * Render the entire screen to sprite buffer (for double buffering during drag)
 */
void renderToSprite(MacWindow* draggedWindow) {
  if (windowSprite == nullptr) return;

  int patternSize = 3;

  // Draw checkered pattern to sprite
  for (int y = 0; y < windowSprite->height(); y += patternSize) {
    for (int x = 0; x < windowSprite->width(); x += patternSize) {
      int screenY = y + 21;
      if ((x / patternSize + screenY / patternSize) % 2 == 0) {
        windowSprite->fillRect(x, y, patternSize, patternSize, MAC_LIGHT_GRAY);
      } else {
        windowSprite->fillRect(x, y, patternSize, patternSize, MAC_WHITE);
      }
    }
  }

  // Draw all visible windows except the dragged one
  if (registeredWindows != nullptr) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
      if (registeredWindows[i] != nullptr &&
          registeredWindows[i] != draggedWindow &&
          registeredWindows[i]->visible) {
        MacWindow* win = registeredWindows[i];

        // Adjust coordinates for sprite (sprite starts at y=21 on screen)
        int spriteY = win->y - 21;

        // Draw window shadow
        windowSprite->fillRect(win->x + 3, spriteY + 3, win->w, win->h, MAC_DARK_GRAY);

        // Draw window background
        windowSprite->fillRect(win->x, spriteY, win->w, win->h, MAC_WHITE);

        // Draw window border
        windowSprite->drawRect(win->x, spriteY, win->w, win->h, MAC_BLACK);
        windowSprite->drawRect(win->x + 1, spriteY + 1, win->w - 2, win->h - 2, MAC_BLACK);

        // Draw title bar
        uint16_t titleColor = win->active ? MAC_BLACK : MAC_GRAY;
        windowSprite->fillRect(win->x + 2, spriteY + 2, win->w - 4, 24, titleColor);

        // Draw title text
        windowSprite->setTextColor(MAC_WHITE, titleColor);
        windowSprite->setTextSize(1);
        int titleX = win->x + (win->w - win->title.length() * 6) / 2;
        windowSprite->setCursor(titleX, spriteY + 10);
        windowSprite->print(win->title);

        // Draw minimize button
        windowSprite->fillRect(win->x + win->w - 24, spriteY + 4, 18, 18, MAC_WHITE);
        windowSprite->drawRect(win->x + win->w - 24, spriteY + 4, 18, 18, MAC_BLACK);
        windowSprite->drawFastHLine(win->x + win->w - 18, spriteY + 12, 8, MAC_BLACK);
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



// ===== DESKTOP ICON IMPLEMENTATION =====

/**
 * Redraw desktop area with proper background pattern
 */
void redrawDesktopArea(lgfx::LGFX_Device& lcd) {
  drawCheckeredPattern(lcd);
}

/**
 * Check if touch is inside desktop icon bounds
 */
bool isInsideDesktopIcon(const DesktopIcon& icon, int tx, int ty) {
  if (!icon.visible)
    return false;
  return tx >= icon.x && tx < icon.x + 64 &&  // icon is 64x64 (32x32 + label)
         ty >= icon.y && ty < icon.y + 50;    // 32px icon + 18px label
}

/**
 * Interactive desktop icon that handles touch for icon clicks
 * Call this every loop for each desktop icon
 */
void interactiveDesktopIcon(lgfx::LGFX_Device& lcd, DesktopIcon& icon) {
  if (!icon.visible)
    return;

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
  // Don't find components if window is not visible
  if (!window.visible || window.childComponents == nullptr || window.childComponentCount == 0) {
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

void drawComponent(lgfx::LGFX_Device& lcd, const MacComponent& component, int windowX,
                   int windowY) {
  if (!component.visible)
    return;

  int absoluteX = windowX + component.x;
  int absoluteY = windowY + component.y;

  switch (component.type) {
    case COMPONENT_BUTTON:
      // Draw button using component data
      if (component.customData != nullptr) {
        MacButton* btnData = (MacButton*)component.customData;
        if (btnData->symbol != SYMBOL_NONE) {
          drawSymbolButton(lcd, absoluteX, absoluteY, component.w, component.h, btnData->symbol,
                           btnData->pressed);
        } else {
          drawButton(lcd, absoluteX, absoluteY, component.w, component.h, btnData->text,
                     btnData->pressed);
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

    case COMPONENT_RUNNING_TEXT:
      if (component.customData != nullptr) {
        MacRunningText* runningTextData = (MacRunningText*)component.customData;
        drawRunningText(lcd, absoluteX, absoluteY, component.w, component.h, *runningTextData);
      }
      break;

    case COMPONENT_LISTVIEW:
      if (component.customData != nullptr) {
        MacListView* listViewData = (MacListView*)component.customData;
        drawListView(lcd, absoluteX, absoluteY, component.w, component.h, *listViewData);
      }
      break;

    case COMPONENT_INPUT_FIELD:
      if (component.customData != nullptr) {
        MacInputField* inputFieldData = (MacInputField*)component.customData;
        drawInputField(lcd, absoluteX, absoluteY, component.w, component.h, *inputFieldData);
      }
      break;

    case COMPONENT_KEYBOARD:
      if (component.customData != nullptr) {
        MacKeyboard* keyboardData = (MacKeyboard*)component.customData;
        drawKeyboard(lcd, absoluteX, absoluteY, component.w, component.h, *keyboardData);
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

// ===== GENERIC WINDOW MANAGEMENT HELPERS =====
// These are utility functions that can be called from user-defined callbacks

void handleWindowClose(lgfx::LGFX_Device& lcd, MacWindow& window, DesktopIcon* associatedIcon) {
  if (associatedIcon) {
    associatedIcon->visible = false;  // Hide icon when window is closed
  }
}

void handleWindowMinimize(lgfx::LGFX_Device& lcd, MacWindow& window, DesktopIcon* associatedIcon) {
  window.minimized = true;
  window.visible = false;
  if (associatedIcon) {
    associatedIcon->visible = true;  // Show icon when window is minimized
  }
}

void handleIconClick(lgfx::LGFX_Device& lcd, MacWindow& window) {
  // Restore window when icon is clicked
  window.minimized = false;
  window.visible = true;
  // Redraw all windows to ensure proper layering and refresh
  lcd.startWrite();
  redrawAllWindows(lcd);
  lcd.endWrite();
}

void handleWindowContentClick(lgfx::LGFX_Device& lcd, MacWindow& window, int relativeX,
                              int relativeY) {
  // Ignore clicks if window is not visible
  if (!window.visible)
    return;

  // Check for components at the clicked position
  MacComponent* clickedComponent =
      findComponentAt(window, window.x + relativeX, window.y + relativeY);

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
    } else if (clickedComponent->type == COMPONENT_LISTVIEW &&
               clickedComponent->customData != nullptr) {
      // Handle ListView - this is now just for tap detection, scrolling happens in
      // interactiveWindow
      MacListView* listViewData = (MacListView*)clickedComponent->customData;

      // Check if this was a tap (not a swipe)
      // Convert touchStartY from absolute to relative coordinates for proper comparison
      int relativeTouchStartY = listViewData->touchStartY - window.y;
      int swipeDistance = abs(relativeY - relativeTouchStartY);
      unsigned long swipeDuration = millis() - listViewData->touchStartTime;

      // If movement was small and quick, it's a tap
      if (swipeDistance < 10 && swipeDuration < 300) {
        // Calculate which item was clicked
        int clickedY = relativeY - clickedComponent->y;
        int clickedItemIndex = (clickedY + listViewData->scrollOffset) / listViewData->itemHeight;

        if (clickedItemIndex >= 0 && clickedItemIndex < listViewData->itemCount) {
          // Update selection
          listViewData->selectedIndex = clickedItemIndex;
          drawComponent(lcd, *clickedComponent, window.x, window.y);

          // Call item click callback if set
          if (listViewData->onItemClick != nullptr) {
            delay(100);  // Brief visual feedback
            listViewData->onItemClick(clickedItemIndex, listViewData->items[clickedItemIndex].data);
          }
        }
      }
    } else if (clickedComponent->onClick != nullptr) {
      // Non-button components just call the callback
      clickedComponent->onClick(clickedComponent->id);
    }
  }
}

void handleWindowMoved(lgfx::LGFX_Device& lcd, MacWindow& window) {
  // During dragging, don't constantly redraw - the drawing happens in interactiveWindow
  // This callback is just for notification purposes
  // Components are positioned relative to window, so no updates needed
}

// ===== WINDOW MANAGER IMPLEMENTATION =====

/**
 * Register a window to be tracked by the window manager
 */
void registerWindow(MacWindow* window) {
  if (registeredWindows == nullptr) {
    registeredWindows = new MacWindow*[MAX_WINDOWS];
    for (int i = 0; i < MAX_WINDOWS; i++) {
      registeredWindows[i] = nullptr;
    }
  }

  // Find empty slot
  for (int i = 0; i < MAX_WINDOWS; i++) {
    if (registeredWindows[i] == nullptr) {
      registeredWindows[i] = window;
      registeredWindowCount++;
      return;
    }
  }
}

/**
 * Unregister a window from the window manager
 */
void unregisterWindow(MacWindow* window) {
  if (registeredWindows == nullptr) return;

  for (int i = 0; i < MAX_WINDOWS; i++) {
    if (registeredWindows[i] == window) {
      registeredWindows[i] = nullptr;
      registeredWindowCount--;
      return;
    }
  }
}

/**
 * Redraw all registered windows
 */
void redrawAllWindows(lgfx::LGFX_Device& lcd) {
  if (registeredWindows == nullptr) return;

  for (int i = 0; i < MAX_WINDOWS; i++) {
    if (registeredWindows[i] != nullptr && registeredWindows[i]->visible) {
      drawWindow(lcd, *registeredWindows[i]);
    }
  }
}

/**
 * Redraw all registered windows except the specified one
 */
void redrawAllWindowsExcept(lgfx::LGFX_Device& lcd, MacWindow* exceptWindow) {
  if (registeredWindows == nullptr) return;

  for (int i = 0; i < MAX_WINDOWS; i++) {
    if (registeredWindows[i] != nullptr &&
        registeredWindows[i] != exceptWindow &&
        registeredWindows[i]->visible) {
      drawWindow(lcd, *registeredWindows[i]);
    }
  }
}

/**
 * Show a window on top of all others (makes it visible and redraws all windows)
 */
void showWindowOnTop(lgfx::LGFX_Device& lcd, MacWindow& window) {
  window.visible = true;
  window.minimized = false;

  lcd.startWrite();
  redrawAllWindowsExcept(lcd, &window);
  drawWindow(lcd, window);
  lcd.endWrite();
}
