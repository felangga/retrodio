/*
 * MacInputField.cpp - Input field component with cursor
 * Part of MacUI Library
 */

#include "MacUI.h"

void drawInputField(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, MacInputField& inputField) {
  // Draw 3D inset frame for input field
  draw3DFrame(lcd, x, y, w, h);
  lcd.fillRect(x + 2, y + 2, w - 4, h - 4, MAC_WHITE);
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

    // Measure text width
    lcd.setTextSize(1);

    // Calculate cursor position
    String textBeforeCursor = inputField.text.substring(0, inputField.cursorPos);
    int cursorX = lcd.textWidth(textBeforeCursor.c_str());

    // Calculate scroll offset to keep cursor visible
    int scrollOffset = 0;
    if (cursorX > maxVisibleWidth - 10) {
      // Scroll so cursor is near the right edge with some padding
      scrollOffset = cursorX - maxVisibleWidth + 20;
    }

    // Set clipping region for text to prevent overflow
    lcd.setClipRect(x + 3, y + 3, w - 6, h - 6);

    // Draw the text without background (transparent text mode)
    lcd.setTextColor(MAC_BLACK);
    lcd.setTextSize(1);

    // Calculate text start position with scroll offset
    int textStartX = textX - scrollOffset;

    // Draw each character individually to prevent wrapping
    lcd.setCursor(textStartX, textY);
    for (int i = 0; i < inputField.text.length(); i++) {
      lcd.print(inputField.text[i]);
    }

    lcd.clearClipRect();

    // Draw cursor if focused
    if (inputField.focused && inputField.cursorVisible) {
      int cursorScreenX = textX + cursorX - scrollOffset;

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

  // Draw focus indicator (blue border) if focused
  if (inputField.focused) {
    lcd.drawRect(x, y, w, h, MAC_BLUE);
  }

  // Reset font to default
  lcd.setFont(nullptr);
}

MacComponent* createInputFieldComponent(int x, int y, int w, int h, int id,
                                        const String& placeholder, int maxLength) {
  MacComponent* component = createComponent(COMPONENT_INPUT_FIELD, x, y, w, h, id);

  MacInputField* inputField = new MacInputField();
  inputField->text = "";
  inputField->placeholder = placeholder;
  inputField->focused = false;
  inputField->cursorPos = 0;
  inputField->maxLength = maxLength;
  inputField->lastCursorBlink = millis();
  inputField->cursorVisible = true;
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
