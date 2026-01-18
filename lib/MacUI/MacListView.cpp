/*
 * MacListView.cpp - ListView Component Implementation
 *
 * Copyright (c) 2025 felangga
 */

#include "MacUI.h"

void drawListView(lgfx::LGFX_Device& lcd, int x, int y, int w, int h, MacListView& listView) {
  if (listView.items == nullptr || listView.itemCount == 0) {
    draw3DFrame(lcd, x, y, w, h);
    lcd.fillRect(x + 2, y + 2, w - 4, h - 4, listView.backgroundColor);
    lcd.setTextColor(MAC_GRAY, listView.backgroundColor);
    lcd.setTextSize(1);
    lcd.setCursor(x + 10, y + h / 2);
    lcd.print("No items");
    listView.needsFullRedraw = false;
    return;
  }

  if (listView.needsFullRedraw) {
    draw3DFrame(lcd, x, y, w, h);
    lcd.fillRect(x + 2, y + 2, w - 4, h - 4, listView.backgroundColor);
    listView.needsFullRedraw = false;
  }

  int scrollbarWidth = 10;
  int clipX = x + 2;
  int clipY = y + 2;
  int clipW = w - 4 - scrollbarWidth;
  int clipH = h - 4;
  lcd.setClipRect(clipX, clipY, clipW, clipH);

  int visibleHeight = h - 4;
  int maxVisibleItems = visibleHeight / listView.itemHeight;
  int startIndex = listView.scrollOffset / listView.itemHeight;
  int endIndex = min(startIndex + maxVisibleItems + 2, listView.itemCount);

  for (int i = startIndex; i < endIndex; i++) {
    int itemY = y + 2 + (i * listView.itemHeight) - listView.scrollOffset;

    if (itemY + listView.itemHeight < y + 2 || itemY > y + h - 2) {
      continue;
    }

    uint16_t bgColor =
        (i == listView.selectedIndex) ? listView.selectedColor : listView.backgroundColor;
    uint16_t txtColor = (i == listView.selectedIndex) ? MAC_WHITE : listView.textColor;
    // Draw item background excluding scrollbar area
    lcd.fillRect(x + 2, itemY, w - 4 - scrollbarWidth, listView.itemHeight, bgColor);

    lcd.setTextColor(txtColor, bgColor);
    lcd.setFont(getFontFromType(listView.font));
    lcd.setTextSize(listView.textSize);
    lcd.setCursor(x + 8, itemY + (listView.itemHeight * listView.textSize) / 2);

    String displayText = listView.items[i].text;
    int maxChars = (w - 16) / (6 * listView.textSize);
    if (displayText.length() > maxChars) {
      displayText = displayText.substring(0, maxChars - 3) + "...";
    }
    lcd.print(displayText);

    lcd.setFont(nullptr);
  }

  lcd.clearClipRect();

  int scrollbarX = x + w - 10;
  int scrollbarY = y + 2;
  int scrollbarHeight = h - 4;

  int totalContentHeight = listView.itemCount * listView.itemHeight;
  bool needsScrolling = totalContentHeight > visibleHeight;

  int thumbHeight;
  int thumbY;

  if (needsScrolling) {
    thumbHeight = max(20, (scrollbarHeight * visibleHeight) / totalContentHeight);
    int maxScroll = max(0, totalContentHeight - visibleHeight);
    thumbY = scrollbarY + ((listView.scrollOffset * (scrollbarHeight - thumbHeight)) / maxScroll);
  } else {
    thumbHeight = scrollbarHeight;
    thumbY = scrollbarY;
  }

  if (abs(thumbY - listView.lastScrollbarThumbY) > 2 || listView.needsFullRedraw) {
    lcd.fillRect(scrollbarX, scrollbarY, 8, scrollbarHeight, MAC_LIGHT_GRAY);
    lcd.drawRect(scrollbarX, scrollbarY, 8, scrollbarHeight, MAC_DARK_GRAY);

    lcd.fillRect(scrollbarX + 1, thumbY, 6, thumbHeight, MAC_GRAY);
    lcd.drawRect(scrollbarX + 1, thumbY, 6, thumbHeight, MAC_BLACK);

    listView.lastScrollbarThumbY = thumbY;
  }
}

MacComponent* createListViewComponent(int x, int y, int w, int h, int id, MacListViewItem* items,
                                      int itemCount, int itemHeight) {
  MacComponent* component = createComponent(COMPONENT_LISTVIEW, x, y, w, h, id);

  MacListView* listViewData = new MacListView();
  listViewData->items = items;
  listViewData->itemCount = itemCount;
  listViewData->selectedIndex = -1;
  listViewData->scrollOffset = 0;
  listViewData->itemHeight = itemHeight;
  listViewData->visibleItemCount = (h - 4) / itemHeight;
  listViewData->textColor = MAC_BLACK;
  listViewData->backgroundColor = MAC_WHITE;
  listViewData->selectedColor = MAC_BLACK;
  listViewData->textSize = 1;
  listViewData->font = FONT_DEFAULT;
  listViewData->onItemClick = nullptr;
  listViewData->isTouching = false;
  listViewData->lastTouchY = 0;
  listViewData->touchStartY = 0;
  listViewData->touchStartTime = 0;
  listViewData->needsFullRedraw = true;
  listViewData->lastScrollbarThumbY = -1;

  component->customData = listViewData;
  return component;
}
