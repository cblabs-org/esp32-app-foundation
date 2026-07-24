/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/LVGLDisplay/LVGLDisplayModule.cpp
 * Description: Generic LVGL v9 colour-TFT display module core implementation.
 *              Manages LVGL lifecycle (lv_init, lv_tick_inc, lv_timer_handler),
 *              DMA partial draw buffer allocation, input device discovery, and focus group wiring.
 */

#include "LVGLDisplayModule.hpp"
#include "esp_heap_caps.h"

// -----------------------------------------------------------------------------
// Weak Symbol Declaration & Default Implementation for SquareLine Studio (D8)
// -----------------------------------------------------------------------------
extern "C" __attribute__((weak)) void ui_init(void) {
    // Weak default no-op implementation. Downstream applications override this.
}

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

LVGLDisplayModule::LVGLDisplayModule(ILVGLHardwareAdapter& adapter, ILVGLSystemScreens* screens)
    : _adapter(adapter), _screens(screens) {
}

LVGLDisplayModule::~LVGLDisplayModule() {
    if (_group != nullptr) {
        lv_group_delete(_group);
        _group = nullptr;
    }
    if (_buf1 != nullptr) {
        heap_caps_free(_buf1);
        _buf1 = nullptr;
    }
    if (_buf2 != nullptr) {
        heap_caps_free(_buf2);
        _buf2 = nullptr;
    }
    if (_display != nullptr) {
        lv_display_delete(_display);
        _display = nullptr;
    }
}

