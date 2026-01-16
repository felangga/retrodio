/*
 * AudioHandlers.cpp - Audio System Management Implementation
 *
 * Copyright (c) 2025 felangga
 *
 * This file implements audio system functions
 */

#include "AudioHandlers.h"
#include "GlobalState.h"
#include "config.h"

#define ENABLE_SERIAL_DEBUG 0
#define ENABLE_DEBUG 0

#if ENABLE_SERIAL_DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#endif

void audio_showstation(const char* info) {
  extern SemaphoreHandle_t metadataMutex;
  extern StreamMetadata streamMetadata;

  if (info && strlen(info) > 0 && metadataMutex) {
    if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) {
      strncpy(streamMetadata.stationName, info, METADATA_BUFFER_SIZE - 1);
      streamMetadata.stationName[METADATA_BUFFER_SIZE - 1] = '\0';
      streamMetadata.received = true;
      xSemaphoreGive(metadataMutex);
    }
  }
}

void audio_showstreamtitle(const char* info) {
  extern SemaphoreHandle_t metadataMutex;
  extern StreamMetadata streamMetadata;

  if (info && strlen(info) > 0 && metadataMutex) {
    if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) {
      strncpy(streamMetadata.trackInfo, info, METADATA_BUFFER_SIZE - 1);
      streamMetadata.trackInfo[METADATA_BUFFER_SIZE - 1] = '\0';
      streamMetadata.received = true;
      xSemaphoreGive(metadataMutex);
    }
  }
}

void audio_id3data(const char* info) {
  extern SemaphoreHandle_t metadataMutex;
  extern StreamMetadata streamMetadata;

  if (info && strlen(info) > 0 && metadataMutex) {
    if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) {
      if (strlen(streamMetadata.trackInfo) == 0) {
        strncpy(streamMetadata.trackInfo, info, METADATA_BUFFER_SIZE - 1);
        streamMetadata.trackInfo[METADATA_BUFFER_SIZE - 1] = '\0';
        streamMetadata.received = true;
      }
      xSemaphoreGive(metadataMutex);
    }
  }
}

void audio_eof_stream(const char* info) {
  extern volatile bool isPlaying;
  isPlaying = false;
}

void audio_callback(Audio::msg_t m) {
  extern SemaphoreHandle_t metadataMutex;
  extern StreamMetadata streamMetadata;
  extern volatile bool isPlaying;

  switch (m.e) {
    case Audio::evt_name:
      if (m.msg && strlen(m.msg) > 0 && metadataMutex) {
        if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) {
          strncpy(streamMetadata.stationName, m.msg, METADATA_BUFFER_SIZE - 1);
          streamMetadata.stationName[METADATA_BUFFER_SIZE - 1] = '\0';
          streamMetadata.received = true;
          xSemaphoreGive(metadataMutex);
        }
      }
      break;

    case Audio::evt_streamtitle:
      if (m.msg && strlen(m.msg) > 0 && metadataMutex) {
        if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) {
          strncpy(streamMetadata.trackInfo, m.msg, METADATA_BUFFER_SIZE - 1);
          streamMetadata.trackInfo[METADATA_BUFFER_SIZE - 1] = '\0';
          streamMetadata.received = true;
          xSemaphoreGive(metadataMutex);
        }
      }
      break;

    case Audio::evt_icydescription:
      if (m.msg && strlen(m.msg) > 0 && metadataMutex) {
        if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) {
          strncpy(streamMetadata.description, m.msg, METADATA_BUFFER_SIZE - 1);
          streamMetadata.description[METADATA_BUFFER_SIZE - 1] = '\0';
          streamMetadata.received = true;
          xSemaphoreGive(metadataMutex);
        }
      }
      break;

    case Audio::evt_bitrate:
      if (m.msg && strlen(m.msg) > 0 && metadataMutex) {
        if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) {
          strncpy(streamMetadata.bitRate, m.msg, METADATA_BUFFER_SIZE - 1);
          streamMetadata.bitRate[METADATA_BUFFER_SIZE - 1] = '\0';
          streamMetadata.received = true;
          xSemaphoreGive(metadataMutex);
        }
      }
      break;

    case Audio::evt_id3data:
      if (m.msg && strlen(m.msg) > 0 && metadataMutex) {
        if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) {
          if (strlen(streamMetadata.id3data) == 0) {
            strncpy(streamMetadata.id3data, m.msg, METADATA_BUFFER_SIZE - 1);
            streamMetadata.id3data[METADATA_BUFFER_SIZE - 1] = '\0';
            streamMetadata.received = true;
          }
          xSemaphoreGive(metadataMutex);
        }
      }
      break;

    case Audio::evt_eof:
      isPlaying = false;
      break;

    case Audio::evt_info:
      if (m.msg && strlen(m.msg) > 0 && metadataMutex) {
        if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) {
          strncpy(streamMetadata.info, m.msg, METADATA_BUFFER_SIZE - 1);
          streamMetadata.info[METADATA_BUFFER_SIZE - 1] = '\0';
          streamMetadata.received = true;
          xSemaphoreGive(metadataMutex);
        }
      }
      break;

    case Audio::evt_lyrics:
      if (m.msg && strlen(m.msg) > 0 && metadataMutex) {
        if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) {
          strncpy(streamMetadata.lyrics, m.msg, METADATA_BUFFER_SIZE - 1);
          streamMetadata.lyrics[METADATA_BUFFER_SIZE - 1] = '\0';
          streamMetadata.received = true;
          xSemaphoreGive(metadataMutex);
        }
      }
      break;

    case Audio::evt_log:
      if (m.msg && strlen(m.msg) > 0 && metadataMutex) {
        if (xSemaphoreTake(metadataMutex, 0) == pdTRUE) {
          strncpy(streamMetadata.log, m.msg, METADATA_BUFFER_SIZE - 1);
          streamMetadata.log[METADATA_BUFFER_SIZE - 1] = '\0';
          streamMetadata.received = true;
          xSemaphoreGive(metadataMutex);
        }
      }
      break;

    default:
      break;
  }
}

void initializeAudio() {
  extern Audio audio;

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  delay(100);

  audio.setVolume(DEFAULT_VOLUME);
  audio.setConnectionTimeout(4000, 8000);

  Audio::audio_info_callback = audio_callback;
}

void audioTask(void* parameter) {
  extern Audio audio;
  extern QueueHandle_t audioCommandQueue;
  extern volatile bool isPlaying;

  vTaskDelay(pdMS_TO_TICKS(500));

  while (true) {
    AudioCommandMsg msg;
    if (xQueueReceive(audioCommandQueue, &msg, 0) == pdTRUE) {
      switch (msg.cmd) {
        case CMD_CONNECT:
          if (isPlaying && audio.isRunning()) {
            audio.stopSong();
            vTaskDelay(pdMS_TO_TICKS(200));
            isPlaying = false;
          }

          if (audio.connecttohost(msg.url)) {
            isPlaying = true;
          } else {
            isPlaying = false;
          }
          break;
        case CMD_STOP:
          if (audio.isRunning()) {
            audio.stopSong();
            vTaskDelay(pdMS_TO_TICKS(100));
            isPlaying = false;
          } else {
            isPlaying = false;
          }
          break;
        default:
          break;
      }
    }

    if (isPlaying) {
      audio.loop();
      vTaskDelay(1);
    } else {
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
  }
}
