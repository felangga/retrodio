/*
 * RadioWindow.cpp - Radio Window Implementation
 *
 * Copyright (c) 2025 felangga
 *
 * This file implements radio window initialization functions
 */

#include "RadioWindow.h"
#include "GlobalState.h"
#include "UIHelpers.h"

void onComponentClick(int id, void* data);

void initializeRadioWindow() {
  extern String currentStationName;
  extern const int BTN_STATION;

  clearChildComponents(radioWindow);

  MacComponent* btnPrev = createButtonComponent(30, 165, 50, 50, 4, "", SYMBOL_PREV);
  btnPrev->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(radioWindow, btnPrev);

  MacComponent* btnPlay = createButtonComponent(80, 160, 60, 60, 1, "", SYMBOL_PLAY);
  btnPlay->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(radioWindow, btnPlay);

  MacComponent* btnStation = createButtonComponent(350, 165, 50, 50, BTN_STATION, "", SYMBOL_LIST);
  btnStation->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(radioWindow, btnStation);

  MacComponent* btnNext = createButtonComponent(140, 165, 50, 50, 5, "", SYMBOL_NEXT);
  btnNext->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(radioWindow, btnNext);

  extern const int TXT_RADIO_NAME;
  extern const int TXT_RADIO_DETAILS;
  extern const int TXT_BITRATE;
  extern const int TXT_ID3;
  extern const int TXT_INFO;
  extern const int TXT_DESCRIPTION;
  extern const int TXT_CPU_LABEL;

  MacComponent* txtRadioName = createRunningTextComponent(20, 45, 380, 25, TXT_RADIO_NAME,
                                                          currentStationName, 2, MAC_BLACK, 3);
  txtRadioName->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(radioWindow, txtRadioName);

  MacComponent* txtRadioDetails = createRunningTextComponent(20, 75, 200, 20, TXT_RADIO_DETAILS,
                                                             "Standby waiting for metadata ...", 2, MAC_BLACK, 1);
  txtRadioDetails->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(radioWindow, txtRadioDetails);

  MacComponent* txtBitRate = createRunningTextComponent(20, 92, 200, 20, TXT_BITRATE,
                                                        "Bitrate: N/A", 2, MAC_BLACK, 1);
  addChildComponent(radioWindow, txtBitRate);

  MacComponent* txtID3 = createRunningTextComponent(20, 109, 200, 20, TXT_ID3,
                                                    "ID3: N/A", 2, MAC_BLACK, 1);
  addChildComponent(radioWindow, txtID3);

  MacComponent* txtInfo = createRunningTextComponent(20, 126, 200, 20, TXT_INFO,
                                                     "", 2, MAC_BLACK, 1);
  addChildComponent(radioWindow, txtInfo);

  MacComponent* txtDescription = createRunningTextComponent(20, 143, 200, 20, TXT_DESCRIPTION,
                                                            "", 2, MAC_BLACK, 1);
  addChildComponent(radioWindow, txtDescription);

  #if ENABLE_DEBUG
  MacComponent* cpuLabel = createLabelComponent(0, 6, 200, 15, TXT_CPU_LABEL, "CPU0: 0% CPU1: 0%", MAC_BLACK);
  cpuLabel->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(radioWindow, cpuLabel);
  #endif

  MacComponent* btnVolUp = createButtonComponent(200, 165, 50, 50, 3, "", SYMBOL_VOL_UP);
  btnVolUp->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(radioWindow, btnVolUp);

  MacComponent* btnVolDn = createButtonComponent(250, 165, 50, 50, 6, "", SYMBOL_VOL_DOWN);
  btnVolDn->onClick = [](int componentId) { onComponentClick(componentId, nullptr); };
  addChildComponent(radioWindow, btnVolDn);
}
