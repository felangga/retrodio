/*
 * MacButton.cpp - Button Component Implementation
 *
 * Copyright (c) 2025 felangga
 */

#include "MacUI.h"

void drawSymbol(lgfx::LGFX_Device& lcd, int x, int y, int size, SymbolType symbol, uint16_t color) {
  switch (symbol) {
    case SYMBOL_PLAY:
      for (int i = 0; i < size; i++) {
        int lineHeight = ((size - i) * size) / size;
        lcd.drawFastVLine(x + i, y + (size - lineHeight) / 2, lineHeight, color);
      }
      break;

    case SYMBOL_PAUSE: {
      int barWidth = size / 4;
      int spacing = size / 3;
      lcd.fillRect(x + 2, y, barWidth, size, color);
      lcd.fillRect(x + 2 + barWidth + spacing, y, barWidth, size, color);
    } break;

    case SYMBOL_STOP:
      lcd.fillRect(x, y, size, size, color);
      break;

    case SYMBOL_PREV: {
      lcd.fillRect(x, y, 2, size, color);
      int triSize = size - 4;
      for (int i = 0; i < triSize; i++) {
        int lineHeight = ((i + 1) * size) / triSize;
        lcd.drawFastVLine(x + 3 + i, y + (size - lineHeight) / 2, lineHeight, color);
      }
    } break;

    case SYMBOL_NEXT: {
      int triSize = size - 4;
      for (int i = 0; i < triSize; i++) {
        int lineHeight = ((triSize - i) * size) / triSize;
        lcd.drawFastVLine(x + i, y + (size - lineHeight) / 2, lineHeight, color);
      }
      lcd.fillRect(x + size - 2, y, 2, size, color);
    } break;

    case SYMBOL_LIST: {
      int lineHeight = 2;
      int spacing = size / 4;
      int lineWidth = size;
      int startY = y + (size - (3 * lineHeight + 2 * spacing)) / 2;
      lcd.fillRect(x, startY, lineWidth, lineHeight, color);
      lcd.fillRect(x, startY + lineHeight + spacing, lineWidth, lineHeight, color);
      lcd.fillRect(x, startY + 2 * (lineHeight + spacing), lineWidth, lineHeight, color);
    } break;

    case SYMBOL_NONE:
    default:
      break;
  }
}

void drawButton(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, int radius, const String& text,
                bool pressed, FontType font) {
  // Choose colors based on pressed state - inverted when pressed
  uint16_t bgColor = pressed ? MAC_BLACK : MAC_WHITE;
  uint16_t textColor = pressed ? MAC_WHITE : MAC_BLACK;
  uint16_t borderColor = MAC_BLACK;

  lcd.fillRoundRect(x, y, w, h, radius, bgColor);
  lcd.drawRoundRect(x, y, w, h, radius, borderColor);

  // Set font
  lcd.setFont(getFontFromType(font));

  // Draw button text
  lcd.setTextColor(textColor, bgColor);

  // Calculate text bounds for proper centering
  int16_t textWidth = lcd.textWidth(text);

  int textX = x + (w - textWidth) / 2;
  int textY = y + h / 2;

  lcd.setTextDatum(textdatum_t::middle_left);
  lcd.setCursor(textX, textY);
  lcd.print(text);

  // Reset font to default
  lcd.setFont(nullptr);
}

/**
 * Draw a button with a symbol instead of text
 */
void drawSymbolButton(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, int radius,
                      SymbolType symbol, bool pressed) {
  // Inverted colors when pressed
  uint16_t bgColor = pressed ? MAC_BLACK : MAC_WHITE;
  uint16_t borderColor = MAC_BLACK;
  uint16_t symbolColor = pressed ? MAC_WHITE : MAC_BLACK;

  lcd.fillRoundRect(x, y, w, h, radius, bgColor);
  lcd.drawRoundRect(x, y, w, h, radius, borderColor);

  // Draw the symbol centered in the button
  int symbolSize = min(w, h) - 32;
  int symbolX = x + (w - symbolSize) / 2;
  int symbolY = y + (h - symbolSize) / 2;

  drawSymbol(lcd, symbolX, symbolY, symbolSize, symbol, symbolColor);
}

MacComponent* createButtonComponent(int x, int y, int w, int h, int id, const String& text,
                                    SymbolType symbol) {
  MacComponent* component = createComponent(COMPONENT_BUTTON, x, y, w, h, id);

  // Create button-specific data
  MacButton* buttonData = new MacButton();
  buttonData->text = text;
  buttonData->symbol = symbol;
  buttonData->pressed = false;
  buttonData->radius = BUTTON_DEFAULT_RADIUS;
  buttonData->font = FONT_CHICAGO_11PT;

  component->customData = buttonData;
  return component;
}
