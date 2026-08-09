/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/LVGLDisplay/LVGLDisplayModule.cpp
 * Description: Generic LVGL v9 colour-TFT display module core implementation.
 *              Manages LVGL lifecycle (lv_init, lv_tick_inc, lv_timer_handler),
 *              DMA partial draw buffer allocation, input device discovery, focus group wiring,
 *              system screens, ScreensaverController, and EventBus subscriptions.
 */

#include "LVGLDisplayModule.hpp"
#include "EventBus.hpp"

#include "ConfigManager.hpp"
#include "MCPToolRegistry.hpp"
#include "../Application/Application.hpp"
#include <base64.h>
#include <cstdio>
#include <cstring>
#include <PsychicHttp.h>
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "interfaces/IConfigGate.hpp"  // GateNames, GateActionStatus

// -----------------------------------------------------------------------------
// Weak Symbol Declaration & Default Implementation for SquareLine Studio (D8)
// -----------------------------------------------------------------------------
extern "C" __attribute__((weak)) void ui_init(void) {
    // Weak default no-op implementation. Downstream applications override this.
}

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

LVGLDisplayModule::LVGLDisplayModule(ILVGLHardwareAdapter& adapter,
                                     const BootBranding* branding,
                                     const lv_image_dsc_t* bootLogo,
                                     ILVGLSystemScreens* screens)
    : _adapter(adapter),
      _branding(branding),
      _screens(screens),
      _bootLogo(bootLogo ? bootLogo : &cb_labs_boot_logo),
      _builtInBootScreen(_bootLogo, _branding ? _branding->appTitle : nullptr) {
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
    if (!_buffersConfigured) {
        uint16_t width = _adapter.getScreenWidth();
        uint16_t lineCount = _adapter.getBufferLineCount();
        _bufSize = static_cast<size_t>(width) * lineCount * sizeof(lv_color_t);

        if (_bufSize > 0) {
            _buf1 = heap_caps_malloc(_bufSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
            _buf2 = heap_caps_malloc(_bufSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);

            if (_buf1 == nullptr || _buf2 == nullptr) {
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
                _buffersConfigured = true;
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

    // --- Step 5: Stage-1 Boot Splash ---
    showBoot();

    // --- Step 6: Apply Hardware Brightness & Rotation ---
    _adapter.setBrightness(_brightness);
    if (_rotation != 0) {
        lv_display_rotation_t rotEnum = LV_DISPLAY_ROTATION_0;
        if (_rotation == 90) rotEnum = LV_DISPLAY_ROTATION_90;
        else if (_rotation == 180) rotEnum = LV_DISPLAY_ROTATION_180;
        else if (_rotation == 270) rotEnum = LV_DISPLAY_ROTATION_270;
        lv_display_set_rotation(_display, rotEnum);
    }
}

bool LVGLDisplayModule::beginEarly() {
    bootstrapHardware();
    return true;
}

void LVGLDisplayModule::begin(EventBus* eventBus) {
    _eventBus = eventBus;

    // Phase 1 fallback: ensure bootstrapHardware() ran
    if (_display == nullptr) {
        bootstrapHardware();
    }

    // Subscribe to EventBus topics
    if (_eventBus != nullptr) {
#ifdef WITH_OTA
        _eventBus->subscribe(ApplicationEvents::OTA_PROGRESS, _onOtaProgress, this);
#endif
        _eventBus->subscribe(LVGLDisplayEvents::DISPLAY_MESSAGE, _onDisplayMessage, this);
        _eventBus->subscribe(LVGLDisplayEvents::SYSTEM_ERROR, _onSystemError, this);
        _eventBus->subscribe(ApplicationEvents::APP_CONFIGURING, _onAppConfiguring, this);
        _eventBus->subscribe(ApplicationEvents::APP_RUNNING, _onAppRunning, this);
    }

    // Sync screensaver settings
    _screensaver.setEnabled(_screensaverEnabled);
    _screensaver.setDimTimeout(_dimTimeoutMs);
    _screensaver.setBlankTimeout(_blankTimeoutMs);

    // Execute weak ui_init() hook if defined by application
    if (ui_init != nullptr) {
        ui_init();
    }

    _bootstrapMode = false;
    resetActivity();
}

// -----------------------------------------------------------------------------
// ITickable
// -----------------------------------------------------------------------------

void LVGLDisplayModule::tick(uint32_t now) {
    if (!_tickInitialized) {
        _tickInitialized = true;
        _lastTickMs = now;
        _screensaver.tick(now, _adapter, _brightness, _eventBus);
        return;
    }

    uint32_t elapsed = now - _lastTickMs;
    _lastTickMs = now;

    // Check LVGL input activity
    if (_display != nullptr) {
        uint32_t indevInactive = lv_display_get_inactive_time(_display);
        if (indevInactive < _lastIndevInactiveMs) {
            resetActivity();
        }
        _lastIndevInactiveMs = indevInactive;
    }

    // Tick Screensaver state machine
    _screensaver.tick(now, _adapter, _brightness, _eventBus);

    // If blanked, skip LVGL timer processing
    if (_screensaver.isBlanked()) {
        return;
    }

    // Drain pending cross-thread events.
    // OTA complete and update are checked independently (not else-if) so that
    // a completion arriving in the same tick as a progress update is never
    // deferred — the completion always wins and the stale progress is discarded.
    bool otaComplete = _pendingOtaComplete.exchange(false);
    bool otaUpdate   = _pendingOtaUpdate.exchange(false);

    if (otaComplete) {
        _pauseRendering.store(false);
        if (_pendingOtaSuccess.load()) {
            restorePriorScreen();
        } else {
            showError("OTA FAILED", _pendingOtaStatus[0] ? _pendingOtaStatus : "Update failed");
        }
        resetActivity();
    } else if (otaUpdate) {
        showOtaProgress(_pendingOtaProgress.load(), _pendingOtaStatus[0] ? _pendingOtaStatus : "Updating...");
        resetActivity();
    }

    if (_pendingError.exchange(false)) {
        showError(_pendingErrorTitle[0] ? _pendingErrorTitle : "SYSTEM ERROR", _pendingErrorMessage);
        resetActivity();
    }

    if (_pendingMessage.exchange(false)) {
        if (_bootstrapMode) {
            reportStatus(_pendingMessageText, -1.0f);
        } else {
            ESP_LOGI("LVGLDisplay", "Message: %s", _pendingMessageText);
            // TODO: Add transient notification overlay widget in a future PR
        }
        resetActivity();
    }

    // Defense-in-depth: if OTA flash write in progress, advance tick clock but skip timer handler
    if (_pauseRendering.load()) {
        lv_tick_inc(elapsed);
        return;
    }

    lv_tick_inc(elapsed);
    lv_timer_handler();
}

// -----------------------------------------------------------------------------
// IStatusReporter
// -----------------------------------------------------------------------------

void LVGLDisplayModule::reportStatus(const char* message, float progress) {
    _builtInBootScreen.setStatus(message, progress);
    if (lv_screen_active() != _builtInBootScreen.getScreen()) {
        showBoot();
    }
}

void LVGLDisplayModule::reportError(const char* errorMessage) {
    showError("FATAL ERROR", errorMessage);
}

void LVGLDisplayModule::clearStatus() {
    _pauseRendering.store(false);
    restorePriorScreen();
}

// -----------------------------------------------------------------------------
// System Screen Management API
// -----------------------------------------------------------------------------

void LVGLDisplayModule::showBoot() {
    _builtInBootScreen.init(_bootLogo, _branding ? _branding->appTitle : nullptr);
    lv_obj_t* bootScr = _builtInBootScreen.getScreen();
    if (bootScr != nullptr) {
        lv_scr_load_anim(bootScr, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false);
    }
}

void LVGLDisplayModule::showError(const char* title, const char* message) {
    if (!_inSystemScreen) {
        _priorScreen = lv_screen_active();
        _inSystemScreen = true;
    }

    if (_screens != nullptr) {
        _screens->onError(title, message);
        lv_obj_t* customScr = _screens->errorScreen();
        if (customScr != nullptr) {
            lv_scr_load_anim(customScr, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false);
            return;
        }
    }

    _builtInErrorScreen.setError(title, message);
    lv_obj_t* errScr = _builtInErrorScreen.getScreen();
    if (errScr != nullptr) {
        lv_scr_load_anim(errScr, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false);
    }
}

void LVGLDisplayModule::showError(int code, const char* message) {
    char titleBuf[32];
    snprintf(titleBuf, sizeof(titleBuf), "ERROR %d", code);
    showError(titleBuf, message);
}

void LVGLDisplayModule::showOtaProgress(float progress, const char* status) {
    if (!_inSystemScreen) {
        _priorScreen = lv_screen_active();
        _inSystemScreen = true;
    }

    if (_screens != nullptr) {
        _screens->onOtaProgress(progress, status);
        lv_obj_t* customScr = _screens->otaScreen();
        if (customScr != nullptr) {
            lv_scr_load_anim(customScr, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false);
            return;
        }
    }

    _builtInOtaScreen.setProgress(progress, status);
    lv_obj_t* otaScr = _builtInOtaScreen.getScreen();
    if (otaScr != nullptr) {
        lv_scr_load_anim(otaScr, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false);
    }
}

void LVGLDisplayModule::restorePriorScreen() {
    _pauseRendering.store(false);
    if (_inSystemScreen && _priorScreen != nullptr) {
        lv_scr_load_anim(_priorScreen, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false);
        _priorScreen = nullptr;
    }
    _inSystemScreen = false;
}

void LVGLDisplayModule::setBootLogo(const lv_image_dsc_t* bootLogo) {
    _bootLogo = bootLogo ? bootLogo : &cb_labs_boot_logo;
    _builtInBootScreen.setBootLogo(_bootLogo);
}

void LVGLDisplayModule::resetActivity() {
    _screensaver.resetActivity();
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

    _screensaver.setEnabled(_screensaverEnabled);
    _screensaver.setDimTimeout(_dimTimeoutMs);
    _screensaver.setBlankTimeout(_blankTimeoutMs);

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

    _screensaver.setEnabled(true);
    _screensaver.setDimTimeout(150000);
    _screensaver.setBlankTimeout(300000);

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
    _screensaver.setEnabled(enabled);
}

void LVGLDisplayModule::setDimTimeout(unsigned long timeoutMs) {
    _dimTimeoutMs = timeoutMs;
    _screensaver.setDimTimeout(timeoutMs);
}

void LVGLDisplayModule::setBlankTimeout(unsigned long timeoutMs) {
    _blankTimeoutMs = timeoutMs;
    _screensaver.setBlankTimeout(timeoutMs);
}

// -----------------------------------------------------------------------------
// Static EventBus Callbacks
// -----------------------------------------------------------------------------

#ifdef WITH_OTA
void LVGLDisplayModule::_onOtaProgress(const char* /*topic*/, const void* payload, void* ctx) {
    auto* self = static_cast<LVGLDisplayModule*>(ctx);
    if (payload == nullptr || self == nullptr) return;

    const auto* ota = static_cast<const ApplicationEvents::OtaProgressPayload*>(payload);
    if (ota->complete) {
        self->_pendingOtaSuccess.store(ota->success);
        if (ota->status) {
            strncpy(self->_pendingOtaStatus, ota->status, sizeof(self->_pendingOtaStatus) - 1);
            self->_pendingOtaStatus[sizeof(self->_pendingOtaStatus) - 1] = '\0';
        } else {
            self->_pendingOtaStatus[0] = '\0';
        }
        self->_pendingOtaComplete.store(true);
    } else {
        self->_pauseRendering.store(true);
        self->_pendingOtaProgress.store(ota->progress);
        if (ota->status) {
            strncpy(self->_pendingOtaStatus, ota->status, sizeof(self->_pendingOtaStatus) - 1);
            self->_pendingOtaStatus[sizeof(self->_pendingOtaStatus) - 1] = '\0';
        } else {
            self->_pendingOtaStatus[0] = '\0';
        }
        self->_pendingOtaUpdate.store(true);
    }
}
#endif

void LVGLDisplayModule::_onDisplayMessage(const char* /*topic*/, const void* payload, void* ctx) {
    auto* self = static_cast<LVGLDisplayModule*>(ctx);
    if (payload == nullptr || self == nullptr) return;

    const auto* msg = static_cast<const DisplayMessagePayload*>(payload);
    if (msg->message != nullptr) {
        strncpy(self->_pendingMessageText, msg->message, sizeof(self->_pendingMessageText) - 1);
        self->_pendingMessageText[sizeof(self->_pendingMessageText) - 1] = '\0';
        self->_pendingMessage.store(true);
    }
}

void LVGLDisplayModule::_onSystemError(const char* /*topic*/, const void* payload, void* ctx) {
    auto* self = static_cast<LVGLDisplayModule*>(ctx);
    if (payload == nullptr || self == nullptr) return;

    const auto* err = static_cast<const SystemErrorPayload*>(payload);
    if (err->message != nullptr) {
        strncpy(self->_pendingErrorMessage, err->message, sizeof(self->_pendingErrorMessage) - 1);
        self->_pendingErrorMessage[sizeof(self->_pendingErrorMessage) - 1] = '\0';
        
        if (err->title != nullptr) {
            strncpy(self->_pendingErrorTitle, err->title, sizeof(self->_pendingErrorTitle) - 1);
            self->_pendingErrorTitle[sizeof(self->_pendingErrorTitle) - 1] = '\0';
        } else {
            self->_pendingErrorTitle[0] = '\0';
        }
        self->_pendingError.store(true);
    }
}

void LVGLDisplayModule::_onAppConfiguring(const char* /*topic*/, const void* payload, void* ctx) {
    auto* self = static_cast<LVGLDisplayModule*>(ctx);
    if (!self || !payload) return;
    auto* p = static_cast<const AppConfiguringPayload*>(payload);
    ESP_LOGI("LVGLDisplayModule", "CONFIGURING gate: %s (status=%d)",
             p->gateName ? p->gateName : "?",
             static_cast<int>(p->actionStatus));

    // D2: LVGL has no built-in configuring screens — the skin (e.g. SLSSystemScreens)
    // supplies them. With no provider there is nothing to display.
    if (self->_screens == nullptr) {
        return;
    }

    // Let the provider populate its configuring widgets (AP SSID, portal URL,
    // gate help text) from the payload before we choose which root to show.
    self->_screens->onConfiguring(p);

    // Three-tier root selection, mirroring U8G2DisplayModule::_onAppConfiguring:
    // show a modal (gate helper / WiFi wizard) only when the gate needs user
    // action; otherwise show the base configuring screen (indeterminate wait).
    lv_obj_t* scr = nullptr;
    if (p->actionStatus == GateActionStatus::NeedsUserAction) {
        scr = self->_screens->gateHelperScreen(p);                          // Tier 1
        if (scr == nullptr && p->gateName != nullptr &&
            strcmp(p->gateName, GateNames::WIFI_STATION) == 0) {
            scr = self->_screens->wifiWizardScreen();                        // Tier 2
        }
        if (scr == nullptr) {
            scr = self->_screens->configuringScreen();                      // Tier 3
        }
    } else {
        scr = self->_screens->configuringScreen();                          // base / InProgress
    }

    // The provider toggles visibility groups in-place on the (single) System
    // screen, so only animate a screen-load on first entry — re-publishes that
    // stay on the same root must not re-trigger the fade.
    if (scr != nullptr && scr != lv_screen_active()) {
        if (!self->_inSystemScreen) {
            self->_priorScreen = lv_screen_active();
            self->_inSystemScreen = true;
        }
        lv_scr_load_anim(scr, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false);
    }
}

#if defined(__GNUC__) || defined(__clang__)
extern "C" {
    __attribute__((weak)) extern lv_obj_t* ui_ui_DiagScreen;
}
#endif

void LVGLDisplayModule::_onAppRunning(const char* /*topic*/, const void* /*payload*/, void* ctx) {
    auto* self = static_cast<LVGLDisplayModule*>(ctx);
    if (self == nullptr) return;
    ESP_LOGI("LVGLDisplayModule", "App transition → RUNNING");
    // Left the system screen (configuring/error/OTA) for the running UI.
    self->_inSystemScreen = false;
    self->_priorScreen = nullptr;
#if defined(__GNUC__) || defined(__clang__)
    if (&ui_ui_DiagScreen != nullptr && ui_ui_DiagScreen != nullptr) {
        lv_scr_load_anim(ui_ui_DiagScreen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
    }
#endif
}

// -----------------------------------------------------------------------------
// IWebContributor & Snapshot Primitive (WO7)
// -----------------------------------------------------------------------------

const char* LVGLDisplayModule::getTabIcon() const {
    return "<rect x=\"2\" y=\"3\" width=\"20\" height=\"14\" rx=\"2\" ry=\"2\"/>"
           "<line x1=\"8\" y1=\"21\" x2=\"16\" y2=\"21\"/>"
           "<line x1=\"12\" y1=\"17\" x2=\"12\" y2=\"21\"/>";
}

bool LVGLDisplayModule::captureSnapshot(uint8_t*& outBuffer, size_t& outSize, uint16_t& outWidth, uint16_t& outHeight) {
    if (_display != nullptr) {
        outWidth = static_cast<uint16_t>(lv_display_get_horizontal_resolution(_display));
        outHeight = static_cast<uint16_t>(lv_display_get_vertical_resolution(_display));
    } else {
        outWidth = _adapter.getScreenWidth();
        outHeight = _adapter.getScreenHeight();
    }
    if (outWidth == 0) outWidth = 240;
    if (outHeight == 0) outHeight = 240;

    outSize = static_cast<size_t>(outWidth) * static_cast<size_t>(outHeight) * 2UL;

    outBuffer = static_cast<uint8_t*>(heap_caps_malloc(outSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (outBuffer == nullptr) {
        outBuffer = static_cast<uint8_t*>(heap_caps_malloc(outSize, MALLOC_CAP_8BIT));
    }
    if (outBuffer == nullptr) {
        outSize = 0;
        return false;
    }

    lv_obj_t* screen = lv_screen_active();
    bool snapshot_taken = false;
    if (screen != nullptr) {
        // Use the non-deprecated lv_snapshot_take_to_draw_buf API (LVGL ≥9.2).
        // Wrap our raw PSRAM buffer in an lv_draw_buf_t so the snapshot renderer
        // can write directly into it without a separate allocation.
        lv_draw_buf_t draw_buf;
        uint32_t stride = (uint32_t)outWidth * (uint32_t)lv_color_format_get_size(LV_COLOR_FORMAT_RGB565);
        lv_draw_buf_init(&draw_buf, outWidth, outHeight, LV_COLOR_FORMAT_RGB565, stride, outBuffer, outSize);
        lv_result_t res = lv_snapshot_take_to_draw_buf(screen, LV_COLOR_FORMAT_RGB565, &draw_buf);
        if (res == LV_RESULT_OK) {
            snapshot_taken = true;
        }
    }

    if (!snapshot_taken) {
        memset(outBuffer, 0, outSize);
    }

    return true;
}

void LVGLDisplayModule::freeSnapshot(uint8_t* buffer) {
    if (buffer != nullptr) {
        heap_caps_free(buffer);
    }
}

void LVGLDisplayModule::registerRoutes(PsychicHttpServer& server) {
    // GET /api/display/status
    server.on("/api/display/status", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        JsonDocument doc;
        doc["brightness"]         = _brightness;
        doc["rotation"]           = _rotation;
        doc["screensaverEnabled"] = _screensaverEnabled;
        doc["dimmed"]             = _screensaver.isDimmed();
        doc["sleeping"]           = _screensaver.isBlanked();
        doc["dimTimeout"]         = _dimTimeoutMs;
        doc["blankTimeout"]       = _blankTimeoutMs;

        String output;
        serializeJson(doc, output);
        response->send(200, "application/json", output.c_str());
        return ESP_OK;
    });

    // POST /api/display/brightness
    server.on("/api/display/brightness", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        const char* body = request->bodyCStr();
        if (!body || strlen(body) == 0) {
            response->send(400, "application/json", "{\"error\":\"body_required\"}");
            return ESP_FAIL;
        }
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);
        if (err || !doc["value"].is<int>()) {
            response->send(400, "application/json", "{\"error\":\"Invalid JSON or missing 'value'\"}");
            return ESP_FAIL;
        }

        int val = doc["value"].as<int>();
        if (val < 0) val = 0;
        if (val > 255) val = 255;
        setBrightness(static_cast<uint8_t>(val));

        if (_configManager != nullptr) {
            _configManager->saveModuleConfig(this);
        }

        response->send(200, "application/json", "{\"status\":\"ok\"}");
        return ESP_OK;
    });

    // POST /api/display/rotation
    server.on("/api/display/rotation", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        const char* body = request->bodyCStr();
        if (!body || strlen(body) == 0) {
            response->send(400, "application/json", "{\"error\":\"body_required\"}");
            return ESP_FAIL;
        }
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);
        int rotVal = -1;
        if (doc["value"].is<int>()) {
            rotVal = doc["value"].as<int>();
        } else if (doc["rotation"].is<int>()) {
            rotVal = doc["rotation"].as<int>();
        }

        if (err || rotVal < 0) {
            response->send(400, "application/json", "{\"error\":\"Invalid JSON or missing rotation value\"}");
            return ESP_FAIL;
        }

        setRotation(static_cast<uint16_t>(rotVal));

        if (_configManager != nullptr) {
            _configManager->saveModuleConfig(this);
        }

        response->send(200, "application/json", "{\"status\":\"ok\"}");
        return ESP_OK;
    });

    // POST /api/display/flip
    server.on("/api/display/flip", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        const char* body = request->bodyCStr();
        if (!body || strlen(body) == 0) {
            response->send(400, "application/json", "{\"error\":\"body_required\"}");
            return ESP_FAIL;
        }
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);
        if (err || !doc["value"].is<bool>()) {
            response->send(400, "application/json", "{\"error\":\"Invalid JSON or missing 'value'\"}");
            return ESP_FAIL;
        }

        bool flip = doc["value"].as<bool>();
        setRotation(flip ? 180 : 0);

        if (_configManager != nullptr) {
            _configManager->saveModuleConfig(this);
        }

        response->send(200, "application/json", "{\"status\":\"ok\"}");
        return ESP_OK;
    });

    // POST /api/display/sleep
    server.on("/api/display/sleep", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        const char* body = request->bodyCStr();
        if (!body || strlen(body) == 0) {
            response->send(400, "application/json", "{\"error\":\"body_required\"}");
            return ESP_FAIL;
        }
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            response->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return ESP_FAIL;
        }

        bool needsPersist = false;

        if (doc["action"].is<const char*>()) {
            const char* action = doc["action"].as<const char*>();
            if (strcmp(action, "sleep") == 0) {
                _screensaver.forceTimeout();
            } else if (strcmp(action, "wake") == 0) {
                resetActivity();
            }
        }

        if (doc["enabled"].is<bool>()) {
            setScreensaverEnabled(doc["enabled"].as<bool>());
            needsPersist = true;
        }

        if (doc["dimTimeout"].is<unsigned long>()) {
            setDimTimeout(doc["dimTimeout"].as<unsigned long>());
            needsPersist = true;
        }

        if (doc["blankTimeout"].is<unsigned long>()) {
            setBlankTimeout(doc["blankTimeout"].as<unsigned long>());
            needsPersist = true;
        }

        if (doc["timeout"].is<unsigned long>()) {
            unsigned long st = doc["timeout"].as<unsigned long>();
            setDimTimeout(st / 2);
            setBlankTimeout(st);
            needsPersist = true;
        }

        if (needsPersist && _configManager != nullptr) {
            _configManager->saveModuleConfig(this);
        }

        response->send(200, "application/json", "{\"status\":\"ok\"}");
        return ESP_OK;
    });

    // GET /api/display/screenshot
    server.on("/api/display/screenshot", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        uint8_t* buf = nullptr;
        size_t size = 0;
        uint16_t w = 0;
        uint16_t h = 0;

        if (!captureSnapshot(buf, size, w, h) || buf == nullptr) {
            response->send(500, "application/json", "{\"error\":\"Framebuffer unavailable\"}");
            return ESP_FAIL;
        }

        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        response->addHeader("Pragma", "no-cache");
        response->addHeader("Expires", "0");
        response->addHeader("X-Display-Width", String(static_cast<int>(w)).c_str());
        response->addHeader("X-Display-Height", String(static_cast<int>(h)).c_str());
        response->addHeader("X-Display-Format", "RGB565");

        response->send(200, "application/octet-stream", buf, size);
        freeSnapshot(buf);
        return ESP_OK;
    });
}

