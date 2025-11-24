/*
 * MacTextBox.cpp - TextBox Component Implementation
 * 
 * Copyright (c) 2025 Felangga
 */

#include "MacUI.h"

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
