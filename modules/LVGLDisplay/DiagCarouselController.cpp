/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/LVGLDisplay/DiagCarouselController.cpp
 * Description: Implementation of the rotary diagnostic carousel.
 *              Manages the 12-item metric table, LVGL arc/label updates,
 *              and periodic value refresh.
 */

#include "DiagCarouselController.hpp"
#include "ui_gen.h"
#include "Logger.hpp"
#include <cstdio>
#include <cstring>

// --- Platform headers (hardware-only) ---
#ifdef ESP_PLATFORM
    #include "esp_heap_caps.h"
    #include "esp_chip_info.h"
    #include "esp_ota_ops.h"
    #include <WiFi.h>
    #include <LittleFS.h>

    extern "C" float temperatureRead(void);
#endif

// BUILD_VERSION is injected by inject_version.py at compile time
#ifndef BUILD_VERSION
    #define BUILD_VERSION "unknown"
#endif

// ─── Brand Colours ───────────────────────────────────────────────────────────

static constexpr uint32_t COLOR_OK       = 0x3FB950;  // Green
static constexpr uint32_t COLOR_WARN     = 0xF0A020;  // Amber
static constexpr uint32_t COLOR_CRITICAL = 0xF85149;  // Red
static constexpr uint32_t COLOR_INFO     = 0x5B6270;  // Grey (no evaluation)

// ─── Metric Read Functions ───────────────────────────────────────────────────

#ifdef ESP_PLATFORM

static float readChipTemp()       { return temperatureRead(); }
static float readFreeHeap()       { return (float)heap_caps_get_free_size(MALLOC_CAP_INTERNAL); }
static float readHeapLowMark()    { return (float)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL); }
static float readMaxAlloc()       { return (float)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL); }
static float readPsramFree()      { return (float)heap_caps_get_free_size(MALLOC_CAP_SPIRAM); }
static float readStoragePercent() {
    size_t total = LittleFS.totalBytes();
    if (total == 0) return 0.0f;
    return ((float)LittleFS.usedBytes() / (float)total) * 100.0f;
}
static float readWifiRssi()       { return (float)WiFi.RSSI(); }
static float readUptimeSeconds()  { return (float)(millis() / 1000); }

static void readWifiIp(char* buf, size_t len) {
    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
        snprintf(buf, len, "%s", WiFi.softAPIP().toString().c_str());
    } else {
        snprintf(buf, len, "%s", WiFi.localIP().toString().c_str());
    }
}

static void readWifiSsid(char* buf, size_t len) {
    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
        snprintf(buf, len, "%s", WiFi.softAPSSID().c_str());
    } else {
        snprintf(buf, len, "%s", WiFi.SSID().c_str());
    }
}

static void readFirmwareVersion(char* buf, size_t len) {
    snprintf(buf, len, "%s", BUILD_VERSION);
}

static void readPartition(char* buf, size_t len) {
    const esp_partition_t* running = esp_ota_get_running_partition();
    if (running != nullptr) {
        snprintf(buf, len, "%s", running->label);
    } else {
        snprintf(buf, len, "n/a");
    }
}

#else
// Stubs for native/test builds
static float readChipTemp()       { return 0.0f; }
static float readFreeHeap()       { return 0.0f; }
static float readHeapLowMark()    { return 0.0f; }
static float readMaxAlloc()       { return 0.0f; }
static float readPsramFree()      { return 0.0f; }
static float readStoragePercent() { return 0.0f; }
static float readWifiRssi()       { return 0.0f; }
static float readUptimeSeconds()  { return 0.0f; }
static void readWifiIp(char* buf, size_t len)       { snprintf(buf, len, "n/a"); }
static void readWifiSsid(char* buf, size_t len)     { snprintf(buf, len, "n/a"); }
static void readFirmwareVersion(char* buf, size_t len) { snprintf(buf, len, "test"); }
static void readPartition(char* buf, size_t len)    { snprintf(buf, len, "n/a"); }
#endif

// ─── Static Metric Table ─────────────────────────────────────────────────────
//
// 12 items: 8 numeric (with evaluation) + 4 text-info.
// Aligned with MCP system_info and portal /api/system.
//
//                                          category          type            units  readNumeric          divisor   warn   crit   higher  dp   readText

