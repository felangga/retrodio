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

#endif
