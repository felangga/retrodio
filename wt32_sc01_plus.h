/*
 * wt32_sc01_plus.h - WT32-SC01 Plus Display Configuration
 *
 * Copyright (c) 2025 Felangga
 *
 * Hardware configuration and display setup for the WT32-SC01 Plus
 * development board with 3.5" capacitive touch screen.
 */

#ifndef WT32_SC01_PLUS_H
#define WT32_SC01_PLUS_H

// ESP32 Core compatibility fix
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
#define LGFX_ESP32_V3_COMPAT
#endif

#define LGFX_USE_V1

#include <Arduino.h>
#include <LovyanGFX.hpp>

/* Change to your screen resolution */
// Using inline to allow definitions in header without multiple definition errors
#ifdef PORTRAIT
inline const uint32_t screenWidth = 320;
inline const uint32_t screenHeight = 480;
#else
inline const uint32_t screenWidth = 480;
inline const uint32_t screenHeight = 320;
#endif

// I2S Audio pins
#define I2S_DOUT 37
#define I2S_BCLK 36
#define I2S_LRC 35

/* I2S Pins */
#define I2S_BCK 36
#define I2S_WS 35
#define I2S_DATA 37

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7796 _panel_instance;  // ST7796UI
  lgfx::Bus_Parallel8 _bus_instance;   // MCU8080 8B
  lgfx::Light_PWM _light_instance;
  lgfx::Touch_FT5x06 _touch_instance;

 public:
  LGFX(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.freq_write = 40000000;
      cfg.pin_wr = 47;
      cfg.pin_rd = -1;
      cfg.pin_rs = 0;

      // LCD data interface, 8bit MCU (8080)
      cfg.pin_d0 = 9;
      cfg.pin_d1 = 46;
      cfg.pin_d2 = 3;
      cfg.pin_d3 = 8;
      cfg.pin_d4 = 18;
      cfg.pin_d5 = 17;
      cfg.pin_d6 = 16;
      cfg.pin_d7 = 15;

      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    {
      auto cfg = _panel_instance.config();

      cfg.pin_cs = -1;
      cfg.pin_rst = 4;
      cfg.pin_busy = -1;

      cfg.panel_width = 320;
      cfg.panel_height = 480;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = false;
      cfg.invert = true;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;

      _panel_instance.config(cfg);
    }

    {
      auto cfg = _light_instance.config();

      cfg.pin_bl = 45;
      cfg.invert = false;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    {
      auto cfg = _touch_instance.config();

      cfg.x_min = 0;
      cfg.x_max = 319;
      cfg.y_min = 0;
      cfg.y_max = 479;
      cfg.pin_int = 7;
      cfg.bus_shared = true;
      cfg.offset_rotation = 0;

      cfg.i2c_port = 1;  // I2C_NUM_1;
      cfg.i2c_addr = 0x38;
      cfg.pin_sda = 6;
      cfg.pin_scl = 5;
      cfg.freq = 400000;

      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }

    setPanel(&_panel_instance);
  }
};

// Create an instance of the prepared class.
inline LGFX tft;

#endif  // WT32_SC01_PLUS_H