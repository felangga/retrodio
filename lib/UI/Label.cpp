/*
 * Label.cpp - Label Component Implementation
 *
 * Copyright (c) 2025 felangga
 */

#include "UI.h"

void drawLabel(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const UILabel& label) {
  lcd.fillRect(x, y, w, h, label.backgroundColor);

  lcd.setTextColor(label.textColor, label.backgroundColor);
  lcd.setFont(getFontFromType(label.font));
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

  // Reset font to default
  lcd.setFont(nullptr);
}

UIComponent* createLabelComponent(int x, int y, int w, int h, int id, const String& text,
                                   uint16_t textColor) {
  UIComponent* component = createComponent(COMPONENT_LABEL, x, y, w, h, id);

  UILabel* labelData = new UILabel();
  labelData->text = text;
  labelData->textColor = textColor;
  labelData->backgroundColor = UI_WHITE;
  labelData->textSize = 1;
  labelData->centerAlign = false;
  labelData->font = FONT_DEFAULT;  

  component->customData = labelData;
  return component;
}
