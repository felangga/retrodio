/*
 * MacButton.cpp - Button Component Implementation
 * 
 * Copyright (c) 2025 Felangga
 */

#include "MacUI.h"

/**
 * Draw a symbol (play, pause, stop, etc.) at the specified position
 */
void drawSymbol(lgfx::LGFX_Device& lcd, int x, int y, int size, SymbolType symbol, uint16_t color) {
  switch (symbol) {
    case SYMBOL_PLAY:
      for (int i = 0; i < size; i++) {
        int lineHeight = ((size - i) * size) / size;
        lcd.drawFastVLine(x + i, y + (size - lineHeight) / 2, lineHeight, color);
      }
      break;

    case SYMBOL_PAUSE:
      {
        int barWidth = size / 4;
        int spacing = size / 3;
        lcd.fillRect(x, y, barWidth, size, color);
        lcd.fillRect(x + barWidth + spacing, y, barWidth, size, color);
      }
      break;

    case SYMBOL_STOP:
      lcd.fillRect(x, y, size, size, color);
      break;

    case SYMBOL_PREV:
      {
        lcd.fillRect(x, y, 2, size, color); 
        int triSize = size - 4;
        for (int i = 0; i < triSize; i++) {
          int lineHeight = ((i + 1) * size) / triSize;
          lcd.drawFastVLine(x + 3 + i, y + (size - lineHeight) / 2, lineHeight, color);
        }
      }
      break;

    case SYMBOL_NEXT:
      {
        int triSize = size - 4;
        for (int i = 0; i < triSize; i++) {
          int lineHeight = ((triSize - i) * size) / triSize;
          lcd.drawFastVLine(x + i, y + (size - lineHeight) / 2, lineHeight, color);
        }
        lcd.fillRect(x + size - 2, y, 2, size, color);  
      }
      break;

    case SYMBOL_VOL_UP:
      lcd.fillRect(x, y + size / 3, size / 3, size / 3, color);
      lcd.fillRect(x + size / 3, y + size / 4, size / 6, size / 2, color);
      // Sound waves
      for (int i = 1; i <= 3; i++) {
        int waveX = x + size / 2 + i * size / 8;
        int waveY = y + size / 2 - i * size / 12;
        int waveH = i * size / 6;
        lcd.drawFastVLine(waveX, waveY, waveH, color);
      }
      break;

    case SYMBOL_VOL_DOWN:
      lcd.fillRect(x, y + size / 3, size / 3, size / 3, color);
      lcd.fillRect(x + size / 3, y + size / 4, size / 6, size / 2, color);
      lcd.fillRect(x + size / 2 + 2, y + size / 2 - 1, size / 4, 2, color);
      break;

    case SYMBOL_NONE:
    default:
      break;
  }
}

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
