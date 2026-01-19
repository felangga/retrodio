/*
 * MacKeyboard.cpp - On-screen keyboard component
 * Part of MacUI Library
 */

#include "MacUI.h"

// Keyboard layout - 4 rows
const char* keyboardRows[] = {"1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm"};

const char* keyboardRowsShift[] = {"!@#$%^&*()", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"};

const char* keyboardRowsSymbol[] = {"[]{}#%^*+=", "-_\\|~<>$/", ".,;:!?'\"@", "()&`"};

const int rowCount = 4;
const int radius = 4;

// Keyboard layout constants
const int KEYBOARD_MARGIN = 5;
const int KEY_SPACING = 2;
const int SPECIAL_ROW_HEIGHT = 28;
const int SPECIAL_KEY_WIDTH = 50;  // Sym/ABC button width
const int DONE_KEY_WIDTH = 55;     // Done button width
const int SPECIAL_KEY_SPACING = 5;
const int SHIFT_WIDTH = 45;
const int BACKSPACE_WIDTH = 55;

// Helper function to draw a single key with Mac Classic style
void drawMacKey(lgfx::LGFX_Device& lcd, int keyX, int keyY, int keyWidth, int keyHeight,
                const char* keyChar, bool pressed) {
  if (pressed) {
    // Pressed state: filled black with white text
    lcd.fillRoundRect(keyX, keyY, keyWidth, keyHeight, radius, MAC_BLACK);
    lcd.drawRoundRect(keyX, keyY, keyWidth, keyHeight, radius, MAC_BLACK);
    lcd.setTextColor(MAC_WHITE, MAC_BLACK);
  } else {
    // Normal state: white with black border and black text
    lcd.fillRoundRect(keyX, keyY, keyWidth, keyHeight, radius, MAC_WHITE);
    lcd.drawRoundRect(keyX, keyY, keyWidth, keyHeight, radius, MAC_BLACK);
    lcd.drawRoundRect(keyX + 1, keyY + 1, keyWidth - 2, keyHeight - 2, radius, MAC_BLACK);
    lcd.setTextColor(MAC_BLACK, MAC_WHITE);
  }

  lcd.setTextSize(1);
  lcd.setFont(getFontFromType(FONT_CHICAGO_9PT));
  int textW = lcd.textWidth(keyChar);
  lcd.setCursor(keyX + (keyWidth - textW) / 2, keyY + (keyHeight) / 2);
  lcd.print(keyChar);
}

// Helper function to restore a single pressed key to normal state
void restorePressedKey(lgfx::LGFX_Device& lcd, MacKeyboard* keyboard, int x, int y, int w, int h) {
  if (!keyboard->isKeyPressed)
    return;

  int rowHeight = (h - (2 * KEYBOARD_MARGIN) - SPECIAL_ROW_HEIGHT - KEY_SPACING) / rowCount;

  // Handle backspace key
  if (keyboard->isBackspace) {
    int row3Y = y + KEYBOARD_MARGIN + 3 * rowHeight;

    bool inSymbolMode = (keyboard->selectedKey == -2);
    const char* row3Keys = inSymbolMode
                               ? keyboardRowsSymbol[3]
                               : (keyboard->shiftActive ? keyboardRowsShift[3] : keyboardRows[3]);
    int row3KeyCount = strlen(row3Keys);

    int availableWidth =
        inSymbolMode
            ? (w - (2 * KEYBOARD_MARGIN) - BACKSPACE_WIDTH - KEY_SPACING)
            : (w - (2 * KEYBOARD_MARGIN) - SHIFT_WIDTH - BACKSPACE_WIDTH - (2 * KEY_SPACING));
    int row3StartX = inSymbolMode ? KEYBOARD_MARGIN : (KEYBOARD_MARGIN + SHIFT_WIDTH + KEY_SPACING);
    int row3KeyWidth = (availableWidth - (row3KeyCount - 1) * KEY_SPACING) / row3KeyCount;

    int backspaceX = x + row3StartX + row3KeyCount * (row3KeyWidth + KEY_SPACING);

    lcd.startWrite();
    lcd.fillRoundRect(backspaceX, row3Y, BACKSPACE_WIDTH, rowHeight - KEY_SPACING, radius,
                      MAC_WHITE);
    lcd.drawRoundRect(backspaceX, row3Y, BACKSPACE_WIDTH, rowHeight - KEY_SPACING, radius,
                      MAC_BLACK);
    lcd.drawRoundRect(backspaceX + 1, row3Y + 1, BACKSPACE_WIDTH - 2, rowHeight - KEY_SPACING - 2,
                      radius, MAC_BLACK);
    lcd.setTextColor(MAC_BLACK, MAC_WHITE);
    lcd.setTextSize(1);
    lcd.setFont(getFontFromType(FONT_CHICAGO_9PT));
    int deleteTextW = lcd.textWidth("Del");
    lcd.setCursor(backspaceX + (BACKSPACE_WIDTH - deleteTextW) / 2,
                  row3Y + (rowHeight - KEY_SPACING) / 2);
    lcd.print("Del");
    lcd.endWrite();
    return;
  }

  // Handle space key
  if (keyboard->isSpace) {
    int specialRowY = y + h - KEYBOARD_MARGIN - SPECIAL_ROW_HEIGHT;
    int spaceStartX = KEYBOARD_MARGIN + SPECIAL_KEY_WIDTH + SPECIAL_KEY_SPACING;
    int spaceW =
        w - (2 * KEYBOARD_MARGIN) - SPECIAL_KEY_WIDTH - DONE_KEY_WIDTH - (2 * SPECIAL_KEY_SPACING);
    int spaceX = x + spaceStartX;

    lcd.startWrite();
    lcd.fillRoundRect(spaceX, specialRowY, spaceW, SPECIAL_ROW_HEIGHT, radius, MAC_WHITE);
    lcd.drawRoundRect(spaceX, specialRowY, spaceW, SPECIAL_ROW_HEIGHT, radius, MAC_BLACK);
    lcd.drawRoundRect(spaceX + 1, specialRowY + 1, spaceW - 2, SPECIAL_ROW_HEIGHT - 2, radius - 1,
                      MAC_BLACK);
    lcd.setTextColor(MAC_BLACK, MAC_WHITE);
    lcd.setTextSize(1);
    lcd.setFont(getFontFromType(FONT_CHICAGO_9PT));
    int spaceTextW = lcd.textWidth("Space");
    lcd.setCursor(spaceX + (spaceW - spaceTextW) / 2, specialRowY + SPECIAL_ROW_HEIGHT / 2);
    lcd.print("Space");
    lcd.endWrite();
    return;
  }

  // Handle regular character keys
  if (keyboard->lastPressedChar != '\0' && keyboard->pressedRow >= 0) {
    int row = keyboard->pressedRow;
    int keyIndex = keyboard->pressedKeyIndex;

    // Determine which character to show (considering shift state change)
    const char* keys;
    if (row == 3) {
      bool inSymbolMode = (keyboard->selectedKey == -2);
      keys = inSymbolMode ? keyboardRowsSymbol[3]
                          : (keyboard->shiftActive ? keyboardRowsShift[3] : keyboardRows[3]);

      int row3Y = y + KEYBOARD_MARGIN + 3 * rowHeight;
      int row3KeyCount = strlen(keys);
      int availableWidth =
          inSymbolMode
              ? (w - (2 * KEYBOARD_MARGIN) - BACKSPACE_WIDTH - KEY_SPACING)
              : (w - (2 * KEYBOARD_MARGIN) - SHIFT_WIDTH - BACKSPACE_WIDTH - (2 * KEY_SPACING));
      int row3StartX =
          inSymbolMode ? KEYBOARD_MARGIN : (KEYBOARD_MARGIN + SHIFT_WIDTH + KEY_SPACING);
      int row3KeyWidth = (availableWidth - (row3KeyCount - 1) * KEY_SPACING) / row3KeyCount;
      int keyX = x + row3StartX + keyIndex * (row3KeyWidth + KEY_SPACING);

      char keyChar[2] = {keys[keyIndex], '\0'};
      lcd.startWrite();
      drawMacKey(lcd, keyX, row3Y, row3KeyWidth, rowHeight - KEY_SPACING, keyChar, false);
      lcd.endWrite();
    } else {
      // Rows 0-2
      keys = (keyboard->selectedKey == -2)
                 ? keyboardRowsSymbol[row]
                 : (keyboard->shiftActive ? keyboardRowsShift[row] : keyboardRows[row]);

      int keyCount = strlen(keys);
      int rowWidth = w - (2 * KEYBOARD_MARGIN);
      int keyWidth = (rowWidth - (keyCount - 1) * KEY_SPACING) / keyCount;
      int startX = x + KEYBOARD_MARGIN;
      int rowY = y + KEYBOARD_MARGIN + row * rowHeight;
      int keyX = startX + keyIndex * (keyWidth + KEY_SPACING);

      char keyChar[2] = {keys[keyIndex], '\0'};
      lcd.startWrite();
      drawMacKey(lcd, keyX, rowY, keyWidth, rowHeight - KEY_SPACING, keyChar, false);
      lcd.endWrite();
    }
  }
}

void drawKeyboard(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, MacKeyboard& keyboard) {
  if (!keyboard.visible)
    return;

  // Draw keyboard background
  lcd.fillRect(x, y, w, h, MAC_LIGHT_GRAY);
  lcd.drawRect(x, y, w, h, MAC_BLACK);
  lcd.setFont(getFontFromType(FONT_CHICAGO_9PT));

  // Uniform margin on all sides
  int rowHeight = (h - (2 * KEYBOARD_MARGIN) - SPECIAL_ROW_HEIGHT - KEY_SPACING) /
                  rowCount;  // Equal top/bottom margin, reserve space for special row

  // Draw rows 0-2 (normal letter rows)
  for (int row = 0; row < 3; row++) {
    // Select the appropriate keyboard layout
    const char* keys;
    if (keyboard.selectedKey == -2) {
      // Symbol mode
      keys = keyboardRowsSymbol[row];
    } else if (keyboard.shiftActive) {
      // Shift mode (uppercase/special)
      keys = keyboardRowsShift[row];
    } else {
      // Normal mode
      keys = keyboardRows[row];
    }

    int keyCount = strlen(keys);
    int rowWidth = w - (2 * KEYBOARD_MARGIN);
    int keyWidth = (rowWidth - (keyCount - 1) * KEY_SPACING) / keyCount;

    int rowY = y + KEYBOARD_MARGIN + row * rowHeight;
    int startX = x + KEYBOARD_MARGIN;

    // Draw each key in the row
    for (int i = 0; i < keyCount; i++) {
      int keyX = startX + i * (keyWidth + KEY_SPACING);
      char keyChar[2] = {keys[i], '\0'};

      // Draw key with Mac Classic style
      drawMacKey(lcd, keyX, rowY, keyWidth, rowHeight - KEY_SPACING, keyChar, false);
    }
  }

  // Row 3 (zxcvbnm) with Shift on left and Backspace on right
  int row3Y = y + KEYBOARD_MARGIN + 3 * rowHeight;

  // Select the appropriate keyboard layout for row 3
  const char* row3Keys;
  if (keyboard.selectedKey == -2) {
    row3Keys = keyboardRowsSymbol[3];
  } else if (keyboard.shiftActive) {
    row3Keys = keyboardRowsShift[3];
  } else {
    row3Keys = keyboardRows[3];
  }

  int row3KeyCount = strlen(row3Keys);

  // Calculate available width based on whether we're in symbol mode
  bool inSymbolMode = (keyboard.selectedKey == -2);
  int availableWidth;
  int row3StartX;

  if (inSymbolMode) {
    // In symbol mode, no shift button - use full width
    availableWidth = w - (2 * KEYBOARD_MARGIN) - BACKSPACE_WIDTH - KEY_SPACING;
    row3StartX = x + KEYBOARD_MARGIN;
  } else {
    // Normal mode, include shift button
    availableWidth = w - (2 * KEYBOARD_MARGIN) - SHIFT_WIDTH - BACKSPACE_WIDTH - (2 * KEY_SPACING);
    row3StartX = x + KEYBOARD_MARGIN + SHIFT_WIDTH + KEY_SPACING;
  }

  int row3KeyWidth = (availableWidth - (row3KeyCount - 1) * KEY_SPACING) / row3KeyCount;

  // Draw Shift button (left side of row 3) - only if NOT in symbol mode
  if (!inSymbolMode) {
    int shiftX = x + KEYBOARD_MARGIN;
    if (keyboard.shiftActive) {
      lcd.fillRoundRect(shiftX, row3Y, SHIFT_WIDTH, rowHeight - KEY_SPACING, radius, MAC_BLACK);
      lcd.drawRoundRect(shiftX, row3Y, SHIFT_WIDTH, rowHeight - KEY_SPACING, radius, MAC_BLACK);
      lcd.setTextColor(MAC_WHITE, MAC_BLACK);
    } else {
      lcd.fillRoundRect(shiftX, row3Y, SHIFT_WIDTH, rowHeight - KEY_SPACING, radius, MAC_WHITE);
      lcd.drawRoundRect(shiftX, row3Y, SHIFT_WIDTH, rowHeight - KEY_SPACING, radius, MAC_BLACK);
      lcd.drawRoundRect(shiftX + 1, row3Y + 1, SHIFT_WIDTH - 2, rowHeight - KEY_SPACING - 2, 2,
                        MAC_BLACK);
      lcd.setTextColor(MAC_BLACK, MAC_WHITE);
    }
    lcd.setTextSize(1);

    // Center-align text - show different label when shift is locked
    const char* shiftLabel = keyboard.shiftLocked ? "CAPS" : "Shift";
    int shiftTextW = lcd.textWidth(shiftLabel);
    lcd.setCursor(shiftX + (SHIFT_WIDTH - shiftTextW) / 2, row3Y + (rowHeight - KEY_SPACING) / 2);
    lcd.print(shiftLabel);

    // Draw underline indicator when shift is locked
    if (keyboard.shiftLocked) {
      int underlineY = row3Y + rowHeight - KEY_SPACING - 4;
      lcd.drawFastHLine(shiftX + 8, underlineY, SHIFT_WIDTH - 16, MAC_WHITE);
    }
  }

  // Draw letter keys (zxcvbnm)
  for (int i = 0; i < row3KeyCount; i++) {
    int keyX = row3StartX + i * (row3KeyWidth + KEY_SPACING);
    char keyChar[2] = {row3Keys[i], '\0'};
    drawMacKey(lcd, keyX, row3Y, row3KeyWidth, rowHeight - KEY_SPACING, keyChar, false);
  }

  // Draw Backspace button (right side of row 3)
  int backspaceX = row3StartX + row3KeyCount * (row3KeyWidth + KEY_SPACING);
  lcd.fillRoundRect(backspaceX, row3Y, BACKSPACE_WIDTH, rowHeight - KEY_SPACING, radius, MAC_WHITE);
  lcd.drawRoundRect(backspaceX, row3Y, BACKSPACE_WIDTH, rowHeight - KEY_SPACING, radius, MAC_BLACK);
  lcd.drawRoundRect(backspaceX + 1, row3Y + 1, BACKSPACE_WIDTH - 2, rowHeight - KEY_SPACING - 2,
                    radius, MAC_BLACK);
  lcd.setTextColor(MAC_BLACK, MAC_WHITE);
  lcd.setTextSize(1);

  int deleteTextW = lcd.textWidth("Del");
  lcd.setCursor(backspaceX + (BACKSPACE_WIDTH - deleteTextW) / 2,
                row3Y + (rowHeight - KEY_SPACING) / 2);
  lcd.print("Del");

  // Draw special keys row at bottom (Sym, Space, Done)
  int specialRowY = y + h - KEYBOARD_MARGIN - SPECIAL_ROW_HEIGHT;

  // Sym/ABC toggle key
  int symX = x + KEYBOARD_MARGIN;
  if (keyboard.selectedKey == -2) {
    lcd.fillRoundRect(symX, specialRowY, SPECIAL_KEY_WIDTH, SPECIAL_ROW_HEIGHT, radius, MAC_BLACK);
    lcd.drawRoundRect(symX, specialRowY, SPECIAL_KEY_WIDTH, SPECIAL_ROW_HEIGHT, radius, MAC_BLACK);
    lcd.setTextColor(MAC_WHITE, MAC_BLACK);
  } else {
    lcd.fillRoundRect(symX, specialRowY, SPECIAL_KEY_WIDTH, SPECIAL_ROW_HEIGHT, radius, MAC_WHITE);
    lcd.drawRoundRect(symX, specialRowY, SPECIAL_KEY_WIDTH, SPECIAL_ROW_HEIGHT, radius, MAC_BLACK);
    lcd.drawRoundRect(symX + 1, specialRowY + 1, SPECIAL_KEY_WIDTH - 2, SPECIAL_ROW_HEIGHT - 2,
                      radius - 1, MAC_BLACK);
    lcd.setTextColor(MAC_BLACK, MAC_WHITE);
  }
  lcd.setTextSize(1);
  // Center-align Sym/ABC text
  const char* symText = keyboard.selectedKey == -2 ? "ABC" : "Sym";
  int symTextW = lcd.textWidth(symText);
  lcd.setCursor(symX + (SPECIAL_KEY_WIDTH - symTextW) / 2, specialRowY + (SPECIAL_ROW_HEIGHT) / 2);
  lcd.print(symText);

  // Space bar (middle, takes remaining space between Sym and Done)
  int spaceX = symX + SPECIAL_KEY_WIDTH + SPECIAL_KEY_SPACING;
  int spaceW =
      w - (2 * KEYBOARD_MARGIN) - SPECIAL_KEY_WIDTH - DONE_KEY_WIDTH - (2 * SPECIAL_KEY_SPACING);
  lcd.fillRoundRect(spaceX, specialRowY, spaceW, SPECIAL_ROW_HEIGHT, radius, MAC_WHITE);
  lcd.drawRoundRect(spaceX, specialRowY, spaceW, SPECIAL_ROW_HEIGHT, radius, MAC_BLACK);
  lcd.drawRoundRect(spaceX + 1, specialRowY + 1, spaceW - 2, SPECIAL_ROW_HEIGHT - 2, radius - 1,
                    MAC_BLACK);
  lcd.setTextColor(MAC_BLACK, MAC_WHITE);
  lcd.setTextSize(1);
  // Center-align "Space" text
  int spaceTextW = lcd.textWidth("Space");
  lcd.setCursor(spaceX + (spaceW - spaceTextW) / 2, specialRowY + (SPECIAL_ROW_HEIGHT) / 2);
  lcd.print("Space");

  // Done button
  int doneX = spaceX + spaceW + SPECIAL_KEY_SPACING;
  lcd.fillRoundRect(doneX, specialRowY, DONE_KEY_WIDTH, SPECIAL_ROW_HEIGHT, radius, MAC_WHITE);
  lcd.drawRoundRect(doneX, specialRowY, DONE_KEY_WIDTH, SPECIAL_ROW_HEIGHT, radius, MAC_BLACK);
  lcd.drawRoundRect(doneX + 1, specialRowY + 1, DONE_KEY_WIDTH - 2, SPECIAL_ROW_HEIGHT - 2,
                    radius - 1, MAC_BLACK);
  lcd.setTextColor(MAC_BLACK, MAC_WHITE);
  lcd.setTextSize(1);

  // Center-align "Done" text
  int doneTextW = lcd.textWidth("Done");
  lcd.setCursor(doneX + (DONE_KEY_WIDTH - doneTextW) / 2, specialRowY + (SPECIAL_ROW_HEIGHT) / 2);
  lcd.print("Done");
}

MacComponent* createKeyboardComponent(int x, int y, int w, int h, int id, int targetInputId) {
  MacComponent* component = createComponent(COMPONENT_KEYBOARD, x, y, w, h, id);

  MacKeyboard* keyboard = new MacKeyboard();
  keyboard->visible = false;
  keyboard->x = x;
  keyboard->y = y;
  keyboard->w = w;
  keyboard->h = h;
  keyboard->targetInputId = targetInputId;
  keyboard->shiftActive = false;
  keyboard->shiftLocked = false;
  keyboard->lastShiftPressTime = 0;
  keyboard->selectedKey = -1;
  // Initialize key repeat tracking
  keyboard->isKeyPressed = false;
  keyboard->keyPressStart = 0;
  keyboard->lastRepeat = 0;
  keyboard->pressedRow = -1;
  keyboard->pressedKeyIndex = -1;
  keyboard->lastPressedChar = '\0';
  keyboard->isBackspace = false;
  keyboard->isSpace = false;

  component->customData = keyboard;
  return component;
}

// Helper function to handle keyboard touch and update input field
bool handleKeyboardTouch(lgfx::LGFX_Device& lcd, MacComponent* keyboardComponent,
                         MacComponent* inputComponent, int touchX, int touchY, MacWindow* window) {
  if (!keyboardComponent || keyboardComponent->type != COMPONENT_KEYBOARD)
    return false;
  if (!inputComponent || inputComponent->type != COMPONENT_INPUT_FIELD)
    return false;

  MacKeyboard* keyboard = (MacKeyboard*)keyboardComponent->customData;
  MacInputField* inputField = (MacInputField*)inputComponent->customData;

  if (!keyboard->visible)
    return false;

  int x = keyboardComponent->x;
  int y = keyboardComponent->y;
  int w = keyboardComponent->w;
  int h = keyboardComponent->h;

  // Check if touch is within keyboard area
  if (touchX < x || touchX > x + w || touchY < y || touchY > y + h)
    return false;

  // Calculate relative touch position
  int relX = touchX - x;
  int relY = touchY - y;

  // Calculate key dimensions (must match drawKeyboard)
  int rowHeight = (h - (2 * KEYBOARD_MARGIN) - SPECIAL_ROW_HEIGHT - KEY_SPACING) / rowCount;

  // Check special keys (bottom row) - Sym, Space, and Done
  int specialRowY = h - KEYBOARD_MARGIN - SPECIAL_ROW_HEIGHT;

  if (relY >= specialRowY) {
    // Sym/ABC toggle key (leftmost)
    if (relX >= KEYBOARD_MARGIN && relX <= KEYBOARD_MARGIN + SPECIAL_KEY_WIDTH) {
      if (keyboard->selectedKey == -2) {
        // Currently in symbol mode, switch back to normal
        keyboard->selectedKey = -1;
      } else {
        // Switch to symbol mode
        keyboard->selectedKey = -2;
      }
      lcd.startWrite();
      drawKeyboard(lcd, x, y, w, h, *keyboard);
      lcd.endWrite();
      delay(200);  // Add delay to prevent accidental double-taps
      return true;
    }

    // Calculate space bar and done button positions (must match drawKeyboard)
    int spaceStartX = KEYBOARD_MARGIN + SPECIAL_KEY_WIDTH + SPECIAL_KEY_SPACING;
    int spaceW =
        w - (2 * KEYBOARD_MARGIN) - SPECIAL_KEY_WIDTH - DONE_KEY_WIDTH - (2 * SPECIAL_KEY_SPACING);
    int spaceEndX = spaceStartX + spaceW;
    int doneStartX = spaceEndX + SPECIAL_KEY_SPACING;
    int doneEndX = doneStartX + DONE_KEY_WIDTH;

    // Space bar (middle)
    if (relX >= spaceStartX && relX <= spaceEndX) {
      if (inputField->text.length() < inputField->maxLength) {
        // Visual feedback for space bar
        int spaceX = x + spaceStartX;
        int btnY = y + h - KEYBOARD_MARGIN - SPECIAL_ROW_HEIGHT;
        lcd.startWrite();
        lcd.fillRoundRect(spaceX, btnY, spaceW, SPECIAL_ROW_HEIGHT, radius, MAC_BLACK);
        lcd.drawRoundRect(spaceX, btnY, spaceW, SPECIAL_ROW_HEIGHT, radius, MAC_BLACK);
        lcd.setTextColor(MAC_WHITE, MAC_BLACK);
        lcd.setTextSize(1);
        lcd.setFont(getFontFromType(FONT_CHICAGO_9PT));
        int spaceTextW = lcd.textWidth("Space");
        lcd.setCursor(spaceX + (spaceW - spaceTextW) / 2, btnY + SPECIAL_ROW_HEIGHT / 2);
        lcd.print("Space");
        lcd.endWrite();

        // Track key press for repeat
        if (!keyboard->isKeyPressed) {
          keyboard->isKeyPressed = true;
          keyboard->keyPressStart = millis();
          keyboard->lastRepeat = millis();
          keyboard->isSpace = true;
          keyboard->isBackspace = false;
          keyboard->lastPressedChar = '\0';

          // Add space immediately on first press
          inputField->text = inputField->text.substring(0, inputField->cursorPos) + " " +
                             inputField->text.substring(inputField->cursorPos);
          inputField->cursorPos++;
        }

        return true;
      }
    }

    // Done button
    if (relX >= doneStartX && relX <= doneEndX) {
      // Visual feedback for done button
      int doneX = x + doneStartX;
      int btnY = y + h - KEYBOARD_MARGIN - SPECIAL_ROW_HEIGHT;
      lcd.startWrite();
      lcd.fillRoundRect(doneX, btnY, DONE_KEY_WIDTH, SPECIAL_ROW_HEIGHT, radius, MAC_BLACK);
      lcd.drawRoundRect(doneX, btnY, DONE_KEY_WIDTH, SPECIAL_ROW_HEIGHT, radius, MAC_BLACK);
      lcd.setTextColor(MAC_WHITE, MAC_BLACK);
      lcd.setTextSize(1);
      lcd.setFont(getFontFromType(FONT_CHICAGO_9PT));
      int doneTextW = lcd.textWidth("Done");
      lcd.setCursor(doneX + (DONE_KEY_WIDTH - doneTextW) / 2, btnY + SPECIAL_ROW_HEIGHT / 2);
      lcd.print("Done");
      lcd.endWrite();
      delay(100);

      // Hide the keyboard
      keyboard->visible = false;
      inputField->focused = false;

      // Clear the keyboard area and redraw the window
      lcd.startWrite();
      drawCheckeredPatternArea(lcd, x, y, w, h);
      if (window != nullptr) {
        drawWindow(lcd, *window);
      }
      drawBottomBar(lcd);
      lcd.endWrite();

      return true;
    }

    return false;
  }

  // Check regular keys
  int row = (relY - KEYBOARD_MARGIN) / rowHeight;
  if (row >= 0 && row < rowCount) {
    // Handle row 3 specially (with Shift and Backspace)
    if (row == 3) {
      int row3Y = y + KEYBOARD_MARGIN + 3 * rowHeight;
      int row3RelY = relY - (KEYBOARD_MARGIN + 3 * rowHeight);

      bool inSymbolMode = (keyboard->selectedKey == -2);

      // Check Shift button (leftmost in row 3) - only if NOT in symbol mode
      if (!inSymbolMode && relX >= KEYBOARD_MARGIN && relX <= KEYBOARD_MARGIN + SHIFT_WIDTH &&
          row3RelY >= 0 && row3RelY <= rowHeight - KEY_SPACING) {
        unsigned long currentTime = millis();
        const unsigned long DOUBLE_TAP_THRESHOLD = 400;  // 400ms window for double-tap

        if (keyboard->shiftLocked) {
          // If shift is locked, single tap unlocks it
          keyboard->shiftLocked = false;
          keyboard->shiftActive = false;
        } else if (keyboard->shiftActive &&
                   (currentTime - keyboard->lastShiftPressTime) < DOUBLE_TAP_THRESHOLD) {
          // Double-tap detected while shift is active - lock it
          keyboard->shiftLocked = true;
          keyboard->shiftActive = true;
        } else {
          // Single tap - toggle shift
          keyboard->shiftActive = !keyboard->shiftActive;
        }

        keyboard->lastShiftPressTime = currentTime;
        lcd.startWrite();
        drawKeyboard(lcd, x, y, w, h, *keyboard);
        lcd.endWrite();
        delay(150);  // Reduced delay to allow double-tap detection
        return true;
      }

      // Select the appropriate keyboard layout for row 3
      const char* row3Keys;
      if (keyboard->selectedKey == -2) {
        row3Keys = keyboardRowsSymbol[3];
      } else if (keyboard->shiftActive) {
        row3Keys = keyboardRowsShift[3];
      } else {
        row3Keys = keyboardRows[3];
      }

      int row3KeyCount = strlen(row3Keys);

      // Calculate available width and start position based on symbol mode (must match drawKeyboard)
      int availableWidth;
      int row3StartX;

      if (inSymbolMode) {
        // In symbol mode, no shift button - use full width
        availableWidth = w - (2 * KEYBOARD_MARGIN) - BACKSPACE_WIDTH - KEY_SPACING;
        row3StartX = KEYBOARD_MARGIN;
      } else {
        // Normal mode, include shift button
        availableWidth =
            w - (2 * KEYBOARD_MARGIN) - SHIFT_WIDTH - BACKSPACE_WIDTH - (2 * KEY_SPACING);
        row3StartX = KEYBOARD_MARGIN + SHIFT_WIDTH + KEY_SPACING;
      }

      int row3KeyWidth = (availableWidth - (row3KeyCount - 1) * KEY_SPACING) / row3KeyCount;

      // Check letter keys (zxcvbnm)
      if (relX >= row3StartX) {
        int relXFromRow3Start = relX - row3StartX;
        int keyIndex = relXFromRow3Start / (row3KeyWidth + KEY_SPACING);

        if (keyIndex >= 0 && keyIndex < row3KeyCount) {
          int keyX = x + row3StartX + keyIndex * (row3KeyWidth + KEY_SPACING);
          int keyEndX = keyX + row3KeyWidth;

          // Make sure touch is within this key bounds
          if (touchX >= keyX && touchX <= keyEndX) {
            if (inputField->text.length() < inputField->maxLength) {
              char newChar = row3Keys[keyIndex];
              char keyChar[2] = {newChar, '\0'};

              // Show pressed state
              lcd.startWrite();
              drawMacKey(lcd, keyX, row3Y, row3KeyWidth, rowHeight - KEY_SPACING, keyChar, true);
              lcd.endWrite();

              // Track key press for repeat
              if (!keyboard->isKeyPressed) {
                keyboard->isKeyPressed = true;
                keyboard->keyPressStart = millis();
                keyboard->lastRepeat = millis();
                keyboard->isBackspace = false;
                keyboard->isSpace = false;
                keyboard->lastPressedChar = newChar;
                keyboard->pressedRow = 3;
                keyboard->pressedKeyIndex = keyIndex;

                // Add character immediately on first press
                inputField->text = inputField->text.substring(0, inputField->cursorPos) +
                                   String(newChar) +
                                   inputField->text.substring(inputField->cursorPos);
                inputField->cursorPos++;
              }

              return true;
            }
          }
        }
      }

      // Check Backspace button (rightmost in row 3)
      int backspaceRelX = row3StartX + row3KeyCount * (row3KeyWidth + KEY_SPACING);
      if (relX >= backspaceRelX && relX <= backspaceRelX + BACKSPACE_WIDTH && row3RelY >= 0 &&
          row3RelY <= rowHeight - KEY_SPACING) {
        if (inputField->cursorPos > 0) {
          // Visual feedback for delete/backspace
          int backspaceX = x + backspaceRelX;
          lcd.startWrite();
          lcd.fillRoundRect(backspaceX, row3Y, BACKSPACE_WIDTH, rowHeight - KEY_SPACING, radius,
                            MAC_BLACK);
          lcd.drawRoundRect(backspaceX, row3Y, BACKSPACE_WIDTH, rowHeight - KEY_SPACING, radius,
                            MAC_BLACK);
          lcd.setTextColor(MAC_WHITE, MAC_BLACK);
          lcd.setTextSize(1);
          lcd.setFont(getFontFromType(FONT_CHICAGO_9PT));
          int deleteTextW = lcd.textWidth("Del");
          lcd.setCursor(backspaceX + (BACKSPACE_WIDTH - deleteTextW) / 2,
                        row3Y + (rowHeight - KEY_SPACING) / 2);
          lcd.print("Del");
          lcd.endWrite();

          // Track key press for repeat
          if (!keyboard->isKeyPressed) {
            keyboard->isKeyPressed = true;
            keyboard->keyPressStart = millis();
            keyboard->lastRepeat = millis();
            keyboard->isBackspace = true;
            keyboard->isSpace = false;
            keyboard->lastPressedChar = '\0';

            // Delete character immediately on first press
            inputField->text = inputField->text.substring(0, inputField->cursorPos - 1) +
                               inputField->text.substring(inputField->cursorPos);
            inputField->cursorPos--;
          }

          return true;
        }
      }

      return false;
    }

    // Handle rows 0-2 (normal rows)
    // Select the appropriate keyboard layout
    const char* keys;
    if (keyboard->selectedKey == -2) {
      // Symbol mode
      keys = keyboardRowsSymbol[row];
    } else if (keyboard->shiftActive) {
      // Shift mode (uppercase/special)
      keys = keyboardRowsShift[row];
    } else {
      // Normal mode
      keys = keyboardRows[row];
    }

    int keyCount = strlen(keys);
    int rowWidth = w - (2 * KEYBOARD_MARGIN);
    int keyWidth = (rowWidth - (keyCount - 1) * KEY_SPACING) / keyCount;

    int keyIndex = (relX - KEYBOARD_MARGIN) / (keyWidth + KEY_SPACING);
    if (keyIndex >= 0 && keyIndex < keyCount) {
      // Add character to input field
      if (inputField->text.length() < inputField->maxLength) {
        char newChar = keys[keyIndex];

        // Calculate key position for visual feedback
        int startX = x + KEYBOARD_MARGIN;
        int rowY = y + KEYBOARD_MARGIN + row * rowHeight;
        int keyX = startX + keyIndex * (keyWidth + KEY_SPACING);
        char keyChar[2] = {newChar, '\0'};

        // Show pressed state
        lcd.startWrite();
        drawMacKey(lcd, keyX, rowY, keyWidth, rowHeight - KEY_SPACING, keyChar, true);
        lcd.endWrite();

        // Track key press for repeat
        if (!keyboard->isKeyPressed) {
          keyboard->isKeyPressed = true;
          keyboard->keyPressStart = millis();
          keyboard->lastRepeat = millis();
          keyboard->isBackspace = false;
          keyboard->isSpace = false;
          keyboard->lastPressedChar = newChar;
          keyboard->pressedRow = row;
          keyboard->pressedKeyIndex = keyIndex;

          // Add character immediately on first press
          inputField->text = inputField->text.substring(0, inputField->cursorPos) +
                             String(newChar) + inputField->text.substring(inputField->cursorPos);
          inputField->cursorPos++;
        }

        return true;
      }
    }
  }

  return false;
}

// Helper function to handle key repeat when keys are held down
void handleKeyboardRepeat(lgfx::LGFX_Device& lcd, MacComponent* keyboardComponent,
                          MacComponent* inputComponent, MacWindow* window) {
  if (!keyboardComponent || keyboardComponent->type != COMPONENT_KEYBOARD)
    return;
  if (!inputComponent || inputComponent->type != COMPONENT_INPUT_FIELD)
    return;

  MacKeyboard* keyboard = (MacKeyboard*)keyboardComponent->customData;
  MacInputField* inputField = (MacInputField*)inputComponent->customData;

  if (!keyboard->visible || !keyboard->isKeyPressed)
    return;

  unsigned long currentTime = millis();
  const unsigned long INITIAL_DELAY = 400;   // Wait 400ms before starting repeat
  const unsigned long REPEAT_INTERVAL = 80;  // Repeat every 80ms while held

  // Check if we should trigger a repeat
  unsigned long timeSincePress = currentTime - keyboard->keyPressStart;

  if (timeSincePress < INITIAL_DELAY) {
    // Still in initial delay period, don't repeat yet
    return;
  }

  unsigned long timeSinceLastRepeat = currentTime - keyboard->lastRepeat;

  if (timeSinceLastRepeat < REPEAT_INTERVAL) {
    // Not enough time has passed since last repeat
    return;
  }

  // Time to repeat the key
  keyboard->lastRepeat = currentTime;

  // Handle backspace repeat
  if (keyboard->isBackspace) {
    if (inputField->cursorPos > 0) {
      inputField->text = inputField->text.substring(0, inputField->cursorPos - 1) +
                         inputField->text.substring(inputField->cursorPos);
      inputField->cursorPos--;

      // Update the input field display
      if (window != nullptr) {
        drawComponent(lcd, *inputComponent, window->x, window->y);
      }
    }
    return;
  }

  // Handle space repeat
  if (keyboard->isSpace) {
    if (inputField->text.length() < inputField->maxLength) {
      inputField->text = inputField->text.substring(0, inputField->cursorPos) + " " +
                         inputField->text.substring(inputField->cursorPos);
      inputField->cursorPos++;

      // Update the input field display
      if (window != nullptr) {
        drawComponent(lcd, *inputComponent, window->x, window->y);
      }
    }
    return;
  }

  // Handle regular character repeat
  if (keyboard->lastPressedChar != '\0' && inputField->text.length() < inputField->maxLength) {
    inputField->text = inputField->text.substring(0, inputField->cursorPos) +
                       String(keyboard->lastPressedChar) +
                       inputField->text.substring(inputField->cursorPos);
    inputField->cursorPos++;

    // Update the input field display
    if (window != nullptr) {
      drawComponent(lcd, *inputComponent, window->x, window->y);
    }
  }
}
