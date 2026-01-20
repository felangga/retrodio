/*
 * TextBox.cpp - TextBox Component Implementation
 *
 * Copyright (c) 2025 felangga
 */

#include "UI.h"

void drawTextBox(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const UITextBox& textbox) {
  uint16_t borderColor = textbox.focused ? UI_DARK_GRAY : UI_BLACK;
  uint16_t bgColor = UI_WHITE;

  lcd.fillRect(x, y, w, h, bgColor);
  lcd.drawRect(x, y, w, h, borderColor);
  draw3DFrame(lcd, x + 1, y + 1, w - 2, h - 2);

  lcd.setTextColor(UI_BLACK, bgColor);
  lcd.setTextSize(1);
  lcd.setCursor(x + 4, y + (h - 8) / 2);

  if (textbox.text.length() > 0) {
    lcd.print(textbox.text);

    if (textbox.focused) {
      int cursorX = x + 4 + textbox.cursorPos * 6;
      lcd.drawFastVLine(cursorX, y + 2, h - 4, UI_BLACK);
    }
  } else if (textbox.placeholder.length() > 0) {
    lcd.setTextColor(UI_GRAY, bgColor);
    lcd.print(textbox.placeholder);
  }
}

UIComponent* createTextBoxComponent(int x, int y, int w, int h, int id, const String& placeholder) {
  UIComponent* component = createComponent(COMPONENT_TEXTBOX, x, y, w, h, id);

  UITextBox* textboxData = new UITextBox();
  textboxData->text = "";
  textboxData->placeholder = placeholder;
  textboxData->focused = false;
  textboxData->cursorPos = 0;
  textboxData->maxLength = 50;

  component->customData = textboxData;
  return component;
}
