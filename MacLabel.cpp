/*
 * MacLabel.cpp - Label Component Implementation
 * 
 * Copyright (c) 2025 Felangga
 */

#include "MacUI.h"

void drawLabel(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const MacLabel& label) {
  // Draw background
  lcd.fillRect(x, y, w, h, label.backgroundColor);
  
  // Draw text
  lcd.setTextColor(label.textColor, label.backgroundColor);
  lcd.setTextSize(label.textSize);
  
  int textX, textY;
  if (label.centerAlign) {
    textX = x + (w - label.text.length() * 6 * label.textSize) / 2;
    textY = y + (h - 8 * label.textSize) / 2;
  } else {
    textX = x + 2;
    textY = y + (h - 8 * label.textSize) / 2;
  }
  
  lcd.setCursor(textX, textY);
  lcd.print(label.text);
}

MacComponent* createLabelComponent(int x, int y, int w, int h, int id, const String& text, uint16_t textColor) {
  MacComponent* component = createComponent(COMPONENT_LABEL, x, y, w, h, id);
  
  // Create label-specific data
  MacLabel* labelData = new MacLabel();
  labelData->text = text;
  labelData->textColor = textColor;
  labelData->backgroundColor = MAC_WHITE;
  labelData->textSize = 1;
  labelData->centerAlign = false;
  
  component->customData = labelData;
  return component;
}
