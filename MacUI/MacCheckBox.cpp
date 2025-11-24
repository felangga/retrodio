/*
 * MacCheckBox.cpp - CheckBox Component Implementation
 * 
 * Copyright (c) 2025 Felangga
 */

#include "MacUI.h"

void drawCheckBox(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const MacCheckBox& checkbox) {
  int boxSize = min(16, min(w, h));
  int boxX = x + 2;
  int boxY = y + (h - boxSize) / 2;
  
  // Draw checkbox box
  lcd.fillRect(boxX, boxY, boxSize, boxSize, MAC_WHITE);
  lcd.drawRect(boxX, boxY, boxSize, boxSize, MAC_BLACK);
  draw3DFrame(lcd, boxX + 1, boxY + 1, boxSize - 2, boxSize - 2, true);
  
  // Draw checkmark if checked
  if (checkbox.checked) {
    // Simple checkmark
    lcd.drawLine(boxX + 3, boxY + boxSize/2, boxX + boxSize/2, boxY + boxSize - 4, MAC_BLACK);
    lcd.drawLine(boxX + boxSize/2, boxY + boxSize - 4, boxX + boxSize - 3, boxY + 3, MAC_BLACK);
  }
  
  // Draw label
  lcd.setTextColor(MAC_BLACK, MAC_WHITE);
  lcd.setTextSize(1);
  lcd.setCursor(boxX + boxSize + 4, boxY + (boxSize - 8) / 2);
  lcd.print(checkbox.label);
}

MacComponent* createCheckBoxComponent(int x, int y, int w, int h, int id, const String& label, bool checked) {
  MacComponent* component = createComponent(COMPONENT_CHECKBOX, x, y, w, h, id);
  
  // Create checkbox-specific data
  MacCheckBox* checkboxData = new MacCheckBox();
  checkboxData->label = label;
  checkboxData->checked = checked;
  
  component->customData = checkboxData;
  return component;
}