// =============================================================================
// IMCPContributor — MCP display tools
// =============================================================================

/**
 * @brief Register display_status, display_control, and capture_display MCP tools.
 *
 * @param registry The MCPToolRegistry to register tools with.
 */
void LVGLDisplayModule::registerMCPTools(MCPToolRegistry& registry) {
    // --- Tool 1: display_status (read-only) ---
    registry.addTool(
        "display_status",
        "Get current display state including brightness, rotation, screensaver status, and resolution",
        [](void* ctx, JsonObject& params, JsonObject& result) {
            auto* self = static_cast<LVGLDisplayModule*>(ctx);
            (void)params;

            result["brightness"]         = self->_brightness;
            result["rotation"]           = self->_rotation;
            result["screensaverEnabled"] = self->_screensaverEnabled;

            // --- Screensaver sub-object ---
            JsonObject ss = result["screensaver"].to<JsonObject>();
            ss["enabled"] = self->_screensaverEnabled;
            if (self->_screensaver.isBlanked()) {
                ss["state"] = "blanked";
            } else if (self->_screensaver.isDimmed()) {
                ss["state"] = "dimmed";
            } else {
                ss["state"] = "active";
            }
            ss["dimTimeout"]   = self->_dimTimeoutMs;
            ss["blankTimeout"] = self->_blankTimeoutMs;

            // --- Resolution sub-object ---
            JsonObject res = result["resolution"].to<JsonObject>();
            if (self->_display != nullptr) {
                res["width"]  = static_cast<int>(lv_display_get_horizontal_resolution(self->_display));
                res["height"] = static_cast<int>(lv_display_get_vertical_resolution(self->_display));
            } else {
                res["width"]  = static_cast<int>(self->_adapter.getScreenWidth());
                res["height"] = static_cast<int>(self->_adapter.getScreenHeight());
            }

            // --- Active screen info ---
            if (self->_inSystemScreen) {
                result["activeScreen"] = "system";
            } else if (self->_screensaver.isBlanked() || self->_screensaver.isDimmed()) {
                result["activeScreen"] = "screensaver";
            } else {
                result["activeScreen"] = "application";
            }
        },
        this,
        /* mutatesState = */ false
    );

    // --- Tool 2: display_control (write, mutates state) ---
    JsonDocument controlSchema;
    controlSchema["type"] = "object";
    JsonObject props = controlSchema["properties"].to<JsonObject>();

    props["action"]["type"]        = "string";
    props["action"]["description"] = "Immediate action: 'wake' to reset inactivity, 'sleep' to force screensaver";
    JsonArray actionEnum = props["action"]["enum"].to<JsonArray>();
    actionEnum.add("wake");
    actionEnum.add("sleep");

    props["brightness"]["type"]        = "integer";
    props["brightness"]["description"] = "Display brightness level (0-255)";
    props["brightness"]["minimum"]     = 0;
    props["brightness"]["maximum"]     = 255;

    props["rotation"]["type"]        = "integer";
    props["rotation"]["description"] = "Display rotation in degrees (0, 90, 180, 270)";

    props["screensaverEnabled"]["type"]        = "boolean";
    props["screensaverEnabled"]["description"] = "Enable or disable the screensaver";

    props["dimTimeout"]["type"]        = "integer";
    props["dimTimeout"]["description"] = "Milliseconds of inactivity before dimming";

    props["blankTimeout"]["type"]        = "integer";
    props["blankTimeout"]["description"] = "Milliseconds of inactivity before blanking";

    registry.addTool(
        "display_control",
        "Control display settings: wake/sleep, enable/disable screensaver, set brightness, rotation, and timeouts",
        controlSchema,
        [](void* ctx, JsonObject& params, JsonObject& result) {
            auto* self = static_cast<LVGLDisplayModule*>(ctx);
            bool needsPersist = false;

            if (params["action"].is<const char*>()) {
                const char* action = params["action"].as<const char*>();
                if (strcmp(action, "wake") == 0) {
                    self->resetActivity();
                } else if (strcmp(action, "sleep") == 0) {
                    self->_screensaver.forceTimeout();
                }
            }

            if (params["brightness"].is<int>()) {
                int val = params["brightness"].as<int>();
                if (val < 0) val = 0;
                if (val > 255) val = 255;
                self->setBrightness(static_cast<uint8_t>(val));
                needsPersist = true;
            }

            if (params["rotation"].is<int>()) {
                int val = params["rotation"].as<int>();
                self->setRotation(static_cast<uint16_t>(val));
                needsPersist = true;
            }

            if (params["screensaverEnabled"].is<bool>()) {
                self->setScreensaverEnabled(params["screensaverEnabled"].as<bool>());
                needsPersist = true;
            }

            if (params["dimTimeout"].is<int>()) {
                self->setDimTimeout(static_cast<unsigned long>(params["dimTimeout"].as<int>()));
                needsPersist = true;
            }

            if (params["blankTimeout"].is<int>()) {
                self->setBlankTimeout(static_cast<unsigned long>(params["blankTimeout"].as<int>()));
                needsPersist = true;
            }

            if (needsPersist && self->_configManager != nullptr) {
                self->_configManager->saveModuleConfig(self);
            }

            result["status"] = "ok";
        },
        this,
        /* mutatesState = */ true
    );

    // --- Tool 3: capture_display (read-only) ---
    registry.addTool(
        "capture_display",
        "Capture a screenshot of the current display as base64-encoded RGB565 data",
        [](void* ctx, JsonObject& params, JsonObject& result) {
            auto* self = static_cast<LVGLDisplayModule*>(ctx);
            (void)params;

            uint8_t* buf = nullptr;
            size_t size = 0;
            uint16_t w = 0;
            uint16_t h = 0;

            if (!self->captureSnapshot(buf, size, w, h) || buf == nullptr) {
                result["error"] = "Failed to allocate snapshot buffer";
                return;
            }

            // Base64-encode the RGB565 framebuffer.
            // Pre-allocate output buffer: base64 expands 3 bytes → 4 chars, plus padding.
            // ~153 KB for a 240×240 RGB565 frame. Transient PSRAM allocation.
            size_t b64Len = ((size + 2) / 3) * 4;
            char* b64Buf = static_cast<char*>(heap_caps_malloc(b64Len + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
            if (b64Buf == nullptr) {
                b64Buf = static_cast<char*>(malloc(b64Len + 1));
            }
            if (b64Buf == nullptr) {
                self->freeSnapshot(buf);
                result["error"] = "Failed to allocate base64 buffer";
                return;
            }

            size_t outPos = 0;
            size_t offset = 0;
            while (offset < size) {
                size_t chunk = (size - offset > 48) ? 48 : (size - offset);
                String b64Chunk = base64::encode(&buf[offset], chunk);
                size_t chunkLen = b64Chunk.length();
                memcpy(b64Buf + outPos, b64Chunk.c_str(), chunkLen);
                outPos += chunkLen;
                offset += chunk;
            }
            b64Buf[outPos] = '\0';

            result["format"] = "RGB565";
            result["width"]  = w;
            result["height"] = h;
            result["data"]   = b64Buf;

            free(b64Buf);
            self->freeSnapshot(buf);
        },
        this,
        /* mutatesState = */ false
    );
}