const DiagItem DiagCarouselController::ITEMS[ITEM_COUNT] = {
    { "TEMPERATURE",  DiagType::NUMERIC, "\xC2\xB0""C", readChipTemp,       1.0f,    65.0f, 80.0f, false, 1, nullptr },
    { "FREE HEAP",    DiagType::NUMERIC, "KB",           readFreeHeap,       1024.0f, 40.0f, 20.0f, true,  0, nullptr },
    { "HEAP LOW MARK",DiagType::NUMERIC, "KB",           readHeapLowMark,    1024.0f, 30.0f, 15.0f, true,  0, nullptr },
    { "MAX ALLOC",    DiagType::NUMERIC, "KB",           readMaxAlloc,       1024.0f, 16.0f,  8.0f, true,  0, nullptr },
    { "PSRAM FREE",   DiagType::NUMERIC, "MB",           readPsramFree,      1048576.0f, 2.0f, 1.0f, true, 1, nullptr },
    { "STORAGE USED", DiagType::NUMERIC, "%",            readStoragePercent, 1.0f,    80.0f, 95.0f, false, 1, nullptr },
    { "WIFI RSSI",    DiagType::NUMERIC, "dBm",          readWifiRssi,       1.0f,   -75.0f,-85.0f, true,  0, nullptr },
    { "UPTIME",       DiagType::NUMERIC, "",             readUptimeSeconds,  1.0f,     0.0f,  0.0f, true,  0, nullptr },
    { "WIFI IP",      DiagType::TEXT,    nullptr,         nullptr,            0, 0, 0, false, 0, readWifiIp },
    { "WIFI SSID",    DiagType::TEXT,    nullptr,         nullptr,            0, 0, 0, false, 0, readWifiSsid },
    { "FIRMWARE",     DiagType::TEXT,    nullptr,         nullptr,            0, 0, 0, false, 0, readFirmwareVersion },
    { "PARTITION",    DiagType::TEXT,    nullptr,         nullptr,            0, 0, 0, false, 0, readPartition },
};

// ─── Helpers ─────────────────────────────────────────────────────────────────

static lv_obj_t* findChildByName(lv_obj_t* parent, const char* name) {
    if (!parent || !name) return nullptr;
    uint32_t count = lv_obj_get_child_count(parent);
    for (uint32_t i = 0; i < count; i++) {
        lv_obj_t* child = lv_obj_get_child(parent, (int32_t)i);
        if (!child) continue;
#if defined(LV_USE_OBJ_NAME) && LV_USE_OBJ_NAME
        const char* childName = lv_obj_get_name(child);
        if (childName && strcmp(childName, name) == 0) {
            return child;
        }
#endif
        lv_obj_t* found = findChildByName(child, name);
        if (found) return found;
    }
    return nullptr;
}

// ─── Controller Implementation ───────────────────────────────────────────────

void DiagCarouselController::begin(lv_group_t* group, lv_obj_t* diagScreen) {
    _group = group;
    _currentIndex = 0;
    _active = true;
    _lastRefreshMs = 0;

    lv_obj_t* scr = diagScreen ? diagScreen : lv_screen_active();
    if (scr != nullptr) {
        _arc = findChildByName(scr, "diag_arc");
        _valueLabel = findChildByName(scr, "diag_value");
        _statusLabel = findChildByName(scr, "diag_status");

        // Fallback by child index if name-based lookup did not find widgets.
        // This should not trigger with LV_USE_OBJ_NAME enabled.
        if (_arc == nullptr && lv_obj_get_child_count(scr) >= 5) {
            ESP_LOGW("DiagCarousel", "Name-based widget lookup failed — "
                     "falling back to child index (expected: diag_arc, diag_value, diag_status)");
            _arc = lv_obj_get_child(scr, 0);
            _valueLabel = lv_obj_get_child(scr, 3);
            _statusLabel = lv_obj_get_child(scr, 4);
        }
    }

    if (_arc == nullptr) return;

    // Configure arc as a discrete N-step selector
    lv_arc_set_range(_arc, 0, (int16_t)(ITEM_COUNT - 1));
    lv_arc_set_value(_arc, 0);

    // Register value-changed callback (once — persists for widget lifetime)
    if (!_callbackRegistered) {
        lv_obj_add_event_cb(_arc, onArcValueChanged,
                            LV_EVENT_VALUE_CHANGED, this);
        _callbackRegistered = true;
    }

    // Explicitly add the arc to the encoder group and focus it.
    if (_group != nullptr) {
        lv_group_add_obj(_group, _arc);
        lv_group_focus_obj(_arc);
        lv_group_set_editing(_group, true);
    }

    // Initial widget population
    updateWidgets();
}

