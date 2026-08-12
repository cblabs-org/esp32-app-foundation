/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/LVGLDisplay/DiagCarouselController.hpp
 * Description: Rotary diagnostic carousel for the CrowPanel DiagScreen.
 *              Drives the SLS arc/label widgets via LVGL's encoder EDIT mode:
 *              encoder rotation snaps the arc between N diagnostic items,
 *              and the text labels update to show each metric's category,
 *              value+units, position counter, and health evaluation.
 *
 * Exported Classes:
 * - DiagCarouselController: Encoder-driven diagnostic carousel controller
 */

#pragma once
#include <cstdint>
#include <cstddef>
#include "lvgl.h"

// ─── Data Model ──────────────────────────────────────────────────────────────

/** @brief Discriminator for numeric vs text-info diagnostic items. */
enum class DiagType : uint8_t {
    NUMERIC,  ///< Has a float value, units, and health evaluation thresholds
    TEXT      ///< Has a string value, no evaluation (info-only)
};

/**
 * @brief Describes a single diagnostic metric for the carousel.
 *
 * Numeric items produce a float value (optionally scaled by displayDivisor)
 * with warn/crit thresholds for health evaluation. Text items produce a
 * string (WiFi IP, firmware version, etc.) with no evaluation.
 */
struct DiagItem {
    const char* category;              ///< Display name ("TEMPERATURE", "WIFI IP", etc.)
    DiagType    type;                  ///< NUMERIC or TEXT

    // --- Numeric fields (type == NUMERIC) ---
    const char* units;                 ///< Unit suffix ("°C", "KB", "dBm", "%")
    float     (*readNumeric)();        ///< Read current value (raw, before divisor)
    float       displayDivisor;        ///< Divide raw value by this for display (1.0=raw, 1024.0=bytes→KB)
    float       warnThreshold;         ///< Threshold for WARN evaluation (post-divisor value)
    float       critThreshold;         ///< Threshold for CRITICAL evaluation (post-divisor value)
    bool        higherIsBetter;        ///< true: low=BAD (e.g. heap), false: high=BAD (e.g. temp)
    uint8_t     decimalPlaces;         ///< Number of decimal places to display

    // --- Text fields (type == TEXT) ---
    void      (*readText)(char* buf, size_t len);  ///< Write string value into buf
};

// ─── Controller ──────────────────────────────────────────────────────────────

/**
 * @brief Encoder-driven diagnostic carousel for the CrowPanel LVGL DiagScreen.
 *
 * When activated (via begin()), configures the SLS DiagArc as a discrete
 * segment selector with range [0, N-1]. LVGL's encoder EDIT mode drives the
 * arc value; a VALUE_CHANGED callback updates the category/value/counter/status
 * labels for the selected diagnostic item.
 *
 * Periodically refreshes the current item's value (tick-driven, ~1 Hz).
 */
class DiagCarouselController {
public:
    /** @brief Number of diagnostic items in the carousel. */
    static constexpr size_t ITEM_COUNT = 12;

    /**
     * @brief Activate the carousel on the DiagScreen.
     *
     * Sets the arc range, registers the value-changed callback, enters
     * EDIT mode on the LVGL group, and performs the initial widget update.
     *
     * @param group  The LVGL encoder focus group (to enter EDIT mode).
     */
    void begin(lv_group_t* group);

    /**
     * @brief Deactivate the carousel (when leaving DiagScreen).
     *
     * Exits EDIT mode and disables the value-changed callback.
     */
    void stop();

    /**
     * @brief Periodic tick — refreshes the current item's displayed value.
     *
     * @param now  Current time in milliseconds (from millis()).
     */
    void tick(uint32_t now);

private:
    void updateWidgets();                              ///< Push all widget content for current item
    const char* evaluateStatus(const DiagItem& item, float value) const;
    lv_color_t evaluateColour(const char* status) const;

    static void onArcValueChanged(lv_event_t* e);     ///< LVGL event callback

    static const DiagItem ITEMS[ITEM_COUNT];           ///< Static metric table

    int         _currentIndex  = 0;
    lv_group_t* _group         = nullptr;
    uint32_t    _lastRefreshMs = 0;
    bool        _active        = false;
    bool        _callbackRegistered = false;
    char        _textBuf[64]   = {};                   ///< Shared buffer for text item reads

    static constexpr uint32_t REFRESH_INTERVAL_MS = 1000;
};
