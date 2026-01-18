/*
 * MacRunningText.cpp - Running Text Component Implementation
 *
 * Copyright (c) 2025 felangga
 */

#include "MacUI.h"

void drawRunningText(lgfx::LGFX_Device& lcd, int x, int y, int w, int h,
                     MacRunningText& runningText) {
  // Use sprite buffer if available for flicker-free updates
  bool useSprite = (componentSprite != nullptr && w <= componentSprite->width() &&
                    h <= componentSprite->height());

  if (useSprite) {
    // Draw to sprite buffer first
    componentSprite->fillRect(0, 0, w, h, runningText.backgroundColor);
  } else {
    // Fall back to direct drawing
    lcd.fillRect(x, y, w, h, runningText.backgroundColor);
  }

  // Calculate text width in pixels based on text size
  int contentW = w - 4;
  int textWidth = runningText.text.length() * 6 * runningText.textSize;

  // Update scroll position if scrolling is enabled
  if (runningText.scrollEnabled) {
    unsigned long currentTime = millis();

    // Check if text fits completely within the content area
    if (textWidth <= contentW) {
      // Text fits completely, no need to scroll - just keep it centered
      runningText.scrollOffset = 0;
    } else {
      // Check if we're in pause state
      if (runningText.isPaused) {
        if (currentTime - runningText.pauseStartTime >= runningText.pauseDuration) {
          // Pause is over, resume scrolling
          runningText.isPaused = false;
          runningText.lastUpdate = currentTime;
        }
      } else {
        // Normal scrolling
        if (currentTime - runningText.lastUpdate >= runningText.updateInterval) {
          runningText.scrollOffset += runningText.scrollSpeed;
          runningText.lastUpdate = currentTime;

          // Check scroll boundaries and pause when text completes one cycle
          if (runningText.scrollSpeed > 0) {
            // Scrolling left (text moving right to left)
            if (runningText.scrollOffset > textWidth + 20) {
              // Text has scrolled completely off screen, reset and pause
              runningText.scrollOffset = 0;
              runningText.isPaused = true;
              runningText.pauseStartTime = currentTime;
            }
          } else if (runningText.scrollSpeed < 0) {
            // Scrolling right (text moving left to right)
            if (runningText.scrollOffset < -(contentW + 20)) {
              // Text has scrolled completely off screen, reset and pause
              runningText.scrollOffset = 0;
              runningText.isPaused = true;
              runningText.pauseStartTime = currentTime;
            }
          }
        }
      }
    }
  }

  // Set up drawing coordinates
  int contentX = useSprite ? 0 : x;
  int contentY = useSprite ? 0 : y;
  int contentH = h;

  lgfx::LGFX_Device* drawTarget = useSprite ? (lgfx::LGFX_Device*)componentSprite : &lcd;

  // Set text properties
  drawTarget->setTextColor(runningText.textColor, runningText.backgroundColor);
  drawTarget->setFont(getFontFromType(runningText.font));
  drawTarget->setTextSize(runningText.textSize);
  drawTarget->setTextWrap(false);
  drawTarget->setTextDatum(lgfx::textdatum_t::middle_left);

  // Calculate text position
  int textX = contentX + (runningText.scrollEnabled ? -runningText.scrollOffset : 0);
  int textY = contentY + contentH / 2;

  // Draw the text (it will be clipped by the component area)
  drawTarget->setClipRect(contentX, contentY, w, contentH);
  drawTarget->setCursor(textX, textY);
  drawTarget->print(runningText.text);

  // If scrolling and text is wrapping around, draw it again on the other side
  // Only draw wrap-around if actively scrolling (not paused and offset is significant)
  if (runningText.scrollEnabled && !runningText.isPaused && runningText.scrollOffset != 0) {
    if (runningText.scrollSpeed > 0 && runningText.scrollOffset > contentW / 4) {
      // Draw text coming in from the right (only when first text is halfway scrolled out)
      int wrapTextX = textX + textWidth + 20;  // 20 pixel gap between repeats
      drawTarget->setCursor(wrapTextX, textY);
      drawTarget->print(runningText.text);
    } else if (runningText.scrollSpeed < 0 && runningText.scrollOffset < -(contentW / 2)) {
      // Draw text coming in from the left (only when first text is halfway scrolled out)
      int wrapTextX = textX - textWidth - 20;  // 20 pixel gap between repeats
      drawTarget->setCursor(wrapTextX, textY);
      drawTarget->print(runningText.text);
    }
  }

  // Clear clipping
  drawTarget->clearClipRect();

  // Reset font to default
  drawTarget->setFont(nullptr);

  // Push sprite buffer to screen if used
  if (useSprite) {
    // Set clipping on the main lcd to prevent overflow
    lcd.setClipRect(x, y, w, h);
    componentSprite->pushSprite(&lcd, x, y);
    lcd.clearClipRect();
  }
}