void DiagCarouselController::stop() {
    _active = false;
    if (_group != nullptr) {
        lv_group_set_editing(_group, false);
    }
}

void DiagCarouselController::tick(uint32_t now) {
    if (!_active) return;

    // Periodic value refresh (lightweight — only value + status text)
    if (now - _lastRefreshMs >= REFRESH_INTERVAL_MS) {
        _lastRefreshMs = now;
        refreshValueOnly();
    }
}

// ─── LVGL Callback ───────────────────────────────────────────────────────────

void DiagCarouselController::onArcValueChanged(lv_event_t* e) {
    auto* self = static_cast<DiagCarouselController*>(lv_event_get_user_data(e));
    if (!self || !self->_active) return;

    lv_obj_t* arc = static_cast<lv_obj_t*>(lv_event_get_target(e));
    int32_t value = lv_arc_get_value(arc);

    // Clamp to valid range
    if (value < 0) value = 0;
    if (value >= (int32_t)ITEM_COUNT) value = ITEM_COUNT - 1;

    self->_currentIndex = value;
    self->updateWidgets();
}

// ─── Widget Update ───────────────────────────────────────────────────────────

void DiagCarouselController::updateWidgets() {
    const DiagItem& item = ITEMS[_currentIndex];

    // --- Arc position subject ---
    lv_subject_set_int(&sys_diag_arc_value, _currentIndex);

    // --- Counter label subject: "3 / 12" ---
    char counterBuf[16];
    snprintf(counterBuf, sizeof(counterBuf), "%d / %d",
             _currentIndex + 1, (int)ITEM_COUNT);
    lv_subject_copy_string(&sys_diag_counter, counterBuf);

    // --- Category label subject ---
    lv_subject_copy_string(&sys_diag_category, item.category);

    // --- Value label + Status label ---
    if (item.type == DiagType::NUMERIC) {
        float rawValue = item.readNumeric ? item.readNumeric() : 0.0f;
        float displayValue = rawValue / item.displayDivisor;

        // Format value string
        char valueBuf[32];

        // Special formatting for uptime (human-readable duration)
        if (item.readNumeric == readUptimeSeconds) {
            uint32_t totalSec = (uint32_t)rawValue;
            uint32_t days  = totalSec / 86400;
            uint32_t hours = (totalSec % 86400) / 3600;
            uint32_t mins  = (totalSec % 3600) / 60;
            if (days > 0) {
                snprintf(valueBuf, sizeof(valueBuf), "%lud %luh %lum", days, hours, mins);
            } else if (hours > 0) {
                snprintf(valueBuf, sizeof(valueBuf), "%luh %lum", hours, mins);
            } else {
                snprintf(valueBuf, sizeof(valueBuf), "%lum %lus", mins, totalSec % 60);
            }
        } else if (item.decimalPlaces == 0) {
            snprintf(valueBuf, sizeof(valueBuf), "%.0f %s", displayValue, item.units);
        } else {
            snprintf(valueBuf, sizeof(valueBuf), "%.*f %s",
                     item.decimalPlaces, displayValue, item.units);
        }

        lv_subject_copy_string(&sys_diag_value, valueBuf);

        // Styling: Large font for numeric values
        if (_valueLabel != nullptr) {
            lv_obj_set_style_text_font(_valueLabel, montserrat_40, LV_PART_MAIN);
        }

        // Evaluation status
        const char* status = evaluateStatus(item, displayValue);
        lv_subject_copy_string(&sys_diag_status, status);

        if (_statusLabel != nullptr) {
            lv_obj_set_style_text_color(_statusLabel,
                                        evaluateColour(status), LV_PART_MAIN);
        }

        // Arc indicator colour matches evaluation
        if (_arc != nullptr) {
            lv_obj_set_style_arc_color(_arc,
                                      evaluateColour(status),
                                      LV_PART_INDICATOR);
        }

    } else {
        // TEXT item
        if (item.readText) {
            item.readText(_textBuf, sizeof(_textBuf));
        } else {
            snprintf(_textBuf, sizeof(_textBuf), "-");
        }
        lv_subject_copy_string(&sys_diag_value, _textBuf);

        // Styling: Smaller font for text values (IPs, SSIDs, versions)
        if (_valueLabel != nullptr) {
            lv_obj_set_style_text_font(_valueLabel, montserrat_22, LV_PART_MAIN);
        }

        // No evaluation for text items
        lv_subject_copy_string(&sys_diag_status, "-");

        if (_statusLabel != nullptr) {
            lv_obj_set_style_text_color(_statusLabel,
                                        lv_color_hex(COLOR_INFO), LV_PART_MAIN);
        }

        // Grey arc for info items
        if (_arc != nullptr) {
            lv_obj_set_style_arc_color(_arc,
                                      lv_color_hex(COLOR_INFO),
                                      LV_PART_INDICATOR);
        }
    }
}

