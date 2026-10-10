/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_u8g2glDisplay/u8g2gl_native_stubs.c
 * Description: U8G2 C-library symbols the U8G2GL runtime references and the
 *              native stubs of the legacy suite do not provide. PlatformIO does
 *              not build U8G2's clib on native (see
 *              test/test_unit/test_u8g2Display/u8g2_native_stubs.c, which this
 *              file includes and so reuses unchanged), and the runtime calls a
 *              few more of its functions than the legacy module does.
 *
 *              Like the legacy stubs these draw nothing: the tests validate
 *              logic, and pixels are test/render's job. The one function that
 *              keeps state is u8g2_SetFontMode, because the board's contract is
 *              to restore the font mode the runtime changes.
 */

#include "../test_u8g2Display/u8g2_native_stubs.c"

/* ── Font data the generated UI references and the legacy stubs do not define ── */

/* The WiFi row's Latin-1 font (globals.xml). Empty like the legacy stubs' fonts: nothing is drawn. */
const uint8_t u8g2_font_helvR08_tf[] = { 0 };

/* ── Drawing functions the runtime calls and the legacy stubs do not provide ─ */

void u8g2_ClearBuffer(u8g2_t *u8g2) { (void)u8g2; }
void u8g2_SendBuffer(u8g2_t *u8g2) { (void)u8g2; }

void u8g2_DrawHLine(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len) {
    (void)u8g2; (void)x; (void)y; (void)len;
}

void u8g2_DrawVLine(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len) {
    (void)u8g2; (void)x; (void)y; (void)len;
}

void u8g2_DrawPixel(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y) {
    (void)u8g2; (void)x; (void)y;
}

void u8g2_DrawFrame(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t h) {
    (void)u8g2; (void)x; (void)y; (void)w; (void)h;
}

void u8g2_DrawRBox(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t h,
                   u8g2_uint_t r) {
    (void)u8g2; (void)x; (void)y; (void)w; (void)h; (void)r;
}

void u8g2_DrawRFrame(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t h,
                     u8g2_uint_t r) {
    (void)u8g2; (void)x; (void)y; (void)w; (void)h; (void)r;
}

void u8g2_DrawCircle(u8g2_t *u8g2, u8g2_uint_t x0, u8g2_uint_t y0, u8g2_uint_t rad, uint8_t option) {
    (void)u8g2; (void)x0; (void)y0; (void)rad; (void)option;
}

void u8g2_DrawDisc(u8g2_t *u8g2, u8g2_uint_t x0, u8g2_uint_t y0, u8g2_uint_t rad, uint8_t option) {
    (void)u8g2; (void)x0; (void)y0; (void)rad; (void)option;
}

void u8g2_DrawEllipse(u8g2_t *u8g2, u8g2_uint_t x0, u8g2_uint_t y0, u8g2_uint_t rx, u8g2_uint_t ry,
                      uint8_t option) {
    (void)u8g2; (void)x0; (void)y0; (void)rx; (void)ry; (void)option;
}

void u8g2_DrawFilledEllipse(u8g2_t *u8g2, u8g2_uint_t x0, u8g2_uint_t y0, u8g2_uint_t rx,
                            u8g2_uint_t ry, uint8_t option) {
    (void)u8g2; (void)x0; (void)y0; (void)rx; (void)ry; (void)option;
}

void u8g2_DrawXBMP(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t h,
                   const uint8_t *bitmap) {
    (void)u8g2; (void)x; (void)y; (void)w; (void)h; (void)bitmap;
}

/* ── State functions ───────────────────────────────────────────────────────── */

void u8g2_SetBitmapMode(u8g2_t *u8g2, uint8_t is_transparent) {
    (void)u8g2; (void)is_transparent;
}

void u8g2_SetMaxClipWindow(u8g2_t *u8g2) { (void)u8g2; }

/* The one stateful stub: the board saves and restores this, and the tests read it back
   from the same field the real library keeps it in. */
void u8g2_SetFontMode(u8g2_t *u8g2, uint8_t is_transparent) {
    u8g2->font_decode.is_transparent = is_transparent;
}
