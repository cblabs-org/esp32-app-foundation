/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/widgets/ClockWidget.cpp
 * Description: Implementation of the real-time clock drawing logic.
 *              Formats time strings from TimeManager, applies font metrics
 *              for right-aligned digit columns, and manages 1Hz colon blink.
 *              All TimeManager calls are using native test stubs.
 *
 * Exported Functions/Classes:
 * - ClockWidget: [Class implementation]
 *   - setFont(): Update the primary typography.
 *   - setSecondaryFont(): Set optional font for seconds digits.
 *   - setFormat(): Switch between HH_MM and HH_MM_SS modes.
 *   - tick(): Logic update for blink timing (500ms toggle).
 *   - render(): Primary full-frame drawing.
 *   - renderAnimationUpdate(): Partial-screen redraw on blink/minute change.
 */

#include "ClockWidget.hpp"
#include <cstring>

/**
 * @brief Initialize the clock with its time manager and default layout.
 *        Defaults to HH_MM format with blinking enabled.
 * @param timeMgr Pointer to the TimeManager dependency (nullable).
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param w Width (-1 for auto).
 * @param h Height (-1 for auto).
 * @param font Optional font override.
 */
ClockWidget::ClockWidget(TimeManager* timeMgr, int x, int y, int w, int h,
                         const uint8_t* font)
    : IGfxWidget(x, y, w, h),
      _showColon(true), _oldColon(true), _blinkEnabled(true),
      _lastBlinkMs(0), _lastMinute(-1), _lastSecond(-1),
      _format(ClockFormat::HH_MM), _font(font), _secondaryFont(nullptr),
      _alignment(1), _timeMgr(timeMgr) {
}

/**
 * @brief Update the primary font used by the clock.
 * @param newFont Pointer to the U8G2 font array.
 */
void ClockWidget::setFont(const uint8_t* newFont) {
    _font = newFont;
}

/**
 * @brief Update the secondary font used for the seconds component.
 * @param newFont Pointer to the U8G2 font array (nullptr = use primary).
 */
void ClockWidget::setSecondaryFont(const uint8_t* newFont) {
    _secondaryFont = newFont;
}

/**
 * @brief Set the display format of the clock.
 * @param newFormat HH_MM or HH_MM_SS.
 */
void ClockWidget::setFormat(ClockFormat newFormat) {
    _format = newFormat;
}

/**
 * @brief Logic pulse for the blinking animation.
 *        Toggles showColon every 500ms when blinkEnabled is true.
 * @param currentMillis System uptime in milliseconds.
 */
void ClockWidget::tick(uint32_t currentMillis) {
    if (!_isVisible || !_blinkEnabled) return;

    if (currentMillis - _lastBlinkMs >= 500) {
        _showColon = !_showColon;
        _lastBlinkMs = currentMillis;
    }
}

// =============================================================================
// Rendering — requires U8G2 hardware driver and TimeManager
// =============================================================================

#include "DrawingPrimitives.hpp"
#include <TimeManager.hpp>
#include <time.h>

/**
 * @brief Full frame render for the time string.
 *        Measures all digit widths to right-align hour/minute/second columns.
 *        Vertically centers text within the bounding box height.
 * @param display U8G2 reference.
 */
