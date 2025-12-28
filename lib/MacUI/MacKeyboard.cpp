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

void drawKeyboard(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, MacKeyboard& keyboard) {
  if (!keyboard.visible)
    return;

  // Draw keyboard background
  lcd.fillRect(x, y, w, h, MAC_LIGHT_GRAY);
  lcd.drawRect(x, y, w, h, MAC_BLACK);

  // Calculate key dimensions
  int keySpacing = 2;
  int rowHeight = (h - 40) / rowCount;  // Leave space for special keys at bottom

  // Draw each row
  for (int row = 0; row < rowCount; row++) {
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
    int rowWidth = w - 10;
    int keyWidth = (rowWidth - (keyCount - 1) * keySpacing) / keyCount;

    int rowY = y + 5 + row * rowHeight;
    int startX = x + 5;

    // Draw each key in the row
    for (int i = 0; i < keyCount; i++) {
      int keyX = startX + i * (keyWidth + keySpacing);

      // Draw key button
      draw3DFrame(lcd, keyX, rowY, keyWidth, rowHeight - keySpacing, false);
      lcd.fillRect(keyX + 2, rowY + 2, keyWidth - 4, rowHeight - keySpacing - 4, MAC_WHITE);

      // Draw key label
      lcd.setTextColor(MAC_BLACK, MAC_WHITE);
      lcd.setTextSize(2);

      char keyChar[2] = {keys[i], '\0'};
      // Center text manually
      int textW = lcd.textWidth(keyChar);
      int textH = 16;  // Approximate height for size 2
      lcd.setCursor(keyX + (keyWidth - textW) / 2, rowY + (rowHeight - keySpacing - textH) / 2);
      lcd.print(keyChar);
    }
  }

  // Draw special keys row at bottom
  int specialRowY = y + h - 32;
  int specialKeyHeight = 28;

  // Shift/Sym key (left)
  int shiftX = x + 5;
  int shiftW = 50;
  draw3DFrame(lcd, shiftX, specialRowY, shiftW, specialKeyHeight, keyboard.shiftActive);
  lcd.fillRect(shiftX + 2, specialRowY + 2, shiftW - 4, specialKeyHeight - 4,
               keyboard.shiftActive ? MAC_DARK_GRAY : MAC_WHITE);
  lcd.setTextColor(keyboard.shiftActive ? MAC_WHITE : MAC_BLACK,
                   keyboard.shiftActive ? MAC_DARK_GRAY : MAC_WHITE);
  lcd.setTextSize(1);
  lcd.setCursor(shiftX + 8, specialRowY + 10);
  lcd.print(keyboard.selectedKey == -2 ? "ABC" : "Sym");

  // Space bar (center)
  int spaceX = shiftX + shiftW + 5;
  int spaceW = w - 2 * shiftW - 30;
  draw3DFrame(lcd, spaceX, specialRowY, spaceW, specialKeyHeight, false);
  lcd.fillRect(spaceX + 2, specialRowY + 2, spaceW - 4, specialKeyHeight - 4, MAC_WHITE);
  lcd.setTextColor(MAC_BLACK, MAC_WHITE);
  lcd.setTextSize(1);
  lcd.setCursor(spaceX + spaceW / 2 - 18, specialRowY + 10);
  lcd.print("Space");

  // Backspace key (right)
  int backspaceX = spaceX + spaceW + 5;
  int backspaceW = 50;
  draw3DFrame(lcd, backspaceX, specialRowY, backspaceW, specialKeyHeight, false);
  lcd.fillRect(backspaceX + 2, specialRowY + 2, backspaceW - 4, specialKeyHeight - 4, MAC_WHITE);
  lcd.setTextColor(MAC_BLACK, MAC_WHITE);
  lcd.setTextSize(1);
  lcd.setCursor(backspaceX + 15, specialRowY + 10);
  lcd.print("Del");
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

  // Calculate key dimensions
  int keySpacing = 2;
  int rowHeight = (h - 40) / rowCount;

  // Check special keys first (bottom row)
  int specialRowY = h - 32 - y;
  if (relY >= specialRowY) {
    // Shift/Symbol toggle key
    if (relX >= 5 && relX <= 55) {
      if (keyboard->selectedKey == -2) {
        // Currently in symbol mode, switch back to normal
        keyboard->selectedKey = -1;
        keyboard->shiftActive = false;
      } else {
        // Switch to symbol mode
        keyboard->selectedKey = -2;
        keyboard->shiftActive = false;
      }
      drawKeyboard(lcd, x, y, w, h, *keyboard);
      return true;
    }
    // Space bar
    else if (relX >= 60 && relX <= w - 60) {
      if (inputField->text.length() < inputField->maxLength) {
        inputField->text = inputField->text.substring(0, inputField->cursorPos) + " " +
                           inputField->text.substring(inputField->cursorPos);
        inputField->cursorPos++;
        return true;
      }
    }
    // Backspace
    else if (relX >= w - 55) {
      if (inputField->cursorPos > 0) {
        inputField->text = inputField->text.substring(0, inputField->cursorPos - 1) +
                           inputField->text.substring(inputField->cursorPos);
        inputField->cursorPos--;
        return true;
      }
    }
    return false;
  }

  // Check regular keys
  int row = (relY - 5) / rowHeight;
  if (row >= 0 && row < rowCount) {
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
        inputField->text = inputField->text.substring(0, inputField->cursorPos) + String(newChar) +
                           inputField->text.substring(inputField->cursorPos);
        inputField->cursorPos++;

        // Auto-disable shift after typing a character (in normal mode only)
        if (keyboard->shiftActive && keyboard->selectedKey != -2 && row > 0) {
          keyboard->shiftActive = false;
          drawKeyboard(lcd, x, y, w, h, *keyboard);
        }

        return true;
      }
    }
  }

  return false;
}
