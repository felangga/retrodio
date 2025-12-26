/*
 * config.h - Configuration file for Retrodio
 * 
 * Copyright (c) 2025 Felangga
 * 
 * This file contains all configuration settings for the radio application.
 * Modify these values according to your setup.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ===== WiFi Configuration =====
#define WIFI_SSID "KENARI"
#define WIFI_PASSWORD "tamankenari"

// ===== Radio Stream Configuration =====
// swaragama  "http://202.65.114.229:9314/" 
#define RADIO_URL "https://ig.idstreamer.com:8090/live"

// ===== Audio Settings =====
#define DEFAULT_VOLUME 5  // Range: 0-21

// ===== NTP Configuration =====
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 0      // Adjust for your timezone (e.g., 25200 for GMT+7)
#define DST_OFFSET_SEC 0      // Daylight saving time offset

#endif // CONFIG_H
