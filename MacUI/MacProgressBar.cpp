/*
 * MacProgressBar.cpp - ProgressBar Component Implementation
 * 
 * Copyright (c) 2025 Felangga
 */

#include "MacUI.h"

void drawProgressBar(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const MacProgressBar& progressBar) {
  // Draw background
  lcd.fillRect(x, y, w, h, MAC_WHITE);
  draw3DFrame(lcd, x, y, w, h, true);
  
  // Calculate fill width
  int range = progressBar.maxValue - progressBar.minValue;
  int fillW = ((progressBar.currentValue - progressBar.minValue) * (w - 4)) / range;
  
  // Draw fill
  if (fillW > 0) {
    lcd.fillRect(x + 2, y + 2, fillW, h - 4, progressBar.fillColor);
  }
  
  // Draw percentage text if enabled
  if (progressBar.showPercentage) {
    int percentage = ((progressBar.currentValue - progressBar.minValue) * 100) / range;
    String percentText = String(percentage) + "%";
    
    lcd.setTextColor(MAC_BLACK, MAC_WHITE);
    lcd.setTextSize(1);
    int textX = x + (w - percentText.length() * 6) / 2;
    int textY = y + (h - 8) / 2;
    lcd.setCursor(textX, textY);
    lcd.print(percentText);
  }
}

MacComponent* createProgressBarComponent(int x, int y, int w, int h, int id, int minVal, int maxVal, int currentVal) {
  MacComponent* component = createComponent(COMPONENT_PROGRESS_BAR, x, y, w, h, id);
  
  // Create progress bar-specific data
  MacProgressBar* progressData = new MacProgressBar();
  progressData->minValue = minVal;
  progressData->maxValue = maxVal;
  progressData->currentValue = currentVal;
  progressData->fillColor = MAC_BLUE;
  progressData->showPercentage = true;
  
  component->customData = progressData;
  return component;
}