void DiagCarouselController::refreshValueOnly() {
    const DiagItem& item = ITEMS[_currentIndex];

    if (item.type == DiagType::NUMERIC) {
        float rawValue = item.readNumeric ? item.readNumeric() : 0.0f;
        float displayValue = rawValue / item.displayDivisor;

        char valueBuf[32];
        if (item.readNumeric == readUptimeSeconds) {
            uint32_t totalSec = (uint32_t)rawValue;
            uint32_t days  = totalSec / 86400;
            uint32_t hours = (totalSec % 86400) / 3600;
            uint32_t mins  = (totalSec % 3600) / 60;
            if (days > 0) {
                snprintf(valueBuf, sizeof(valueBuf), "%lud %luh %lum", days, hours, mins);
            } else if (hours > 0) {
                snprintf(valueBuf, sizeof(valueBuf), "%luh %lum", hours, mins);
            } else {
                snprintf(valueBuf, sizeof(valueBuf), "%lum %lus", mins, totalSec % 60);
            }
        } else if (item.decimalPlaces == 0) {
            snprintf(valueBuf, sizeof(valueBuf), "%.0f %s", displayValue, item.units);
        } else {
            snprintf(valueBuf, sizeof(valueBuf), "%.*f %s",
                     item.decimalPlaces, displayValue, item.units);
        }
        lv_subject_copy_string(&sys_diag_value, valueBuf);

        // Refresh status text (evaluation may change as values cross thresholds)
        const char* status = evaluateStatus(item, displayValue);
        lv_subject_copy_string(&sys_diag_status, status);

        if (_statusLabel != nullptr) {
            lv_obj_set_style_text_color(_statusLabel,
                                        evaluateColour(status), LV_PART_MAIN);
        }

        if (_arc != nullptr) {
            lv_obj_set_style_arc_color(_arc,
                                      evaluateColour(status),
                                      LV_PART_INDICATOR);
        }
    }
    // TEXT items don't change frequently — skip refresh
}

// ─── Evaluation Logic ────────────────────────────────────────────────────────

const char* DiagCarouselController::evaluateStatus(const DiagItem& item, float value) const {
    // No evaluation if thresholds are both zero (e.g. uptime)
    if (item.warnThreshold == 0.0f && item.critThreshold == 0.0f) return "-";

    if (item.higherIsBetter) {
        // Higher is better (e.g. free heap): low values are bad
        if (value <= item.critThreshold) return "CRITICAL";
        if (value <= item.warnThreshold) return "LOW";
        return "OK";
    } else {
        // Lower is better (e.g. temperature): high values are bad
        if (value >= item.critThreshold) return "CRITICAL";
        if (value >= item.warnThreshold) return "HIGH";
        return "OK";
    }
}

lv_color_t DiagCarouselController::evaluateColour(const char* status) const {
    if (strcmp(status, "OK") == 0)       return lv_color_hex(COLOR_OK);
    if (strcmp(status, "CRITICAL") == 0) return lv_color_hex(COLOR_CRITICAL);
    if (strcmp(status, "LOW") == 0)      return lv_color_hex(COLOR_WARN);
    if (strcmp(status, "HIGH") == 0)     return lv_color_hex(COLOR_WARN);
    return lv_color_hex(COLOR_INFO);  // "—" or unknown
}
