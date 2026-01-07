/*
 * AudioHandlers.h - Audio System Management
 *
 * Copyright (c) 2025 felangga
 *
 * This header file contains declarations for audio system functions
 */

#ifndef AUDIO_HANDLERS_H
#define AUDIO_HANDLERS_H

#include "Audio.h"

// Audio initialization and management
void initializeAudio();
void connectToWiFi();

// Audio callback functions (ESP32-audioI2S v3.0.12+ API)
void audio_showstation(const char* info);
void audio_showstreamtitle(const char* info);
void audio_id3data(const char* info);
void audio_eof_stream(const char* info);
void audio_callback(Audio::msg_t m);

// Audio task
void audioTask(void* parameter);

#endif
