/*
 * config.h - Configuration file for Retrodio
 *
 * Copyright (c) 2025 felangga
 *
 */

#ifndef CONFIG_H
#define CONFIG_H

// ===== Audio Settings =====
#define DEFAULT_VOLUME 10  // Range: 0-21

// ===== NTP Configuration =====
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 25200  // Adjust for your timezone (e.g., 25200 for GMT+7)
#define DST_OFFSET_SEC 0      // Daylight saving time offset

// ===== Power Save Configuration =====
#define DEFAULT_LCD_IDLE_TIMEOUT 300000  // Default: 5 minutes (in milliseconds)
#define MIN_LCD_IDLE_TIMEOUT 0           // Minimum: 0 (disabled)
#define MAX_LCD_IDLE_TIMEOUT 1800000     // Maximum: 30 minutes (in milliseconds)
#define LCD_BACKLIGHT_PIN 45             // Backlight control pin
#define LCD_BACKLIGHT_PWM_CHANNEL 7      // PWM channel for backlight

#endif
