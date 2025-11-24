/*
 * MacButton.cpp - Button Component Implementation
 * 
 * Copyright (c) 2025 Felangga
 */

#include "MacUI.h"

/**
 * Draw a classic Mac OS button with rounded corners and ellipsis
 */
void drawButton(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const String& text, bool pressed) {
  // Choose colors based on pressed state - inverted when pressed
  uint16_t bgColor = pressed ? MAC_BLACK : MAC_WHITE;
  uint16_t textColor = pressed ? MAC_WHITE : MAC_BLACK;
  uint16_t borderColor = MAC_BLACK;

  // Draw button background with rounded corners effect
  lcd.fillRect(x + 1, y, w - 2, h, bgColor);
  lcd.fillRect(x, y + 1, w, h - 2, bgColor);

  // Draw rounded corners (simple 1-pixel corner cuts)
  lcd.drawPixel(x + 1, y + 1, bgColor);
  lcd.drawPixel(x + w - 2, y + 1, bgColor);
  lcd.drawPixel(x + 1, y + h - 2, bgColor);
  lcd.drawPixel(x + w - 2, y + h - 2, bgColor);

  // Draw border with rounded corners
  lcd.drawFastHLine(x + 2, y, w - 4, borderColor);
  lcd.drawFastHLine(x + 2, y + h - 1, w - 4, borderColor);
  lcd.drawFastVLine(x, y + 2, h - 4, borderColor);
  lcd.drawFastVLine(x + w - 1, y + 2, h - 4, borderColor);

  // Corner pixels for rounded effect
  lcd.drawPixel(x + 1, y + 1, borderColor);
  lcd.drawPixel(x + w - 2, y + 1, borderColor);
  lcd.drawPixel(x + 1, y + h - 2, borderColor);
  lcd.drawPixel(x + w - 2, y + h - 2, borderColor);

  // Draw button text (larger text for bigger buttons)
  lcd.setTextColor(textColor, bgColor);
  lcd.setTextSize(2);
  int textX = x + (w - text.length() * 12) / 2;
  int textY = y + (h - 16) / 2;
  lcd.setCursor(textX, textY);
  lcd.print(text);

  // Draw ellipsis (three dots) below text for classic Mac look
  if (h > 24) {
    int ellipsisY = textY + 18;
    int ellipsisX = x + w / 2 - 6;
    for (int i = 0; i < 3; i++) {
      lcd.fillCircle(ellipsisX + i * 4, ellipsisY, 1, textColor);
    }
  }
}

/**
 * Draw a button with a symbol instead of text
 */
void drawSymbolButton(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, SymbolType symbol, bool pressed) {
  // Inverted colors when pressed
  uint16_t bgColor = pressed ? MAC_BLACK : MAC_WHITE;
  uint16_t borderColor = MAC_BLACK;
  uint16_t symbolColor = pressed ? MAC_WHITE : MAC_BLACK;

  lcd.fillRect(x + 1, y + 1, w - 2, h - 2, bgColor);

  // Clear corner pixels for rounded effect
  lcd.drawPixel(x, y, MAC_WHITE);
  lcd.drawPixel(x + w - 1, y, MAC_WHITE);
  lcd.drawPixel(x, y + h - 1, MAC_WHITE);
  lcd.drawPixel(x + w - 1, y + h - 1, MAC_WHITE);
  lcd.drawPixel(x + 1, y + 1, bgColor);
  lcd.drawPixel(x + w - 2, y + 1, bgColor);
  lcd.drawPixel(x + 1, y + h - 2, bgColor);
  lcd.drawPixel(x + w - 2, y + h - 2, bgColor);

  // Draw border with rounded corners
  lcd.drawFastHLine(x + 2, y, w - 4, borderColor);
  lcd.drawFastHLine(x + 2, y + h - 1, w - 4, borderColor);
  lcd.drawFastVLine(x, y + 2, h - 4, borderColor);
  lcd.drawFastVLine(x + w - 1, y + 2, h - 4, borderColor);

  // Corner pixels for rounded effect
  lcd.drawPixel(x + 1, y + 1, borderColor);
  lcd.drawPixel(x + w - 2, y + 1, borderColor);
  lcd.drawPixel(x + 1, y + h - 2, borderColor);
  lcd.drawPixel(x + w - 2, y + h - 2, borderColor);

  // Draw the symbol centered in the button
  int symbolSize = min(w, h) - 32;
  int symbolX = x + (w - symbolSize) / 2;
  int symbolY = y + (h - symbolSize) / 2;

  drawSymbol(lcd, symbolX, symbolY, symbolSize, symbol, symbolColor);
}

MacComponent* createButtonComponent(int x, int y, int w, int h, int id, const String& text, SymbolType symbol) {
  MacComponent* component = createComponent(COMPONENT_BUTTON, x, y, w, h, id);
  
  // Create button-specific data
  MacButton* buttonData = new MacButton();
  buttonData->text = text;
  buttonData->symbol = symbol;
  buttonData->pressed = false;
  
  component->customData = buttonData;
  return component;
}
