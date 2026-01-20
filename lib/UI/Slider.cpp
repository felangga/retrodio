/*
 * Slider.cpp - Slider Component Implementation
 *
 * Copyright (c) 2025 felangga
 */

#include "UI.h"

#define SLIDER_THUMB_SIZE 30
#define SLIDER_TRACK_WIDTH 8

void drawSlider(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, const UISlider& slider) {
  lcd.fillRect(x, y, w + (SLIDER_THUMB_SIZE / 2), h, UI_WHITE);

  if (slider.vertical) {
    int trackX = x + w / 2 - (SLIDER_TRACK_WIDTH / 2);
    int trackH = h - (SLIDER_THUMB_SIZE / 2);
    int trackY = y + (SLIDER_THUMB_SIZE / 4);

    // Draw track
    lcd.fillRect(trackX, trackY, SLIDER_TRACK_WIDTH, trackH, UI_GRAY);

    // Calculate thumb position
    int range = slider.maxValue - slider.minValue;
    int thumbY = trackY + trackH - ((slider.currentValue - slider.minValue) * trackH / range) -
                 (SLIDER_THUMB_SIZE / 2);

    // Draw thumb
    lcd.fillRect(trackX - (SLIDER_THUMB_SIZE / 2), thumbY, SLIDER_THUMB_SIZE, SLIDER_THUMB_SIZE,
                 UI_WHITE);
  } else {
    int trackY = y + h / 2;
    int trackW = w - (SLIDER_THUMB_SIZE / 2);
    int trackX = x + (SLIDER_THUMB_SIZE / 2);

    // Draw track
    lcd.fillRect(trackX, trackY, trackW, SLIDER_TRACK_WIDTH, UI_GRAY);

    // Calculate thumb position
    int range = slider.maxValue - slider.minValue;
    int thumbX = trackX + ((slider.currentValue - slider.minValue) * trackW / range) -
                 (SLIDER_THUMB_SIZE / 2);

    lcd.fillRoundRect(thumbX, trackY - (SLIDER_THUMB_SIZE / 2) + (SLIDER_TRACK_WIDTH / 2),
                      SLIDER_THUMB_SIZE, SLIDER_THUMB_SIZE, 5, UI_WHITE);
    lcd.drawRoundRect(thumbX, trackY - (SLIDER_THUMB_SIZE / 2) + (SLIDER_TRACK_WIDTH / 2),
                      SLIDER_THUMB_SIZE, SLIDER_THUMB_SIZE, 5, UI_BLACK);
  }
}

UIComponent* createSliderComponent(int x, int y, int w, int h, int id, int minVal, int maxVal,
                                   int currentVal, bool vertical) {
  UIComponent* component = createComponent(COMPONENT_SLIDER, x, y, w, h, id);

  // Create slider-specific data
  UISlider* sliderData = new UISlider();
  sliderData->minValue = minVal;
  sliderData->maxValue = maxVal;
  sliderData->currentValue = currentVal;
  sliderData->vertical = vertical;

  component->customData = sliderData;
  return component;
}
