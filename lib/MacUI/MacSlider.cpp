/*
 * MacSlider.cpp - Slider Component Implementation
 *
 * Copyright (c) 2025 felangga
 */

#include "MacUI.h"

void drawSlider(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const MacSlider& slider) {
  if (slider.vertical) {
    // Vertical slider
    int trackX = x + w / 2 - 2;
    int trackW = 4;
    int trackH = h - 20;
    int trackY = y + 10;

    // Draw track
    lcd.fillRect(trackX, trackY, trackW, trackH, MAC_LIGHT_GRAY);
    draw3DFrame(lcd, trackX, trackY, trackW, trackH);

    // Calculate thumb position
    int range = slider.maxValue - slider.minValue;
    int thumbY = trackY + trackH - ((slider.currentValue - slider.minValue) * trackH / range) - 5;

    // Draw thumb
    lcd.fillRect(trackX - 3, thumbY, trackW + 6, 10, MAC_WHITE);
    draw3DFrame(lcd, trackX - 3, thumbY, trackW + 6, 10);
  } else {
    // Horizontal slider
    int trackY = y + h / 2 - 2;
    int trackH = 4;
    int trackW = w - 20;
    int trackX = x + 10;

    // Draw track
    lcd.fillRect(trackX, trackY, trackW, trackH, MAC_LIGHT_GRAY);
    draw3DFrame(lcd, trackX, trackY, trackW, trackH);

    // Calculate thumb position
    int range = slider.maxValue - slider.minValue;
    int thumbX = trackX + ((slider.currentValue - slider.minValue) * trackW / range) - 5;

    // Draw thumb
    lcd.fillRect(thumbX, trackY - 3, 10, trackH + 6, MAC_WHITE);
    draw3DFrame(lcd, thumbX, trackY - 3, 10, trackH + 6);
  }
}

MacComponent* createSliderComponent(int x, int y, int w, int h, int id, int minVal, int maxVal,
                                    int currentVal, bool vertical) {
  MacComponent* component = createComponent(COMPONENT_SLIDER, x, y, w, h, id);

  // Create slider-specific data
  MacSlider* sliderData = new MacSlider();
  sliderData->minValue = minVal;
  sliderData->maxValue = maxVal;
  sliderData->currentValue = currentVal;
  sliderData->vertical = vertical;

  component->customData = sliderData;
  return component;
}
