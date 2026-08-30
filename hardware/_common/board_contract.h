/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Module: hardware/_common/board_contract.h
 * Description: Compile-time contract check validating required display symbols based on WITH_DISPLAY_* flags.
 */

#pragma once

#include "board_pins.h"

// ── WebServer Configuration Fallbacks ───────────────────────────────────────
#ifndef BOARD_HTTP_STACK_SIZE
#define BOARD_HTTP_STACK_SIZE 8192          // safe for all non-screenshot routes (compression is offloaded, WO2)
#endif

#ifndef BOARD_HTTP_MAX_OPEN_SOCKETS
#define BOARD_HTTP_MAX_OPEN_SOCKETS 4       // browser (2-3) + MCP (1); boards with more DRAM can override upward
#endif

// ── Audit Budget Fallback ─────────────────────────────────────────────────
#ifndef BOARD_AUDIT_BYTES
#define BOARD_AUDIT_BYTES 32768              // 32 KiB default; boards override in board.h
#endif


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

#ifndef PIN_DISPLAY_RST
#error "WITH_DISPLAY_U8G2 set but PIN_DISPLAY_RST undefined (use PIN_UNASSIGNED if not wired)"
#endif

// Compile-time assertion that mandatory pin symbols are valid symbols
static_assert(PIN_DISPLAY_CS >= -1 && PIN_DISPLAY_CS <= 48, "PIN_DISPLAY_CS out of valid GPIO range");
static_assert(PIN_DISPLAY_DC >= -1 && PIN_DISPLAY_DC <= 48, "PIN_DISPLAY_DC out of valid GPIO range");
static_assert(PIN_DISPLAY_SCK >= -1 && PIN_DISPLAY_SCK <= 48, "PIN_DISPLAY_SCK out of valid GPIO range");
static_assert(PIN_DISPLAY_MISO >= -1 && PIN_DISPLAY_MISO <= 48, "PIN_DISPLAY_MISO out of valid GPIO range");
static_assert(PIN_DISPLAY_MOSI >= -1 && PIN_DISPLAY_MOSI <= 48, "PIN_DISPLAY_MOSI out of valid GPIO range");
static_assert(PIN_DISPLAY_RST >= -1 && PIN_DISPLAY_RST <= 48, "PIN_DISPLAY_RST out of valid GPIO range");

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

static_assert(PIN_LCD_SPI_SCLK >= -1 && PIN_LCD_SPI_SCLK <= 48, "PIN_LCD_SPI_SCLK out of valid GPIO range");
static_assert(PIN_LCD_CS >= -1 && PIN_LCD_CS <= 48, "PIN_LCD_CS out of valid GPIO range");
static_assert(PIN_TOUCH_SDA >= -1 && PIN_TOUCH_SDA <= 48, "PIN_TOUCH_SDA out of valid GPIO range");
static_assert(PIN_ENCODER_A >= -1 && PIN_ENCODER_A <= 48, "PIN_ENCODER_A out of valid GPIO range");

#endif // WITH_DISPLAY_LVGL


// ── Button Contract Verification ──────────────────────────────────────────────
#ifdef WITH_BUTTONS

static_assert(BOARD_BUTTON_COUNT > 0, "WITH_BUTTONS set but BOARD_BUTTONS[] is empty");

#endif // WITH_BUTTONS


// ── Encoder Contract Verification ──────────────────────────────────────────────
#ifdef WITH_ENCODER

static_assert(BOARD_ENCODER_COUNT > 0, "WITH_ENCODER set but BOARD_ENCODERS[] is empty");

#if __has_include("soc/soc_caps.h")
#include "soc/soc_caps.h"
#ifdef SOC_PCNT_UNITS_PER_GROUP
static_assert(BOARD_ENCODER_COUNT <= SOC_PCNT_UNITS_PER_GROUP, "BOARD_ENCODER_COUNT exceeds available hardware PCNT units");
#endif
#endif

#endif // WITH_ENCODER



