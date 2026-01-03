/*
 * MacListView.cpp - ListView Component Implementation
 *
 * Copyright (c) 2025 felangga
 */

#include "MacUI.h"

void drawListView(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, MacListView& listView) {
  // Only draw frame on full redraw (not during scrolling)
  if (listView.needsFullRedraw) {
    draw3DFrame(lcd, x, y, w, h, true);
    listView.needsFullRedraw = false;
  }

  if (listView.items == nullptr || listView.itemCount == 0) {
    // Draw "No items" message
    lcd.fillRect(x + 2, y + 2, w - 4, h - 4, listView.backgroundColor);
    lcd.setTextColor(MAC_GRAY, listView.backgroundColor);
    lcd.setTextSize(1);
    lcd.setCursor(x + 10, y + h / 2);
    lcd.print("No items");
    return;
  }

  // Set clipping region to content area (exclude frame)
  int clipX = x + 2;
  int clipY = y + 2;
  int clipW = w - 4;
  int clipH = h - 4;
  lcd.setClipRect(clipX, clipY, clipW, clipH);

  // Calculate visible area
  int visibleHeight = h - 4;
  int maxVisibleItems = visibleHeight / listView.itemHeight;
  int startIndex = listView.scrollOffset / listView.itemHeight;
  int endIndex = min(startIndex + maxVisibleItems + 2, listView.itemCount);

  // Draw visible items directly to screen (sprite disabled to save RAM)
  for (int i = startIndex; i < endIndex; i++) {
    int itemY = y + 2 + (i * listView.itemHeight) - listView.scrollOffset;

    // Skip if item is completely outside visible area
    if (itemY + listView.itemHeight < y + 2 || itemY > y + h - 2) {
      continue;
    }

    // Determine colors based on selection
    uint16_t bgColor =
        (i == listView.selectedIndex) ? listView.selectedColor : listView.backgroundColor;
    uint16_t txtColor = (i == listView.selectedIndex) ? MAC_WHITE : listView.textColor;

    // Draw item background
    lcd.fillRect(x + 2, itemY, w - 4, listView.itemHeight, bgColor);

    // Draw item text
    lcd.setTextColor(txtColor, bgColor);
    lcd.setFont(getFontFromType(listView.font));
    lcd.setTextSize(listView.textSize);
    lcd.setCursor(x + 8, itemY + (listView.itemHeight - 8 * listView.textSize) / 2);

    // Truncate text if too long
    String displayText = listView.items[i].text;
    int maxChars = (w - 16) / (6 * listView.textSize);
    if (displayText.length() > maxChars) {
      displayText = displayText.substring(0, maxChars - 3) + "...";
    }
    lcd.print(displayText);

    // Reset font to default
    lcd.setFont(nullptr);

    // Draw separator line between items (except last visible item)
    if (i < listView.itemCount - 1) {
      lcd.drawFastHLine(x + 4, itemY + listView.itemHeight - 1, w - 8, MAC_GRAY);
    }
  }

  // Clear clipping before drawing scrollbar
  lcd.clearClipRect();

  // Draw scrollbar if needed
  if (listView.itemCount * listView.itemHeight > visibleHeight) {
    int scrollbarX = x + w - 10;
    int scrollbarY = y + 2;
    int scrollbarHeight = h - 4;

    // Calculate thumb size and position
    int totalContentHeight = listView.itemCount * listView.itemHeight;
    int thumbHeight = max(20, (scrollbarHeight * visibleHeight) / totalContentHeight);
    int maxScroll = max(0, totalContentHeight - visibleHeight);
    int thumbY =
        scrollbarY + ((listView.scrollOffset * (scrollbarHeight - thumbHeight)) / maxScroll);

    // Only redraw scrollbar if thumb position changed significantly (reduce flicker)
    if (abs(thumbY - listView.lastScrollbarThumbY) > 2 || listView.needsFullRedraw) {
      // Draw scrollbar track
      lcd.fillRect(scrollbarX, scrollbarY, 8, scrollbarHeight, MAC_LIGHT_GRAY);
      lcd.drawRect(scrollbarX, scrollbarY, 8, scrollbarHeight, MAC_DARK_GRAY);

      // Draw scrollbar thumb
      lcd.fillRect(scrollbarX + 1, thumbY, 6, thumbHeight, MAC_GRAY);
      lcd.drawRect(scrollbarX + 1, thumbY, 6, thumbHeight, MAC_BLACK);

      // Update last thumb position
      listView.lastScrollbarThumbY = thumbY;
    }
  }
}

MacComponent* createListViewComponent(int x, int y, int w, int h, int id, MacListViewItem* items,
                                      int itemCount, int itemHeight) {
  MacComponent* component = createComponent(COMPONENT_LISTVIEW, x, y, w, h, id);

  // Create listview-specific data
  MacListView* listViewData = new MacListView();
  listViewData->items = items;
  listViewData->itemCount = itemCount;
  listViewData->selectedIndex = -1;  // No selection initially
  listViewData->scrollOffset = 0;
  listViewData->itemHeight = itemHeight;
  listViewData->visibleItemCount = (h - 4) / itemHeight;
  listViewData->textColor = MAC_BLACK;
  listViewData->backgroundColor = MAC_WHITE;
  listViewData->selectedColor = MAC_BLUE;
  listViewData->textSize = 1;
  listViewData->font = FONT_DEFAULT;  // Default font
  listViewData->onItemClick = nullptr;
  // Initialize touch tracking
  listViewData->isTouching = false;
  listViewData->lastTouchY = 0;
  listViewData->touchStartY = 0;
  listViewData->touchStartTime = 0;
  listViewData->needsFullRedraw = true;  // First draw should be full
  listViewData->lastScrollbarThumbY = -1;  // Initialize to invalid position

  component->customData = listViewData;
  return component;
}