MacComponent* createRunningTextComponent(int x, int y, int w, int h, int id, const String& text,
                                         int scrollSpeed, uint16_t textColor, int textSize,
                                         FontType font) {
  MacComponent* component = createComponent(COMPONENT_RUNNING_TEXT, x, y, w, h, id);

  MacRunningText* runningTextData = new MacRunningText();
  runningTextData->text = text;
  runningTextData->textColor = textColor;
  runningTextData->backgroundColor = MAC_WHITE;
  runningTextData->textSize = textSize;
  runningTextData->scrollOffset = 0;
  runningTextData->scrollSpeed = scrollSpeed;
  runningTextData->lastUpdate = millis();
  runningTextData->updateInterval = 50;
  runningTextData->scrollEnabled = true;
  runningTextData->isPaused = true;
  runningTextData->pauseStartTime = millis();
  runningTextData->pauseDuration = 2000;
  runningTextData->font = font;

  component->customData = runningTextData;
  return component;
}

void updateRunningTextProperties(MacComponent* component, const String* newText, int* newTextSize,
                                 uint16_t* newTextColor, uint16_t* newBgColor, int* newScrollSpeed,
                                 int* newPauseDuration, FontType* newFont) {
  if (component == nullptr || component->type != COMPONENT_RUNNING_TEXT ||
      component->customData == nullptr) {
    return;
  }

  MacRunningText* runningTextData = (MacRunningText*)component->customData;

  if (newText != nullptr) {
    runningTextData->text = *newText;
    runningTextData->scrollOffset = 0;
    runningTextData->isPaused = true;
    runningTextData->pauseStartTime = millis();
  }

  if (newTextSize != nullptr) {
    runningTextData->textSize = *newTextSize;
  }

  if (newTextColor != nullptr) {
    runningTextData->textColor = *newTextColor;
  }

  if (newBgColor != nullptr) {
    runningTextData->backgroundColor = *newBgColor;
  }

  if (newScrollSpeed != nullptr) {
    runningTextData->scrollSpeed = *newScrollSpeed;
  }

  if (newPauseDuration != nullptr) {
    runningTextData->pauseDuration = *newPauseDuration;
  }

  if (newFont != nullptr) {
    runningTextData->font = *newFont;
  }
}

void updateRunningTextComponents(lgfx::LGFX_Device& lcd, MacWindow& window) {
  if (!window.visible || window.childComponents == nullptr || window.childComponentCount == 0) {
    return;
  }

  // Skip updates if window is being dragged to prevent flicker
  if (window.isDragging) {
    return;
  }

  for (int i = 0; i < window.childComponentCount; i++) {
    MacComponent* component = window.childComponents[i];
    if (component != nullptr && component->visible && component->type == COMPONENT_RUNNING_TEXT) {
      if (component->customData != nullptr) {
        MacRunningText* runningTextData = (MacRunningText*)component->customData;

        // Check if it's time to update
        unsigned long currentTime = millis();
        if (runningTextData->scrollEnabled &&
            currentTime - runningTextData->lastUpdate >= runningTextData->updateInterval) {
          // Redraw this component to animate the scroll
          drawComponent(lcd, *component, window.x, window.y);
        }
      }
    }
  }
}
