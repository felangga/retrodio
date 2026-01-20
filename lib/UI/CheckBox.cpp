/*
 * CheckBox.cpp - CheckBox Component Implementation
 *
 * Copyright (c) 2025 felangga
 */

#include "UI.h"

void drawCheckBox(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const UICheckBox& checkbox) {
  int boxSize = min(16, min(w, h));
  int boxX = x + 2;
  int boxY = y + (h - boxSize) / 2;

  lcd.fillRect(boxX, boxY, boxSize, boxSize, UI_WHITE);
  lcd.drawRect(boxX, boxY, boxSize, boxSize, UI_BLACK);
  draw3DFrame(lcd, boxX + 1, boxY + 1, boxSize - 2, boxSize - 2);

  if (checkbox.checked) {
    lcd.drawLine(boxX + 3, boxY + boxSize / 2, boxX + boxSize / 2, boxY + boxSize - 4, UI_BLACK);
    lcd.drawLine(boxX + boxSize / 2, boxY + boxSize - 4, boxX + boxSize - 3, boxY + 3, UI_BLACK);
  }

  lcd.setTextColor(UI_BLACK, UI_WHITE);
  lcd.setTextSize(1);
  lcd.setCursor(boxX + boxSize + 4, boxY + (boxSize - 8) / 2);
  lcd.print(checkbox.label);
}

UIComponent* createCheckBoxComponent(int x, int y, int w, int h, int id, const String& label,
                                      bool checked) {
  UIComponent* component = createComponent(COMPONENT_CHECKBOX, x, y, w, h, id);

  UICheckBox* checkboxData = new UICheckBox();
  checkboxData->label = label;
  checkboxData->checked = checked;

  component->customData = checkboxData;
  return component;
}
