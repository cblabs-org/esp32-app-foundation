/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Module: hardware/_common/board_contract.h
 * Description: Compile-time contract check validating required display symbols based on WITH_DISPLAY_* flags.
 */

#pragma once

#include "board_pins.h"

// ── U8G2 Display Contract Verification ───────────────────────────────────────
#ifdef WITH_DISPLAY_U8G2

#ifndef PIN_DISPLAY_CS
#error "WITH_DISPLAY_U8G2 set but PIN_DISPLAY_CS undefined"
#endif

#ifndef PIN_DISPLAY_DC
#error "WITH_DISPLAY_U8G2 set but PIN_DISPLAY_DC undefined"
#endif

#ifndef PIN_DISPLAY_SCK
#error "WITH_DISPLAY_U8G2 set but PIN_DISPLAY_SCK undefined"
#endif

#ifndef PIN_DISPLAY_MISO
#error "WITH_DISPLAY_U8G2 set but PIN_DISPLAY_MISO undefined"
#endif

#ifndef PIN_DISPLAY_MOSI
#error "WITH_DISPLAY_U8G2 set but PIN_DISPLAY_MOSI undefined"
#endif

#ifndef U8G2_DISPLAY_TYPE
#error "WITH_DISPLAY_U8G2 set but U8G2_DISPLAY_TYPE undefined"
#endif

// Compile-time assertion that mandatory pin symbols are valid symbols
static_assert(PIN_DISPLAY_CS == PIN_DISPLAY_CS, "PIN_DISPLAY_CS contract assertion");
static_assert(PIN_DISPLAY_DC == PIN_DISPLAY_DC, "PIN_DISPLAY_DC contract assertion");
static_assert(PIN_DISPLAY_SCK == PIN_DISPLAY_SCK, "PIN_DISPLAY_SCK contract assertion");
static_assert(PIN_DISPLAY_MISO == PIN_DISPLAY_MISO, "PIN_DISPLAY_MISO contract assertion");
static_assert(PIN_DISPLAY_MOSI == PIN_DISPLAY_MOSI, "PIN_DISPLAY_MOSI contract assertion");

#endif // WITH_DISPLAY_U8G2


// ── LVGL Display Contract Verification ───────────────────────────────────────
#ifdef WITH_DISPLAY_LVGL

#ifndef PIN_LCD_SPI_SCLK
#error "WITH_DISPLAY_LVGL set but PIN_LCD_SPI_SCLK undefined"
#endif

#ifndef PIN_LCD_SPI_MOSI
#error "WITH_DISPLAY_LVGL set but PIN_LCD_SPI_MOSI undefined"
#endif

#ifndef PIN_LCD_CS
#error "WITH_DISPLAY_LVGL set but PIN_LCD_CS undefined"
#endif

#ifndef PIN_LCD_DC
#error "WITH_DISPLAY_LVGL set but PIN_LCD_DC undefined"
#endif

#ifndef PIN_LCD_RST
#error "WITH_DISPLAY_LVGL set but PIN_LCD_RST undefined"
#endif

#ifndef PIN_LCD_BL
#error "WITH_DISPLAY_LVGL set but PIN_LCD_BL undefined"
#endif

#ifndef PIN_TOUCH_SDA
#error "WITH_DISPLAY_LVGL set but PIN_TOUCH_SDA undefined"
#endif

#ifndef PIN_TOUCH_SCL
#error "WITH_DISPLAY_LVGL set but PIN_TOUCH_SCL undefined"
#endif

#ifndef PIN_TOUCH_INT
#error "WITH_DISPLAY_LVGL set but PIN_TOUCH_INT undefined"
#endif

#ifndef PIN_TOUCH_RST
#error "WITH_DISPLAY_LVGL set but PIN_TOUCH_RST undefined"
#endif

#ifndef PIN_ENCODER_A
#error "WITH_DISPLAY_LVGL set but PIN_ENCODER_A undefined"
#endif

#ifndef PIN_ENCODER_B
#error "WITH_DISPLAY_LVGL set but PIN_ENCODER_B undefined"
#endif

#ifndef PIN_ENCODER_SW
#error "WITH_DISPLAY_LVGL set but PIN_ENCODER_SW undefined"
#endif

#ifndef PIN_AUX_I2C_SDA
#error "WITH_DISPLAY_LVGL set but PIN_AUX_I2C_SDA undefined"
#endif

#ifndef PIN_AUX_I2C_SCL
#error "WITH_DISPLAY_LVGL set but PIN_AUX_I2C_SCL undefined"
#endif

static_assert(PIN_LCD_SPI_SCLK == PIN_LCD_SPI_SCLK, "PIN_LCD_SPI_SCLK contract assertion");
static_assert(PIN_LCD_CS == PIN_LCD_CS, "PIN_LCD_CS contract assertion");
static_assert(PIN_TOUCH_SDA == PIN_TOUCH_SDA, "PIN_TOUCH_SDA contract assertion");
static_assert(PIN_ENCODER_A == PIN_ENCODER_A, "PIN_ENCODER_A contract assertion");

#endif // WITH_DISPLAY_LVGL