void ClockWidget::render(U8G2& display) {
    if (!_isVisible) return;

    int renderW = (_width > 0) ? _width : 56;
    int renderH = (_height > 0) ? _height : 14;

    U8G2StateSaver stateSaver(display);
    if (_font) display.setFont(_font);

    if (!_timeMgr) return;

    // --- Step 1: Acquire current time from injected TimeManager ---
    struct tm timeinfo;
    if (!_timeMgr->getLocalTime(&timeinfo)) return;

    _lastMinute = timeinfo.tm_min;
    _lastSecond = timeinfo.tm_sec;

    // --- Step 2: Format time components as strings ---
    char hourStr[3];
    char minStr[3];
    char secStr[3];
    strftime(hourStr, sizeof(hourStr), "%H", &timeinfo);
    strftime(minStr, sizeof(minStr), "%M", &timeinfo);
    strftime(secStr, sizeof(secStr), "%S", &timeinfo);

    // --- Step 3: Measure widths for right-aligned digit columns ---
    int hourW = display.getStrWidth(hourStr);
    int minW = display.getStrWidth(minStr);
    int colonW = display.getStrWidth(":");

    int maxHourW = display.getStrWidth("00");
    int maxMinW = display.getStrWidth("00");

    int maxSecW = 0;
    int maxSecColonW = 0;
    int secW = 0;

    if (_format == ClockFormat::HH_MM_SS) {
        if (_secondaryFont) display.setFont(_secondaryFont);
        secW = display.getStrWidth(secStr);
        maxSecW = display.getStrWidth("00");
        maxSecColonW = display.getStrWidth(":");
        if (_secondaryFont) display.setFont(_font); // Restore main font
    }

    // --- Step 4: Calculate total footprint and alignment offset ---
    int actualW = maxHourW + colonW + maxMinW + 2; // 2px spacing between elements
    if (_format == ClockFormat::HH_MM_SS) {
        actualW += maxSecColonW + maxSecW + 2;
    }

    int startX = _x;
    if (_alignment == 1) {       // CENTER
        startX = _x + (renderW - actualW) / 2;
    } else if (_alignment == 2) { // RIGHT
        startX = _x + renderW - actualW - 1;
    }

    // --- Step 5: Vertical centering within bounding box ---
    // setFontPosTop is used globally — calculate padding for visual centering.
    int fontHeight = display.getAscent() - display.getDescent();
    int drawY = _y + (renderH - fontHeight) / 2;

    int secDrawY = drawY;
    if (_format == ClockFormat::HH_MM_SS && _secondaryFont) {
        display.setFont(_secondaryFont);
        int secFontHeight = display.getAscent() - display.getDescent();
        // Align bottoms of the two text boundaries (flushes numerical baselines)
        secDrawY = drawY + fontHeight - secFontHeight;
        display.setFont(_font);
    }

    // --- Step 6: Clear and draw ---
    blankArea(display, _x, _y, renderW, renderH);

    // Draw hours right-aligned within its max bounding box
    int hOff = maxHourW - hourW;
    display.drawStr(startX + hOff, drawY, hourStr);

    int currentX = startX + maxHourW + 1;

    // Draw colon (visible/hidden for blink effect)
    if (_showColon) {
        display.drawStr(currentX, drawY, ":");
    }
    currentX += colonW + 1;

    // Draw minutes right-aligned
    int mOff = maxMinW - minW;
    display.drawStr(currentX + mOff, drawY, minStr);
    currentX += maxMinW + 1;

    // --- Draw seconds if HH_MM_SS format ---
    if (_format == ClockFormat::HH_MM_SS) {
        if (_secondaryFont) display.setFont(_secondaryFont);

        // Second colon is always visible (non-blinking)
        int sColonW = display.getStrWidth(":");
        int sColOff = (maxSecColonW - sColonW) / 2;
        display.drawStr(currentX + sColOff, secDrawY, ":");

        currentX += maxSecColonW + 1;

        int sOff = maxSecW - secW;
        display.drawStr(currentX + sOff, secDrawY, secStr);

        if (_secondaryFont) display.setFont(_font);
    }
}

/**
 * @brief Partial redraw for the blinking colon or minute/second change.
 *        Only calls render() and updateDisplayArea() if visual state changed.
 * @param display U8G2 reference.
 * @param currentMillis Milliseconds since boot.
 */
void ClockWidget::renderAnimationUpdate(U8G2& display, uint32_t currentMillis) {
    if (!_isVisible || !_timeMgr) return;

    struct tm timeinfo;
    if (!_timeMgr->getLocalTime(&timeinfo)) return;

    bool minuteChanged = (timeinfo.tm_min != _lastMinute);
    bool secondChanged = false;
    if (_format == ClockFormat::HH_MM_SS) {
        secondChanged = (timeinfo.tm_sec != _lastSecond);
    }

    if (_oldColon != _showColon || minuteChanged || secondChanged) {
        _oldColon = _showColon;
        _lastMinute = timeinfo.tm_min;
        _lastSecond = timeinfo.tm_sec;
        render(display);

        int renderW = (_width > 0) ? _width : 56;
        int renderH = (_height > 0) ? _height : 14;
        display.updateDisplayArea(_x / 8, _y / 8, (renderW + 7) / 8, (renderH + 7) / 8);
    }
}

