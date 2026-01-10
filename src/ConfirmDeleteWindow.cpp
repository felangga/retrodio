/*
 * ConfirmDeleteWindow.cpp - Confirm Delete Window Implementation
 *
 * Copyright (c) 2025 felangga
 *
 * This file implements confirm delete window initialization functions
 */

#include "ConfirmDeleteWindow.h"
#include "GlobalState.h"

void onComponentClick(int id, void* data);

void initializeConfirmDeleteWindow() {
  extern const int BTN_CONFIRM_YES;
  extern const int BTN_CONFIRM_NO;

  clearChildComponents(confirmDeleteWindow);

  MacComponent* lblMessage = createLabelComponent(20, 45, 240, 20, 500, "Delete this station?");
  if (lblMessage && lblMessage->customData) {
    MacLabel* labelData = (MacLabel*)lblMessage->customData;
    labelData->font = FONT_CHICAGO_9PT;
  }
  addChildComponent(confirmDeleteWindow, lblMessage);

  MacComponent* btnYes = createButtonComponent(50, 75, 80, 30, BTN_CONFIRM_YES, "Yes");
  btnYes->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(confirmDeleteWindow, btnYes);

  MacComponent* btnNo = createButtonComponent(150, 75, 80, 30, BTN_CONFIRM_NO, "No");
  btnNo->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(confirmDeleteWindow, btnNo);
}
