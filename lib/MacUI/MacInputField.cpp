/*
 * MacInputField.cpp - Input field component with cursor
 * 
 * Copyright (c) 2025 felangga
 */

#include "MacUI.h"

void drawInputField(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, MacInputField& inputField) {
  draw3DFrame(lcd, x, y, w, h);
  lcd.fillRect(x + 2, y + 2, w - 4, h - 4, MAC_WHITE);

  // Set font and text size BEFORE measuring text widths
  lcd.setTextSize(1);
  lcd.setFont(getFontFromType(inputField.font));

  // Calculate text position
  int textX = x + 5;
  int textY = y + (h - 1) / 2; 

  // Handle cursor blinking (500ms interval)
  unsigned long currentTime = millis();
  if (inputField.focused && currentTime - inputField.lastCursorBlink > 500) {
    inputField.cursorVisible = !inputField.cursorVisible;
    inputField.lastCursorBlink = currentTime;
  }

  if (inputField.text.length() > 0) {
    // Calculate visible text (scroll if needed)
    int maxVisibleWidth = w - 10;  // Leave space for padding

    // Calculate cursor position in pixels
    String textBeforeCursor = inputField.text.substring(0, inputField.cursorPos);
    int cursorX = lcd.textWidth(textBeforeCursor.c_str());

    // Calculate total text width
    int totalTextWidth = lcd.textWidth(inputField.text.c_str());

    // Calculate scroll offset to keep cursor visible
    const int CURSOR_PADDING = 10;
    
    // Only reset scroll if all text fits, otherwise follow cursor
    if (totalTextWidth <= maxVisibleWidth) {
      inputField.scrollOffset = 0;
    } else {
      // Text is longer than visible area - scroll to follow cursor
      if (cursorX > inputField.scrollOffset + maxVisibleWidth - CURSOR_PADDING) {
        // Cursor is beyond right edge - scroll right
        inputField.scrollOffset = cursorX - maxVisibleWidth + CURSOR_PADDING;
      } else if (cursorX < inputField.scrollOffset + CURSOR_PADDING) {
        // Cursor is beyond left edge - scroll left
        inputField.scrollOffset = max(0, cursorX - CURSOR_PADDING);
      }

      // Ensure we don't scroll past the end
      int maxScrollOffset = totalTextWidth - maxVisibleWidth;
      if (inputField.scrollOffset > maxScrollOffset) {
        inputField.scrollOffset = maxScrollOffset;
      }
    }

    // Calculate text start position with scroll offset
    int textStartX = textX - inputField.scrollOffset;

    // Define visible area boundaries
    int visibleLeft = x + 3;
    int visibleRight = x + w - 3;

    // Draw only characters that are visible within the bounds
    int charX = textStartX;
    for (int i = 0; i < inputField.text.length(); i++) {
      char c = inputField.text[i];
      int charWidth = lcd.textWidth(String(c).c_str());

      // Check if this character is fully within visible bounds
      if (charX >= visibleLeft && charX + charWidth <= visibleRight) {
        lcd.setCursor(charX, textY);
        lcd.print(c);
      }

      charX += charWidth;

      // Stop drawing if we've gone past the visible area
      if (charX >= visibleRight) {
        break;
      }
    }

    // Draw cursor if focused
    if (inputField.focused && inputField.cursorVisible) {
      int cursorScreenX = textX + cursorX - inputField.scrollOffset;

      // Only draw cursor if it's within visible area
      if (cursorScreenX >= x + 5 && cursorScreenX < x + w - 5) {
        lcd.drawLine(cursorScreenX, y + 5, cursorScreenX, y + h - 5, MAC_BLACK);
      }
    }
  } else {
    // Show placeholder text if empty
    lcd.setTextColor(MAC_GRAY);
    lcd.setTextSize(1);
    lcd.setCursor(textX, textY);
    lcd.print(inputField.placeholder);
    
    // Draw cursor at start if focused
    if (inputField.focused && inputField.cursorVisible) {
      lcd.drawLine(textX, y + 5, textX, y + h - 5, MAC_BLACK);
    }
  }

  if (inputField.focused) {
    lcd.drawRect(x, y, w, h, MAC_BLACK);
  }

  lcd.setFont(nullptr);
}

MacComponent* createInputFieldComponent(int x, int y, int w, int h, int id,
                                        const String& placeholder, int maxLength, const String& defaultText) {
  MacComponent* component = createComponent(COMPONENT_INPUT_FIELD, x, y, w, h, id);

  MacInputField* inputField = new MacInputField();
  inputField->text = defaultText;
  inputField->placeholder = placeholder;
  inputField->focused = false;
  inputField->cursorPos = defaultText.length();  
  inputField->maxLength = maxLength;
  inputField->lastCursorBlink = millis();
  inputField->cursorVisible = true;
  inputField->scrollOffset = 0;
  inputField->onTextChanged = nullptr;
  inputField->font = FONT_CHICAGO_9PT;

  component->customData = inputField;
  return component;
}

// Update all input field components in a window (for cursor blinking)
void updateInputFieldComponents(lgfx::LGFX_Device& lcd, MacWindow& window) {
  if (!window.visible || window.childComponents == nullptr || window.childComponentCount == 0) {
    return;
  }

  // Skip updates if window is being dragged to prevent flicker
  if (window.isDragging) {
    return;
  }

  for (int i = 0; i < window.childComponentCount; i++) {
    MacComponent* component = window.childComponents[i];
    if (component != nullptr && component->visible && component->type == COMPONENT_INPUT_FIELD) {
      if (component->customData != nullptr) {
        MacInputField* inputFieldData = (MacInputField*)component->customData;

        // Check if cursor blink state changed
        unsigned long currentTime = millis();
        if (inputFieldData->focused && currentTime - inputFieldData->lastCursorBlink >= 500) {
          // Redraw this component to update cursor blink
          drawComponent(lcd, *component, window.x, window.y);
        }
      }
    }
  }
}
