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

  lcd.setTextSize(2);
  int textW = lcd.textWidth(keyChar);
  int textH = 16;
  lcd.setCursor(keyX + (keyWidth - textW) / 2, keyY + (keyHeight - textH) / 2);
  lcd.print(keyChar);
}

void drawKeyboard(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, MacKeyboard& keyboard) {
  if (!keyboard.visible)
    return;

  // Draw keyboard background
  lcd.fillRect(x, y, w, h, MAC_LIGHT_GRAY);
  lcd.drawRect(x, y, w, h, MAC_BLACK);

  // Uniform margin on all sides
  int margin = 5;
  int keySpacing = 2;
  int rowHeight = (h - (2 * margin) - 32) / rowCount;  // Equal top/bottom margin, reserve space for special row

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
    int rowWidth = w - (2 * margin);
    int keyWidth = (rowWidth - (keyCount - 1) * keySpacing) / keyCount;

    int rowY = y + margin + row * rowHeight;
    int startX = x + margin;

    // Draw each key in the row
    for (int i = 0; i < keyCount; i++) {
      int keyX = startX + i * (keyWidth + keySpacing);
      char keyChar[2] = {keys[i], '\0'};

      // Draw key with Mac Classic style
      drawMacKey(lcd, keyX, rowY, keyWidth, rowHeight - keySpacing, keyChar, false);
    }
  }

  // Row 3 (zxcvbnm) with Shift on left and Backspace on right
  int row3Y = y + margin + 3 * rowHeight;
  int shiftW = 45;  // Shift button width
  int backspaceW = 55;  // Backspace button width
  int row3KeySpacing = 2;

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
  int availableWidth = w - (2 * margin) - shiftW - backspaceW - (2 * row3KeySpacing);
  int row3KeyWidth = (availableWidth - (row3KeyCount - 1) * row3KeySpacing) / row3KeyCount;

  // Draw Shift button (left side of row 3)
  int shiftX = x + margin;
  if (keyboard.shiftActive) {
    lcd.fillRoundRect(shiftX, row3Y, shiftW, rowHeight - keySpacing, radius, MAC_BLACK);
    lcd.drawRoundRect(shiftX, row3Y, shiftW, rowHeight - keySpacing, radius, MAC_BLACK);
    lcd.setTextColor(MAC_WHITE, MAC_BLACK);
  } else {
    lcd.fillRoundRect(shiftX, row3Y, shiftW, rowHeight - keySpacing, radius, MAC_WHITE);
    lcd.drawRoundRect(shiftX, row3Y, shiftW, rowHeight - keySpacing, radius, MAC_BLACK);
    lcd.drawRoundRect(shiftX + 1, row3Y + 1, shiftW - 2, rowHeight - keySpacing - 2, 2, MAC_BLACK);
    lcd.setTextColor(MAC_BLACK, MAC_WHITE);
  }
  lcd.setTextSize(1);
  // Center-align "Shift" text
  int shiftTextW = lcd.textWidth("Shift");
  lcd.setCursor(shiftX + (shiftW - shiftTextW) / 2, row3Y + (rowHeight - keySpacing - 8) / 2);
  lcd.print("Shift");

  // Draw letter keys (zxcvbnm)
  int row3StartX = shiftX + shiftW + row3KeySpacing;
  for (int i = 0; i < row3KeyCount; i++) {
    int keyX = row3StartX + i * (row3KeyWidth + row3KeySpacing);
    char keyChar[2] = {row3Keys[i], '\0'};
    drawMacKey(lcd, keyX, row3Y, row3KeyWidth, rowHeight - keySpacing, keyChar, false);
  }

  // Draw Backspace button (right side of row 3)
  int backspaceX = row3StartX + row3KeyCount * (row3KeyWidth + row3KeySpacing);
  lcd.fillRoundRect(backspaceX, row3Y, backspaceW, rowHeight - keySpacing, radius, MAC_WHITE);
  lcd.drawRoundRect(backspaceX, row3Y, backspaceW, rowHeight - keySpacing, radius, MAC_BLACK);
  lcd.drawRoundRect(backspaceX + 1, row3Y + 1, backspaceW - 2, rowHeight - keySpacing - 2, radius, MAC_BLACK);
  lcd.setTextColor(MAC_BLACK, MAC_WHITE);
  lcd.setTextSize(1);
  // Center-align "Delete" text
  int deleteTextW = lcd.textWidth("Delete");
  lcd.setCursor(backspaceX + (backspaceW - deleteTextW) / 2, row3Y + (rowHeight - keySpacing - 8) / 2);
  lcd.print("Delete");

  // Draw special keys row at bottom (Sym, Space, Done)
  int specialRowY = y + h - margin - 28;
  int specialKeyHeight = 28;
  int specialKeyW = 50;
  int doneKeyW = 55;  // Same width as backspace
  int spacing = 5;

  // Sym/ABC toggle key (leftmost)
  int symX = x + margin;
  if (keyboard.selectedKey == -2) {
    lcd.fillRoundRect(symX, specialRowY, specialKeyW, specialKeyHeight, radius, MAC_BLACK);
    lcd.drawRoundRect(symX, specialRowY, specialKeyW, specialKeyHeight, radius, MAC_BLACK);
    lcd.setTextColor(MAC_WHITE, MAC_BLACK);
  } else {
    lcd.fillRoundRect(symX, specialRowY, specialKeyW, specialKeyHeight, radius, MAC_WHITE);
    lcd.drawRoundRect(symX, specialRowY, specialKeyW, specialKeyHeight, radius, MAC_BLACK);
    lcd.drawRoundRect(symX + 1, specialRowY + 1, specialKeyW - 2, specialKeyHeight - 2, radius-1, MAC_BLACK);
    lcd.setTextColor(MAC_BLACK, MAC_WHITE);
  }
  lcd.setTextSize(1);
  // Center-align Sym/ABC text
  const char* symText = keyboard.selectedKey == -2 ? "ABC" : "Sym";
  int symTextW = lcd.textWidth(symText);
  lcd.setCursor(symX + (specialKeyW - symTextW) / 2, specialRowY + (specialKeyHeight - 8) / 2);
  lcd.print(symText);

  // Space bar (middle, takes remaining space between Sym and Done)
  int spaceX = symX + specialKeyW + spacing;
  int spaceW = w - (2 * margin) - specialKeyW - doneKeyW - (2 * spacing);
  lcd.fillRoundRect(spaceX, specialRowY, spaceW, specialKeyHeight, radius, MAC_WHITE);
  lcd.drawRoundRect(spaceX, specialRowY, spaceW, specialKeyHeight, radius, MAC_BLACK);
  lcd.drawRoundRect(spaceX + 1, specialRowY + 1, spaceW - 2, specialKeyHeight - 2, radius-1, MAC_BLACK);
  lcd.setTextColor(MAC_BLACK, MAC_WHITE);
  lcd.setTextSize(1);
  // Center-align "Space" text
  int spaceTextW = lcd.textWidth("Space");
  lcd.setCursor(spaceX + (spaceW - spaceTextW) / 2, specialRowY + (specialKeyHeight - 8) / 2);
  lcd.print("Space");

  // Done button (rightmost)
  int doneX = spaceX + spaceW + spacing;
  lcd.fillRoundRect(doneX, specialRowY, doneKeyW, specialKeyHeight, radius, MAC_WHITE);
  lcd.drawRoundRect(doneX, specialRowY, doneKeyW, specialKeyHeight, radius, MAC_BLACK);
  lcd.drawRoundRect(doneX + 1, specialRowY + 1, doneKeyW - 2, specialKeyHeight - 2, radius-1, MAC_BLACK);
  lcd.setTextColor(MAC_BLACK, MAC_WHITE);
  lcd.setTextSize(1);
  // Center-align "Done" text
  int doneTextW = lcd.textWidth("Done");
  lcd.setCursor(doneX + (doneKeyW - doneTextW) / 2, specialRowY + (specialKeyHeight - 8) / 2);
  lcd.print("Done");
}