void LVGLDisplayModule::bootstrapHardware() {
    if (_display != nullptr) {
        return;  // Already bootstrapped
    }

    // --- Step 1: LVGL Core Initialization ---
    lv_init();

    // --- Step 2: Hardware Adapter Display Creation ---
    _display = _adapter.createDisplay();
    if (_display == nullptr) {
        return;
    }

    // --- Step 3: Draw Buffer Management (OQ-7) ---
    // Module allocates partial double buffers in SRAM by default using getBufferLineCount().
    // If the adapter pre-configured buffers (lv_display_get_buf_1 != nullptr), respect it and skip.
    if (lv_display_get_buf_1(_display) == nullptr) {
        uint16_t width = _adapter.getScreenWidth();
        uint16_t lineCount = _adapter.getBufferLineCount();
        _bufSize = static_cast<size_t>(width) * lineCount * sizeof(lv_color_t);

        if (_bufSize > 0) {
            _buf1 = heap_caps_malloc(_bufSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
            _buf2 = heap_caps_malloc(_bufSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);

            if (_buf1 == nullptr || _buf2 == nullptr) {
                // Graceful degrade: free partial allocations on failure
                if (_buf1 != nullptr) {
                    heap_caps_free(_buf1);
                    _buf1 = nullptr;
                }
                if (_buf2 != nullptr) {
                    heap_caps_free(_buf2);
                    _buf2 = nullptr;
                }
                _bufSize = 0;
            } else {
                lv_display_set_buffers(_display, _buf1, _buf2, _bufSize, LV_DISPLAY_RENDER_MODE_PARTIAL);
            }
        }
    }

    // --- Step 4: Input Peripheral Initialization & Group Wiring ---
    _touchInDev = _adapter.createTouchInput();
    _encoderInDev = _adapter.createEncoderInput();
    _keypadInDev = _adapter.createKeypadInput();

    if (_encoderInDev != nullptr || _keypadInDev != nullptr) {
        _group = lv_group_create();
        if (_group != nullptr) {
            if (_encoderInDev != nullptr) {
                lv_indev_set_group(_encoderInDev, _group);
            }
            if (_keypadInDev != nullptr) {
                lv_indev_set_group(_keypadInDev, _group);
            }
        }
    }

    _bootstrapMode = true;

    // --- Step 5: Apply Hardware Brightness & Rotation ---
    _adapter.setBrightness(_brightness);
    if (_rotation != 0) {
        lv_display_rotation_t rotEnum = LV_DISPLAY_ROTATION_0;
        if (_rotation == 90) rotEnum = LV_DISPLAY_ROTATION_90;
        else if (_rotation == 180) rotEnum = LV_DISPLAY_ROTATION_180;
        else if (_rotation == 270) rotEnum = LV_DISPLAY_ROTATION_270;
        lv_display_set_rotation(_display, rotEnum);
    }
}

void LVGLDisplayModule::begin(EventBus* eventBus) {
    (void)eventBus;

    // Phase 1 fallback: ensure bootstrapHardware() ran
    if (_display == nullptr) {
        bootstrapHardware();
    }

    // Execute weak ui_init() hook if defined by application
    if (ui_init != nullptr) {
        ui_init();
    }

    // TODO: WO5 adds IStatusReporter system screens
    // TODO: WO6 adds EventBus subscriptions (ota.progress, display.message, system.error)
    // TODO: WO7 adds IWebContributor REST API and portal tab
    // TODO: WO8 adds IMCPContributor MCP tools

    _bootstrapMode = false;
}

// -----------------------------------------------------------------------------
// ITickable
// -----------------------------------------------------------------------------

void LVGLDisplayModule::tick(uint32_t now) {
    if (_lastTickMs == 0) {
        _lastTickMs = now;
        return;
    }

    uint32_t elapsed = now - _lastTickMs;
    _lastTickMs = now;

    lv_tick_inc(elapsed);
    lv_timer_handler();
}

// -----------------------------------------------------------------------------
// IConfigurable — ConfigManager Gateway (namespace: "display")
// -----------------------------------------------------------------------------

void LVGLDisplayModule::applyConfig(const JsonObject& section) {
    if (!section["brightness"].isNull()) {
        int b = section["brightness"].as<int>();
        if (b < 0) b = 0;
        if (b > 255) b = 255;
        _brightness = static_cast<uint8_t>(b);
    } else {
        _brightness = 255;
    }

    if (!section["rotation"].isNull()) {
        int r = section["rotation"].as<int>();
        if (r == 90 || r == 1) {
            _rotation = 90;
        } else if (r == 180 || r == 2) {
            _rotation = 180;
        } else if (r == 270 || r == 3) {
            _rotation = 270;
        } else {
            _rotation = 0;
        }
    } else {
        _rotation = 0;
    }

    _screensaverEnabled = section["screensaverEnabled"] | true;

    if (!section["dimTimeout"].isNull()) {
        _dimTimeoutMs   = section["dimTimeout"]   | 150000UL;
        _blankTimeoutMs = section["blankTimeout"] | 300000UL;
    } else if (!section["sleepTimeout"].isNull()) {
        unsigned long st = section["sleepTimeout"] | 300000UL;
        _dimTimeoutMs   = st / 2;
        _blankTimeoutMs = st;
    } else {
        _dimTimeoutMs   = 150000UL;
        _blankTimeoutMs = 300000UL;
    }

    _adapter.setBrightness(_brightness);

    if (_display != nullptr) {
        lv_display_rotation_t rotEnum = LV_DISPLAY_ROTATION_0;
        if (_rotation == 90) rotEnum = LV_DISPLAY_ROTATION_90;
        else if (_rotation == 180) rotEnum = LV_DISPLAY_ROTATION_180;
        else if (_rotation == 270) rotEnum = LV_DISPLAY_ROTATION_270;

        lv_display_set_rotation(_display, rotEnum);
    }
}

void LVGLDisplayModule::serializeConfig(JsonObject& section) const {
    section["brightness"]         = _brightness;
    section["rotation"]           = _rotation;
    section["screensaverEnabled"] = _screensaverEnabled;
    section["dimTimeout"]         = _dimTimeoutMs;
    section["blankTimeout"]       = _blankTimeoutMs;
}

void LVGLDisplayModule::resetConfig() {
    _brightness         = 255;
    _rotation           = 0;
    _screensaverEnabled = true;
    _dimTimeoutMs       = 150000;
    _blankTimeoutMs     = 300000;

    _adapter.setBrightness(_brightness);

    if (_display != nullptr) {
        lv_display_set_rotation(_display, LV_DISPLAY_ROTATION_0);
    }
}

// -----------------------------------------------------------------------------
// Runtime Setters
// -----------------------------------------------------------------------------

void LVGLDisplayModule::setBrightness(uint8_t brightness) {
    _brightness = brightness;
    _adapter.setBrightness(_brightness);
}

void LVGLDisplayModule::setRotation(uint16_t degrees) {
    if (degrees == 90 || degrees == 1) {
        _rotation = 90;
    } else if (degrees == 180 || degrees == 2) {
        _rotation = 180;
    } else if (degrees == 270 || degrees == 3) {
        _rotation = 270;
    } else {
        _rotation = 0;
    }

    if (_display != nullptr) {
        lv_display_rotation_t rotEnum = LV_DISPLAY_ROTATION_0;
        if (_rotation == 90) rotEnum = LV_DISPLAY_ROTATION_90;
        else if (_rotation == 180) rotEnum = LV_DISPLAY_ROTATION_180;
        else if (_rotation == 270) rotEnum = LV_DISPLAY_ROTATION_270;

        lv_display_set_rotation(_display, rotEnum);
    }
}

void LVGLDisplayModule::setScreensaverEnabled(bool enabled) {
    _screensaverEnabled = enabled;
}

void LVGLDisplayModule::setDimTimeout(unsigned long timeoutMs) {
    _dimTimeoutMs = timeoutMs;
}

void LVGLDisplayModule::setBlankTimeout(unsigned long timeoutMs) {
    _blankTimeoutMs = timeoutMs;
}

