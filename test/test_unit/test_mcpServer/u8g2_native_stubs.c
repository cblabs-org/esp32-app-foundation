/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Module: test/test_unit/test_u8g2Display/u8g2_native_stubs.c
 * Description: Stub implementations of U8g2 C library functions for native
 *              host-test builds. PlatformIO's LDF does not build the U8g2 C
 *              library (clib/) on native because the explicit -I flags
 *              short-circuit library discovery. These stubs provide the linker
 *              symbols referenced by libU8G2Display.a objects.
 *
 *              All drawing functions are no-ops — the tests validate module
 *              logic (carousel, config, events), not pixel rendering.
 */

#include "clib/u8g2.h"
#include "clib/u8x8.h"

/* ── Drawing functions ─────────────────────────────────────────────────────── */

void u8g2_DrawBox(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y,
                  u8g2_uint_t w, u8g2_uint_t h) {
    (void)u8g2; (void)x; (void)y; (void)w; (void)h;
}

u8g2_uint_t u8g2_DrawGlyph(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y,
                           uint16_t encoding) {
    (void)u8g2; (void)x; (void)y; (void)encoding;
    return 0;
}

void u8g2_DrawHVLine(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y,
                     u8g2_uint_t len, uint8_t dir) {
    (void)u8g2; (void)x; (void)y; (void)len; (void)dir;
}

void u8g2_DrawLine(u8g2_t *u8g2, u8g2_uint_t x1, u8g2_uint_t y1,
                   u8g2_uint_t x2, u8g2_uint_t y2) {
    (void)u8g2; (void)x1; (void)y1; (void)x2; (void)y2;
}

u8g2_uint_t u8g2_DrawStr(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y,
                         const char *str) {
    (void)u8g2; (void)x; (void)y; (void)str;
    return 0;
}

void u8g2_DrawTriangle(u8g2_t *u8g2, int16_t x0, int16_t y0,
                       int16_t x1, int16_t y1,
                       int16_t x2, int16_t y2) {
    (void)u8g2; (void)x0; (void)y0; (void)x1; (void)y1; (void)x2; (void)y2;
}

u8g2_uint_t u8g2_DrawUTF8(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y,
                          const char *str) {
    (void)u8g2; (void)x; (void)y; (void)str;
    return 0;
}

void u8g2_DrawXBM(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y,
                  u8g2_uint_t w, u8g2_uint_t h, const uint8_t *bitmap) {
    (void)u8g2; (void)x; (void)y; (void)w; (void)h; (void)bitmap;
}

/* ── Measurement functions ─────────────────────────────────────────────────── */

u8g2_uint_t u8g2_GetStrWidth(u8g2_t *u8g2, const char *str) {
    (void)u8g2; (void)str;
    return 0;
}

u8g2_uint_t u8g2_GetUTF8Width(u8g2_t *u8g2, const char *str) {
    (void)u8g2; (void)str;
    return 0;
}

/* ── State functions ───────────────────────────────────────────────────────── */

void u8g2_SetClipWindow(u8g2_t *u8g2, u8g2_uint_t x0, u8g2_uint_t y0,
                        u8g2_uint_t x1, u8g2_uint_t y1) {
    (void)u8g2; (void)x0; (void)y0; (void)x1; (void)y1;
}

void u8g2_SetDrawColor(u8g2_t *u8g2, uint8_t color) {
    (void)u8g2; (void)color;
}

void u8g2_SetFont(u8g2_t *u8g2, const uint8_t *font) {
    (void)u8g2; (void)font;
}

void u8g2_UpdateDisplayArea(u8g2_t *u8g2, uint8_t tx, uint8_t ty,
                            uint8_t tw, uint8_t th) {
    (void)u8g2; (void)tx; (void)ty; (void)tw; (void)th;
}

/* ── U8X8 low-level functions ──────────────────────────────────────────────── */

void u8x8_SetContrast(u8x8_t *u8x8, uint8_t value) {
    (void)u8x8; (void)value;
}

void u8x8_SetPowerSave(u8x8_t *u8x8, uint8_t is_enable) {
    (void)u8x8; (void)is_enable;
}

uint16_t u8x8_ascii_next(u8x8_t *u8x8, uint8_t b) {
    (void)u8x8; (void)b;
    return b;
}

void u8x8_utf8_init(u8x8_t *u8x8) {
    (void)u8x8;
}

/* ── Display setup (NativeU8G2DisplayType.h constructor calls these) ───── */

void u8g2_Setup_ssd1306_i2c_128x64_noname_f(u8g2_t *u8g2,
                                             const u8g2_cb_t *rotation,
                                             u8x8_msg_cb byte_cb,
                                             u8x8_msg_cb gpio_cb) {
    (void)u8g2; (void)rotation; (void)byte_cb; (void)gpio_cb;
    /* No-op — tests don't render pixels.  The NativeU8G2Display constructor
       calls this to populate the u8g2 struct; on native it's a no-op. */
}

uint8_t u8x8_byte_empty(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    (void)u8x8; (void)msg; (void)arg_int; (void)arg_ptr;
    return 1;
}

uint8_t u8x8_dummy_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    (void)u8x8; (void)msg; (void)arg_int; (void)arg_ptr;
    return 1;
}

/* ── Rotation constants ────────────────────────────────────────────────────── */

/* U8G2_R0 expands to &u8g2_cb_r0 — a static rotation descriptor.
   The member initializer `U8G2_DISPLAY_TYPE _u8g2{U8G2_R0}` references it. */
const u8g2_cb_t u8g2_cb_r0 = { NULL, NULL, NULL };

/* ── Font data (empty arrays — satisfy extern declarations) ─────────────── */

const uint8_t u8g2_font_helvB10_tr[] = { 0 };
const uint8_t u8g2_font_helvB12_tr[] = { 0 };
const uint8_t u8g2_font_helvR08_tr[] = { 0 };
const uint8_t u8g2_font_helvR10_tr[] = { 0 };
const uint8_t u8g2_font_open_iconic_embedded_2x_t[] = { 0 };