MacComponent* createKeyboardComponent(int x, int y, int w, int h, int id, int targetInputId) {
  MacComponent* component = createComponent(COMPONENT_KEYBOARD, x, y, w, h, id);

  MacKeyboard* keyboard = new MacKeyboard();
  keyboard->visible = false;  // Start hidden
  keyboard->x = x;
  keyboard->y = y;
  keyboard->w = w;
  keyboard->h = h;
  keyboard->targetInputId = targetInputId;
  keyboard->shiftActive = false;
  keyboard->selectedKey = -1;

  component->customData = keyboard;
  return component;
}

// Helper function to handle keyboard touch and update input field
bool handleKeyboardTouch(lgfx::LGFX_Device& lcd, MacComponent* keyboardComponent,
                         MacComponent* inputComponent, int touchX, int touchY) {
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
  int margin = 5;
  int keySpacing = 2;
  int rowHeight = (h - (2 * margin) - 32) / rowCount;

  // Check special keys (bottom row) - Sym, Space, and Done
  int specialRowY = h - margin - 28;
  int specialKeyHeight = 28;
  int specialKeyW = 50;
  int doneKeyW = 55;
  int spacing = 5;

  if (relY >= specialRowY) {
    // Sym/ABC toggle key (leftmost)
    if (relX >= margin && relX <= margin + specialKeyW) {
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
      return true;
    }

    // Calculate space bar and done button positions
    int spaceStartX = 5 + specialKeyW + spacing;
    int spaceW = w - specialKeyW - doneKeyW - (4 * spacing) - 10;
    int spaceEndX = spaceStartX + spaceW;
    int doneStartX = spaceEndX + spacing;
    int doneEndX = doneStartX + doneKeyW;

    // Space bar (middle)
    if (relX >= spaceStartX && relX <= spaceEndX) {
      if (inputField->text.length() < inputField->maxLength) {
        // Visual feedback for space bar
        int spaceX = x + spaceStartX;
        int btnY = y + h - 32;
        int btnH = 28;
        lcd.startWrite();
        lcd.fillRoundRect(spaceX, btnY, spaceW, btnH, radius, MAC_BLACK);
        lcd.drawRoundRect(spaceX, btnY, spaceW, btnH, radius, MAC_BLACK);
        lcd.setTextColor(MAC_WHITE, MAC_BLACK);
        lcd.setTextSize(1);
        lcd.setCursor(spaceX + spaceW / 2 - 18, btnY + 10);
        lcd.print("Space");
        lcd.endWrite();
        delay(80);

        // Add space
        inputField->text = inputField->text.substring(0, inputField->cursorPos) + " " +
                           inputField->text.substring(inputField->cursorPos);
        inputField->cursorPos++;

        // Restore space bar
        lcd.startWrite();
        lcd.fillRoundRect(spaceX, btnY, spaceW, btnH, radius, MAC_WHITE);
        lcd.drawRoundRect(spaceX, btnY, spaceW, btnH, radius, MAC_BLACK);
        lcd.drawRoundRect(spaceX + 1, btnY + 1, spaceW - 2, btnH - 2, radius-1, MAC_BLACK);
        lcd.setTextColor(MAC_BLACK, MAC_WHITE);
        lcd.setTextSize(1);
        lcd.setCursor(spaceX + spaceW / 2 - 18, btnY + 10);
        lcd.print("Space");
        lcd.endWrite();

        return true;
      }
    }

    // Done button (rightmost) - hides keyboard
    if (relX >= doneStartX && relX <= doneEndX) {
      // Visual feedback for done button
      int doneX = x + doneStartX;
      int btnY = y + h - 32;
      int btnH = 28;
      lcd.startWrite();
      lcd.fillRoundRect(doneX, btnY, doneKeyW, btnH, radius, MAC_BLACK);
      lcd.drawRoundRect(doneX, btnY, doneKeyW, btnH, radius, MAC_BLACK);
      lcd.setTextColor(MAC_WHITE, MAC_BLACK);
      lcd.setTextSize(1);
      lcd.setCursor(doneX + 13, btnY + 10);
      lcd.print("Done");
      lcd.endWrite();
      delay(80);

      // Hide the keyboard
      keyboard->visible = false;
      inputField->focused = false;

      // Restore done button before hiding
      lcd.startWrite();
      lcd.fillRoundRect(doneX, btnY, doneKeyW, btnH, radius, MAC_WHITE);
      lcd.drawRoundRect(doneX, btnY, doneKeyW, btnH, radius, MAC_BLACK);
      lcd.drawRoundRect(doneX + 1, btnY + 1, doneKeyW - 2, btnH - 2, radius-1, MAC_BLACK);
      lcd.setTextColor(MAC_BLACK, MAC_WHITE);
      lcd.setTextSize(1);
      lcd.setCursor(doneX + 13, btnY + 10);
      lcd.print("Done");
      lcd.endWrite();

      return true;
    }

    return false;
  }

  // Check regular keys
  int row = (relY - 5) / rowHeight;
  if (row >= 0 && row < rowCount) {
    // Handle row 3 specially (with Shift and Backspace)
    if (row == 3) {
      int row3Y = y + 5 + 3 * rowHeight;
      int row3RelY = relY - (5 + 3 * rowHeight);
      int shiftW = 45;
      int backspaceW = 55;
      int row3KeySpacing = 2;

      // Check Shift button (leftmost in row 3)
      if (relX >= 5 && relX <= 5 + shiftW && row3RelY >= 0 && row3RelY <= rowHeight - keySpacing) {
        keyboard->shiftActive = !keyboard->shiftActive;
        lcd.startWrite();
        drawKeyboard(lcd, x, y, w, h, *keyboard);
        lcd.endWrite();
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
      int availableWidth = w - 10 - shiftW - backspaceW - (2 * row3KeySpacing);
      int row3KeyWidth = (availableWidth - (row3KeyCount - 1) * row3KeySpacing) / row3KeyCount;
      int row3StartX = 5 + shiftW + row3KeySpacing;

      // Check letter keys (zxcvbnm)
      if (relX >= row3StartX) {
        int relXFromRow3Start = relX - row3StartX;
        int keyIndex = relXFromRow3Start / (row3KeyWidth + row3KeySpacing);

        if (keyIndex >= 0 && keyIndex < row3KeyCount) {
          int keyX = x + row3StartX + keyIndex * (row3KeyWidth + row3KeySpacing);
          int keyEndX = keyX + row3KeyWidth;

          // Make sure touch is within this key bounds
          if (touchX >= keyX && touchX <= keyEndX) {
            if (inputField->text.length() < inputField->maxLength) {
              char newChar = row3Keys[keyIndex];
              char keyChar[2] = {newChar, '\0'};

              // Show pressed state
              lcd.startWrite();
              drawMacKey(lcd, keyX, row3Y, row3KeyWidth, rowHeight - keySpacing, keyChar, true);
              lcd.endWrite();
              delay(80);

              // Add character
              inputField->text = inputField->text.substring(0, inputField->cursorPos) + String(newChar) +
                                 inputField->text.substring(inputField->cursorPos);
              inputField->cursorPos++;

              // Restore key to normal state and handle shift
              lcd.startWrite();
              if (keyboard->shiftActive && keyboard->selectedKey != -2) {
                keyboard->shiftActive = false;
                drawKeyboard(lcd, x, y, w, h, *keyboard);
              } else {
                drawMacKey(lcd, keyX, row3Y, row3KeyWidth, rowHeight - keySpacing, keyChar, false);
              }
              lcd.endWrite();

              return true;
            }
          }
        }
      }

      // Check Backspace button (rightmost in row 3)
      int backspaceRelX = row3StartX + row3KeyCount * (row3KeyWidth + row3KeySpacing);
      if (relX >= backspaceRelX && relX <= backspaceRelX + backspaceW &&
          row3RelY >= 0 && row3RelY <= rowHeight - keySpacing) {
        if (inputField->cursorPos > 0) {
          // Visual feedback for delete/backspace
          int backspaceX = x + backspaceRelX;
          lcd.startWrite();
          lcd.fillRoundRect(backspaceX, row3Y, backspaceW, rowHeight - keySpacing, radius, MAC_BLACK);
          lcd.drawRoundRect(backspaceX, row3Y, backspaceW, rowHeight - keySpacing, radius, MAC_BLACK);
          lcd.setTextColor(MAC_WHITE, MAC_BLACK);
          lcd.setTextSize(1);
          lcd.setCursor(backspaceX + 10, row3Y + (rowHeight - keySpacing - 16) / 2 + 4);
          lcd.print("Delete");
          lcd.endWrite();
          delay(80);

          // Delete character
          inputField->text = inputField->text.substring(0, inputField->cursorPos - 1) +
                             inputField->text.substring(inputField->cursorPos);
          inputField->cursorPos--;

          // Restore delete key
          lcd.startWrite();
          lcd.fillRoundRect(backspaceX, row3Y, backspaceW, rowHeight - keySpacing, radius, MAC_WHITE);
          lcd.drawRoundRect(backspaceX, row3Y, backspaceW, rowHeight - keySpacing, radius, MAC_BLACK);
          lcd.drawRoundRect(backspaceX + 1, row3Y + 1, backspaceW - 2, rowHeight - keySpacing - 2, radius-1, MAC_BLACK);
          lcd.setTextColor(MAC_BLACK, MAC_WHITE);
          lcd.setTextSize(1);
          lcd.setCursor(backspaceX + 10, row3Y + (rowHeight - keySpacing - 16) / 2 + 4);
          lcd.print("Delete");
          lcd.endWrite();

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
    int rowWidth = w - 10;
    int keyWidth = (rowWidth - (keyCount - 1) * keySpacing) / keyCount;

    int keyIndex = (relX - 5) / (keyWidth + keySpacing);
    if (keyIndex >= 0 && keyIndex < keyCount) {
      // Add character to input field
      if (inputField->text.length() < inputField->maxLength) {
        char newChar = keys[keyIndex];

        // Calculate key position for visual feedback
        int startX = x + 5;
        int rowY = y + 5 + row * rowHeight;
        int keyX = startX + keyIndex * (keyWidth + keySpacing);
        char keyChar[2] = {newChar, '\0'};

        // Show pressed state
        lcd.startWrite();
        drawMacKey(lcd, keyX, rowY, keyWidth, rowHeight - keySpacing, keyChar, true);
        lcd.endWrite();
        delay(80);  // Brief visual feedback

        // Add character
        inputField->text = inputField->text.substring(0, inputField->cursorPos) + String(newChar) +
                           inputField->text.substring(inputField->cursorPos);
        inputField->cursorPos++;

        // Restore key to normal state and handle shift
        lcd.startWrite();
        if (keyboard->shiftActive && keyboard->selectedKey != -2 && row > 0) {
          keyboard->shiftActive = false;
          drawKeyboard(lcd, x, y, w, h, *keyboard);
        } else {
          drawMacKey(lcd, keyX, rowY, keyWidth, rowHeight - keySpacing, keyChar, false);
        }
        lcd.endWrite();

        return true;
      }
    }
  }

  return false;
}
