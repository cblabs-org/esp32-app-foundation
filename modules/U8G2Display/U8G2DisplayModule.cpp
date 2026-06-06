/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/U8G2Display/U8G2DisplayModule.cpp
 * Description: Implementation of the U8G2 display module — pure service with
 *              three-layer rendering (Modal → Base/Carousel → Screensaver),
 *              carousel engine, sleep/wake power management, IStatusReporter
 *              delegation (Boot/Error boards), EventBus subscriptions
 *              (ota.progress, display.message), EventBus publishing
 *              (display.sleep, display.wake, display.board.changed),
 *              IWebContributor REST API under /api/display/,
 *              IMCPContributor tools (display_status, display_control), and
 *              IConfigurable persistence at /mod/display.json.
 *
 * Exported Functions:
 * - U8G2DisplayModule::bootstrapHardware(): Phase 1 hardware init
 * - U8G2DisplayModule::begin():             Phase 2 framework integration
 * - U8G2DisplayModule::tick():              Screensaver, modal expiry, carousel, render
 * - U8G2DisplayModule::showBase():          Set persistent base board
 * - U8G2DisplayModule::showModal():         Push modal overlay board
 * - U8G2DisplayModule::clearModal():        Clear active modal
 * - U8G2DisplayModule::resetActivity():     Reset inactivity timer, wake if sleeping
 * - U8G2DisplayModule::visibleBoard():      Return currently rendered board
 * - U8G2DisplayModule::reportStatus():      Delegate to BootBoard, show it
 * - U8G2DisplayModule::reportError():       Delegate to ErrorBoard (fatal), show it
 * - U8G2DisplayModule::clearStatus():       Mark loading complete, advance if showing
 * - U8G2DisplayModule::registerRoutes():    Register /api/display/* REST endpoints
 * - U8G2DisplayModule::registerMCPTools():  Register display_status/display_control MCP tools
 * - U8G2DisplayModule::applyConfig():       Apply display settings from JSON section
 * - U8G2DisplayModule::serializeConfig():   Serialize display settings to JSON section
 * - U8G2DisplayModule::resetConfig():       Reset to factory defaults
 * - U8G2DisplayModule::setBrightness():     Runtime brightness adjustment
 * - U8G2DisplayModule::setFlipScreen():     Runtime screen flip toggle
 * - U8G2DisplayModule::setSleepTimeout():   Runtime sleep timeout adjustment
 * - U8G2DisplayModule::setRotationInterval(): Runtime carousel interval adjustment
 * - U8G2DisplayModule::setTargetFps():      Runtime frame-rate target adjustment
 * - U8G2DisplayModule::_onOtaProgress():    Static EventBus callback for OTA events
 * - U8G2DisplayModule::_onDisplayMessage(): Static EventBus callback for display messages
 */

#include "U8G2DisplayModule.hpp"
#include "platform/LogShim.h"
#include <cstring>

static const char* TAG = "U8g2Display";

#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "../Application/Application.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
  #if defined(DISPLAY_SPI_SCK) && defined(DISPLAY_SPI_MISO) && defined(DISPLAY_SPI_MOSI)
  #include <SPI.h>
  #endif
#endif

// Include EventBus directly — it is native-safe (no Arduino.h dependency).
#include "../EventBus/EventBus.hpp"
#include "../ConfigManager/ConfigManager.hpp"
#include "../MCPServer/MCPToolRegistry.hpp"
#include "fonts/Fonts.hpp"

// =============================================================================
// Lifecycle
// =============================================================================

/**
 * @brief Phase 1: Bootstrap display hardware with zero framework dependencies.
 *
 * Initializes SPI bus and U8G2 driver, activates the boot splash, and renders
 * it immediately. Sets bootstrap mode so that reportStatus() performs inline
 * blocking animations during the subsequent app.begin() call.
 *
 * Safe to call before any framework service (EventBus, ConfigManager, etc.)
 * is available. Call begin(app) after app.begin() to complete integration.
 */
void U8G2DisplayModule::bootstrapHardware() {
#ifndef NATIVE_BUILD
    // --- Step 1: Initialize SPI bus ---
    // If explicit SPI pins are defined, secure the SPI bus before U8G2 init.
    // CRITICAL: On ESP32-S3, the default Arduino pin mapping can collide with
    // the internal Flash/PSRAM bus. Explicit pins prevent watchdog crashes.
  #if defined(DISPLAY_SPI_SCK) && defined(DISPLAY_SPI_MISO) && defined(DISPLAY_SPI_MOSI)
    ESP_LOGI(TAG, "Bootstrap: SPI.begin(SCK=%d, MISO=%d, MOSI=%d)",
             DISPLAY_SPI_SCK, DISPLAY_SPI_MISO, DISPLAY_SPI_MOSI);
    SPI.begin(DISPLAY_SPI_SCK, DISPLAY_SPI_MISO, DISPLAY_SPI_MOSI, -1);
  #else
    ESP_LOGW(TAG, "Bootstrap: No explicit SPI pins — using Arduino defaults");
    SPI.begin();
  #endif

    // --- Step 2: Initialize U8G2 driver ---
  #ifdef DISPLAY_CS_PIN
    ESP_LOGI(TAG, "Bootstrap: U8G2 init (CS=%d, DC=%d, RST=%d)",
             DISPLAY_CS_PIN, DISPLAY_DC_PIN, DISPLAY_RST_PIN);
  #else
    ESP_LOGI(TAG, "Bootstrap: U8G2 init (I2C mode, no pin defines)");
  #endif

    _u8g2.begin();

    // --- Step 3: Configure draw mode ---
    // These calls match the departures-board's proven init sequence.
    // setDrawColor(1) is critical — without it the display may draw
    // black-on-black (invisible) on some SSD13xx controllers.
    _u8g2.setDrawColor(1);
    _u8g2.setFontMode(1);
    _u8g2.setFontRefHeightAll();
    _u8g2.setFontPosTop();
    _u8g2.setContrast(0); // Start dark — fade-in will ramp to _brightness
    _u8g2.setFlipMode(_flipScreen ? 1 : 0);

    ESP_LOGI(TAG, "Bootstrap: U8G2 driver ready (brightness=%u, flip=%s)",
             _brightness, _flipScreen ? "yes" : "no");

    // --- Step 4: Show boot splash with fade-in ---
    _bootBoard.activate(BUILD_VERSION);
    showModal(&_bootBoard);
    _renderCurrentBoard();

    // --- Step 4b: Fade-in over first 1 second, then hold for 2 more ---
    // Ramp display contrast from 0 → _brightness across ~30 steps (33ms each)
    // for a smooth fade-in effect. Then hold the remaining 2 seconds.
    {
        const unsigned long kFadeDurationMs = 1000;
        const unsigned long kHoldDurationMs = 2000;
        const int kFadeSteps = 250;
        const unsigned long kStepMs = 4;

        for (int i = 1; i <= kFadeSteps; i++) {
            uint8_t level = static_cast<uint8_t>(
                (static_cast<unsigned long>(_brightness) * i) / kFadeSteps
            );
            _u8g2.setContrast(level);
            delay(kStepMs);
        }
        // Ensure we land exactly on the target brightness
        _u8g2.setContrast(_brightness);
        delay(kHoldDurationMs);
    }
#endif

    // --- Step 5: Enter bootstrap mode ---
    // reportStatus() will render inline until begin(app) exits bootstrap mode.
    _bootstrapMode = true;
    _lastBootProgress = 0.0f;
    ESP_LOGI(TAG, "Bootstrap: display hardware ready, inline rendering enabled");
}

/**
 * @brief Phase 2: Complete framework integration.
 *
 * Subscribes to EventBus topics, locks the board pool, and exits bootstrap
 * mode. If bootstrapHardware() was already called, skips hardware init.
 * Otherwise performs full hardware + framework init in one step.
 *
 * @param app Reference to the Application composition root.
 */
void U8G2DisplayModule::begin(EventBus* eventBus) {
    _eventBus = eventBus;
    
    _screensaver.setBoard(&_screensaverBoard);

    // --- Step 1: Initialize hardware (skip if already bootstrapped) ---
    if (!_bootstrapMode) {
#ifndef NATIVE_BUILD
      #if defined(DISPLAY_SPI_SCK) && defined(DISPLAY_SPI_MISO) && defined(DISPLAY_SPI_MOSI)
        SPI.begin(DISPLAY_SPI_SCK, DISPLAY_SPI_MISO, DISPLAY_SPI_MOSI, -1);
      #endif

        ESP_LOGI(TAG, "Initializing hardware display driver");
        _u8g2.begin();
        _u8g2.setContrast(_brightness);
        _u8g2.setFlipMode(_flipScreen ? 1 : 0);
#endif
    } else {
        ESP_LOGI(TAG, "Hardware already bootstrapped — skipping SPI/U8G2 init");
    }

    // --- Step 2: Subscribe to EventBus topics ---
    // Guarded: getEventBus() requires Application.hpp (unavailable in native)
#ifndef NATIVE_BUILD
    if (_eventBus != nullptr) {
        ESP_LOGI(TAG, "Subscribing to EventBus topics: ota.progress, display.message");
        _eventBus->subscribe("ota.progress", _onOtaProgress, this);
        _eventBus->subscribe("display.message", _onDisplayMessage, this);
    }
#endif

    // --- Step 3: Activate boot splash (skip if bootstrapped — already shown) ---
    if (!_bootstrapMode) {
        _bootBoard.activate(BUILD_VERSION);
        showModal(&_bootBoard);
    }

    // --- Step 5: Exit bootstrap mode ---
    // From this point, tick() handles rendering via the main loop.
    _bootstrapMode = false;
    _lastBootProgress = 0.0f;

    // --- Step 6: Start the inactivity timer
#ifndef NATIVE_BUILD
    resetActivity();
    _lastRenderMs   = millis();
#endif

    ESP_LOGI(TAG, "Display module fully initialized — tick-driven rendering active");
}

/**
 * @brief Periodic tick — two-layer base+modal architecture.
 *
 * Called by the Application tick loop. If sleeping, returns early.
 * Flow:
 *   1. Tick screensaver (dim/blank/wake)
 *   2. Auto-clear expired modals
 *   3. Tick Carousel
 *   4. Determine visible board (Modal → Base → Screensaver → Carousel)
 *   5. Render visible board
 */
void U8G2DisplayModule::tick() {
    // --- 1. Tick Screensaver ---
#ifndef NATIVE_BUILD
    unsigned long currentMs = millis();
    bool wasBlanked = _screensaver.isBlanked();
    _screensaver.tick(currentMs, _u8g2, _brightness);
    bool isBlanked = _screensaver.isBlanked();

    if (wasBlanked != isBlanked && _eventBus != nullptr) {
        _eventBus->publish(isBlanked ? "display.sleep" : "display.wake");
    }
    
    // If sleeping, skip carousel and rendering
    if (isBlanked) {
        return;
    }
#endif

    // --- 2. Auto-clear expired modals ---
    if (_modal != nullptr && _modal->isExpired()) {
        clearModal();
    }

    // --- 3. Tick Carousel ---
#ifndef NATIVE_BUILD
    _carousel.tick(currentMs);
#endif

    // --- 4. Determine visible board ---
    // Priority: Modal (highest) → Base → Screensaver (idle) → Carousel (lowest)
    // Modal always wins so OTA/error overlays are never hidden.
    // Base board is an explicit app-driven override (e.g. configuring status).
    // Screensaver activates on inactivity, overriding the carousel.
    // Carousel is the default background rotation.
    IDisplayBoard* visible = nullptr;
    if (_modal != nullptr) {
        visible = _modal;
    } else if (_baseBoard != nullptr) {
        visible = _baseBoard;
    } else if (_screensaver.isActive() && _screensaver.board() != nullptr) {
        visible = _screensaver.board();
    } else {
        visible = _carousel.currentBoard();
    }

    // --- Guard: nothing to render ---
    if (visible == nullptr) {
        return;
    }

    // --- 5. Tick + render the visible board (frame-rate limited) ---
#ifndef NATIVE_BUILD
    visible->tick(millis());
    unsigned long now = millis();
    if ((now - _lastRenderMs) >= _targetFrameIntervalMs) {
        _renderCurrentBoard();
        _lastRenderMs = now;
    }
#else
    visible->tick(0);
    _renderCurrentBoard();
#endif
}

void U8G2DisplayModule::showBase(IDisplayBoard* board) {
    IDisplayBoard* previousBoard = visibleBoard();
    if (_baseBoard != nullptr && _baseBoard != board) {
        _baseBoard->onDeactivate();
    }
    _baseBoard = board;
    if (board != nullptr) {
        board->onActivate();
    }
    
    // Only fire event if the visible board actually changed
    if (previousBoard != visibleBoard() && _eventBus != nullptr) {
        _eventBus->publish("display.board.changed");
    }
}


/**
 * @brief Push a modal overlay board onto the display.
 *
 * Deactivates any existing modal, activates the new one, resets the
 * inactivity timer and dim state. The modal auto-clears when isExpired()
 * returns true (checked in tick()).
 *
 * @param board Pointer to the modal board to display.
 */
void U8G2DisplayModule::showModal(IDisplayBoard* board) {
    if (_modal != nullptr && _modal != board) {
        _modal->onDeactivate();
    }
    _modal = board;
    if (board != nullptr) {
        board->onActivate();
    }
    _screensaver.resetActivity();
}

/**
 * @brief Manually clear the active modal overlay.
 *
 * Deactivates the current modal, restores brightness to the configured
 * level, and returns rendering to the phase-determined base layer.
 */
void U8G2DisplayModule::clearModal() {
    if (_modal != nullptr) {
        _modal->onDeactivate();
        _modal = nullptr;
#ifndef NATIVE_BUILD
        _u8g2.setContrast(_brightness);
#endif
    }
}

/**
 * @brief Reset the inactivity timer. If sleeping, wake the display.
 *
 * This is the public API for signalling user-initiated activity (HTTP
 * requests, touch events, EventBus interactions). It resets the timer,
 * wakes from sleep, and dismisses the screensaver if active.
 */
void U8G2DisplayModule::resetActivity() {
    _screensaver.resetActivity();
}

/**
 * @brief Return the board currently being rendered, considering all layers.
 *
 * Priority: modal → base → carousel → screensaver.
 * Returns nullptr if no board is active on any layer.
 *
 * @return Pointer to the visible IDisplayBoard, or nullptr.
 */
IDisplayBoard* U8G2DisplayModule::visibleBoard() const {
    // Priority: Modal → Base → Screensaver (idle) → Carousel
    if (_modal != nullptr) {
        return _modal;
    }
    if (_baseBoard != nullptr) {
        return _baseBoard;
    }
    if (_screensaver.isActive() && _screensaver.board() != nullptr) {
        return _screensaver.board();
    }
    return _carousel.currentBoard();
}



// =============================================================================
// IStatusReporter — real implementations delegating to system boards
// =============================================================================

/**
 * @brief Report status by delegating to BootBoard.
 *
 * Updates the loading board's message and progress, shows the loading board
 * if it isn't already the active board, and resets the inactivity timer.
 *
 * In bootstrap mode (before begin()), performs a blocking inline animation
 * that smoothly interpolates the progress bar from the previous value to
 * the new value over ~150ms. Each frame is rendered directly to the display.
 *
 * @param message  Status text for the loading board.
 * @param progress Fractional progress [0.0, 1.0]; negative = indeterminate.
 */
void U8G2DisplayModule::reportStatus(const char* message, float progress) {
    if (visibleBoard() != &_bootBoard) {
        showModal(&_bootBoard);
    }

    if (_bootstrapMode) {
        // --- Inline blocking animation during boot ---
        // Smoothly interpolate progress bar from previous to new value.
        // ~10 frames × 15ms = 150ms per milestone for premium boot UX.
#ifndef NATIVE_BUILD
        float delta = progress - _lastBootProgress;
        if (delta > 0.005f) {
            const int kSteps = 10;
            const int kStepDelayMs = 15;
            for (int i = 1; i <= kSteps; i++) {
                float p = _lastBootProgress + (delta * (float)i / (float)kSteps);
                _bootBoard.setStatus(message, p);
                _renderCurrentBoard();
                delay(kStepDelayMs);
            }
        } else {
            _bootBoard.setStatus(message, progress);
            _renderCurrentBoard();
        }
#else
        _bootBoard.setStatus(message, progress);
#endif
        _lastBootProgress = progress;
    } else {
        // --- Normal mode: update state, tick() renders ---
        _bootBoard.setStatus(message, progress);
        resetActivity();
    }
}

/**
 * @brief Report a fatal error by delegating to ErrorBoard.
 *
 * Displays the error as fatal (no auto-dismiss), forces the error board
 * onto the screen, and resets the inactivity timer. In bootstrap mode,
 * renders the error board immediately (no tick loop available).
 *
 * @param errorMessage Error description text.
 */
void U8G2DisplayModule::reportError(const char* errorMessage) {
    // --- Delegate to ErrorBoard with fatal flag ---
    _error.showError(errorMessage, true);

    // --- Force-show the error board ---
    showModal(&_error);

    // --- Immediate render in bootstrap mode (no tick loop yet) ---
    if (_bootstrapMode) {
        _renderCurrentBoard();
    }

    resetActivity();
}

/**
 * @brief Clear the active status overlay.
 *
 * Marks the BootBoard as complete (triggers isExpired()). If the boot
 * board is currently the active board, advances the carousel to move past it.
 * In bootstrap mode, renders the resulting board immediately.
 */
void U8G2DisplayModule::clearStatus() {
    _bootBoard.setComplete();

    // --- If currently showing the boot board, force carousel advance ---
    // Manually clear the modal (boot board was shown via showModal) so the
    // carousel base layer becomes visible on the next tick.
    if (visibleBoard() == &_bootBoard) {
        clearModal();
    }

    // --- Immediate render in bootstrap mode (show whatever board advanced to) ---
    if (_bootstrapMode) {
        _renderCurrentBoard();
    }
}

// =============================================================================
// IConfigurable — ConfigManager gateway (namespace: "display")
// =============================================================================

/**
 * @brief Apply display settings from a parsed JSON section.
 *
 * Called by ConfigManager during beginAll(). Reads brightness, flip,
 * screensaverEnabled, dimTimeout, blankTimeout, rotationInterval, and
 * targetFps from the JSON object and applies them to internal state
 * and hardware.
 *
 * Provides migration from the legacy single-key "sleepTimeout" config:
 * if "sleepTimeout" is present and the new keys are absent, derives
 * dimTimeout = sleepTimeout / 2 and blankTimeout = sleepTimeout.
 *
 * @param section Read-only JSON object containing display config keys.
 */
void U8G2DisplayModule::applyConfig(const JsonObject& section) {
    _brightness         = section["brightness"]      | (uint8_t)255;
    _flipScreen         = section["flip"]            | false;

    // --- Screensaver settings (with legacy migration) ---
    bool screensaverEnabled = section["screensaverEnabled"] | true;
    _screensaver.setEnabled(screensaverEnabled);

    if (section.containsKey("dimTimeout")) {
        // --- New-style config: independent timeouts ---
        unsigned long dimTimeoutMs   = section["dimTimeout"]   | (unsigned long)150000;
        unsigned long blankTimeoutMs = section["blankTimeout"]  | (unsigned long)300000;
        _screensaver.setDimTimeout(dimTimeoutMs);
        _screensaver.setBlankTimeout(blankTimeoutMs);
    } else if (section.containsKey("sleepTimeout")) {
        // --- Migration: derive from legacy sleepTimeout ---
        unsigned long sleepTimeoutMs = section["sleepTimeout"] | (unsigned long)300000;
        _screensaver.setDimTimeout(sleepTimeoutMs / 2);
        _screensaver.setBlankTimeout(sleepTimeoutMs);
    } else {
        // --- Defaults (no config present) ---
        _screensaver.setDimTimeout(150000);
        _screensaver.setBlankTimeout(300000);
    }

    unsigned long rotationInterval = section["rotationInterval"] | (unsigned long)8000;
    _carousel.setInterval(rotationInterval);
    _targetFrameIntervalMs = 1000UL / (section["targetFps"] | (uint8_t)30);
    _clockFontSize      = section["clockFontSize"]    | (uint8_t)2;

    const uint8_t* font = AppFonts::defaultMedium();
    if (_clockFontSize == 1) font = AppFonts::defaultSmall();
    else if (_clockFontSize == 3) font = AppFonts::defaultLarge();
    _screensaverBoard.setClockFont(font);

    // --- Apply hardware settings ---
#ifndef NATIVE_BUILD
    _u8g2.setContrast(_brightness);
    _u8g2.setFlipMode(_flipScreen ? 1 : 0);
#endif

    ESP_LOGI(TAG, "Config applied (brightness=%d, flip=%d, screensaver=%d, dim=%lu, blank=%lu)",
             _brightness, _flipScreen, screensaverEnabled,
             _screensaver.getDimTimeout(), _screensaver.getBlankTimeout());
}

/**
 * @brief Serialize current display settings into a JSON section.
 *
 * Called by ConfigManager during saveModuleConfig(). Writes the display
 * config keys (brightness, flip, screensaverEnabled, dimTimeout,
 * blankTimeout, rotationInterval, targetFps, clockFontSize) into the
 * provided JSON object. ConfigManager appends the _v field separately.
 *
 * @param section Mutable JSON object to write config into.
 */
void U8G2DisplayModule::serializeConfig(JsonObject& section) const {
    section["brightness"]          = _brightness;
    section["flip"]                = _flipScreen;
    section["screensaverEnabled"]  = _screensaver.isEnabled();
    section["dimTimeout"]          = _screensaver.getDimTimeout();
    section["blankTimeout"]        = _screensaver.getBlankTimeout();
    section["rotationInterval"]    = _carousel.getInterval();
    section["targetFps"]           = (uint8_t)(1000UL / _targetFrameIntervalMs);
    section["clockFontSize"]       = _clockFontSize;
}

/**
 * @brief Reset display settings to factory defaults.
 *
 * Sets: brightness=255, flipScreen=false, screensaverEnabled=true,
 * dimTimeout=150000ms, blankTimeout=300000ms, rotationInterval=8000ms,
 * targetFrameIntervalMs=33ms (30 FPS). Applies hardware settings.
 * ConfigManager will call saveModuleConfig() afterwards to persist.
 */
void U8G2DisplayModule::resetConfig() {
    // --- Set factory defaults ---
    _brightness         = 255;
    _flipScreen         = false;
    _screensaver.setEnabled(true);
    _screensaver.setDimTimeout(150000);
    _screensaver.setBlankTimeout(300000);
    _carousel.setInterval(8000);
    _targetFrameIntervalMs = 33;
    _clockFontSize      = 2;

    // --- Apply hardware settings ---
#ifndef NATIVE_BUILD
    _u8g2.setContrast(_brightness);
    _u8g2.setFlipMode(_flipScreen ? 1 : 0);
#endif
}

// =============================================================================
// MCP display info accessor
// =============================================================================

/**
 * @brief Return a DisplayInfo struct with the live U8G2 framebuffer pointer.
 *
 * The buffer pointer remains valid for the lifetime of the U8G2 instance
 * (i.e. the entire program). Size is calculated from tile dimensions.
 *
 * @return Populated DisplayInfo (framebuffer is nullptr in native builds).
 */
DisplayInfo U8G2DisplayModule::getDisplayInfo() {
    DisplayInfo info;
#ifndef NATIVE_BUILD
    info.framebuffer     = _u8g2.getBufferPtr();
    info.framebufferSize = 8u * _u8g2.getBufferTileHeight() * _u8g2.getBufferTileWidth();
    info.width           = _displayWidth;
    info.height          = _displayHeight;
#endif
    return info;
}

// =============================================================================
// Runtime display configuration setters
// =============================================================================

/**
 * @brief Set brightness and apply immediately to hardware.
 * @param brightness Display contrast/brightness level (0–255).
 */
void U8G2DisplayModule::setBrightness(uint8_t brightness) {
    _brightness = brightness;
#ifndef NATIVE_BUILD
    _u8g2.setContrast(_brightness);
#endif
}

/**
 * @brief Set the screen flip mode and apply immediately.
 * @param flip True to flip the display 180 degrees.
 */
void U8G2DisplayModule::setFlipScreen(bool flip) {
    _flipScreen = flip;
#ifndef NATIVE_BUILD
    _u8g2.setFlipMode(_flipScreen ? 1 : 0);
#endif
}

/**
 * @brief Set the inactivity sleep timeout.
 * @param timeoutMs Timeout in milliseconds (0 disables sleep).
 */
void U8G2DisplayModule::setSleepTimeout(unsigned long timeoutMs) {
    _screensaver.setBlankTimeout(timeoutMs);
}

/**
 * @brief Set the carousel rotation interval.
 * @param intervalMs Interval in milliseconds between board rotations.
 */
void U8G2DisplayModule::setRotationInterval(unsigned long intervalMs) {
    _carousel.setInterval(intervalMs);
}

/**
 * @brief Set the target render frame rate.
 *
 * Converts the FPS value to a millisecond interval stored in
 * _targetFrameIntervalMs. Zero is treated as 30 FPS to prevent
 * division-by-zero.
 *
 * @param fps Target frames per second (1–60). 0 treated as 30.
 */
void U8G2DisplayModule::setTargetFps(uint8_t fps) {
    _targetFrameIntervalMs = (fps > 0) ? (1000UL / fps) : 33;
}

/**
 * @brief Set the clock font size.
 * @param size Font size level (1=small, 2=medium, 3=large). Clamped to range.
 */
void U8G2DisplayModule::setClockFontSize(uint8_t size) {
    if (size < 1) size = 1;
    if (size > 3) size = 3;
    _clockFontSize = size;
    
    const uint8_t* font = AppFonts::defaultMedium();
    if (_clockFontSize == 1) font = AppFonts::defaultSmall();
    else if (_clockFontSize == 3) font = AppFonts::defaultLarge();
    _screensaverBoard.setClockFont(font);
}

/**
 * @brief Get the current clock font size.
 * @return Font size level (1=small, 2=medium, 3=large).
 */
uint8_t U8G2DisplayModule::getClockFontSize() const {
    return _clockFontSize;
}

// =============================================================================
// IWebContributor — REST API Routes
// =============================================================================

/**
 * @brief Register display REST API routes with the web server.
 *
 * All route handler bodies are guarded by #ifndef NATIVE_BUILD since
 * ESPAsyncWebServer is unavailable in the native GoogleTest runner.
 *
 * @param server Reference to the AsyncWebServer instance.
 */
void U8G2DisplayModule::registerRoutes(AsyncWebServer& server) {
#ifndef NATIVE_BUILD
    // --- GET /api/display/status — current display state ---
    server.on("/api/display/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["brightness"]          = _brightness;
        doc["flip"]                = _flipScreen;
        doc["screensaverEnabled"]  = _screensaver.isEnabled();
        doc["dimmed"]              = _screensaver.isDimmed();
        doc["sleeping"]            = _screensaver.isBlanked();
        doc["dimTimeout"]          = _screensaver.getDimTimeout();
        doc["blankTimeout"]        = _screensaver.getBlankTimeout();
        doc["rotationInterval"]    = _carousel.getInterval();
        doc["boardCount"]          = _carousel.boardCount();
        doc["clockFontSize"]       = _clockFontSize;

        IDisplayBoard* active = visibleBoard();
        doc["activeBoard"] = (active != nullptr) ? active->getBoardName() : "none";

        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // --- POST /api/display/brightness — set brightness level ---
    server.on("/api/display/brightness", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t /*index*/, size_t /*total*/) {
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, data, len);
            if (err || !doc["value"].is<int>()) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON or missing 'value'\"}");
                return;
            }

            int value = doc["value"] | 255;
            if (value < 0) value = 0;
            if (value > 255) value = 255;

            setBrightness((uint8_t)value);

            // Persist via ConfigManager gateway
            if (_configManager != nullptr) {
                _configManager->saveModuleConfig(this);
            }

            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );

    // --- POST /api/display/flip — toggle screen flip ---
    server.on("/api/display/flip", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t /*index*/, size_t /*total*/) {
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, data, len);
            if (err || !doc["value"].is<bool>()) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON or missing 'value'\"}");
                return;
            }

            setFlipScreen(doc["value"] | false);

            // Persist via ConfigManager gateway
            if (_configManager != nullptr) {
                _configManager->saveModuleConfig(this);
            }

            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );

    // --- POST /api/display/sleep — control screensaver behaviour ---
    server.on("/api/display/sleep", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t /*index*/, size_t /*total*/) {
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, data, len);
            if (err) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }

            bool needsPersist = false;

            // Handle immediate sleep/wake toggle
            if (doc["action"].is<const char*>()) {
                const char* action = doc["action"];
                if (strcmp(action, "sleep") == 0) {
                    _screensaver.forceTimeout();
                } else if (strcmp(action, "wake") == 0) {
                    resetActivity();
                }
            }

            // Handle screensaver enable/disable
            if (doc["enabled"].is<bool>()) {
                _screensaver.setEnabled(doc["enabled"].as<bool>());
                needsPersist = true;
            }

            // Handle dimTimeout configuration
            if (doc["dimTimeout"].is<unsigned long>()) {
                _screensaver.setDimTimeout(doc["dimTimeout"] | (unsigned long)150000);
                needsPersist = true;
            }

            // Handle blankTimeout configuration
            if (doc["blankTimeout"].is<unsigned long>()) {
                _screensaver.setBlankTimeout(doc["blankTimeout"] | (unsigned long)300000);
                needsPersist = true;
            }

            // Handle legacy timeout configuration
            if (doc["timeout"].is<unsigned long>()) {
                unsigned long timeout = doc["timeout"] | (unsigned long)300000;
                _screensaver.setDimTimeout(timeout / 2);
                _screensaver.setBlankTimeout(timeout);
                needsPersist = true;
            }

            if (needsPersist && _configManager != nullptr) {
                _configManager->saveModuleConfig(this);
            }

            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );

    // --- GET /api/display/boards — list all pool boards ---
    server.on("/api/display/boards", HTTP_GET, [this](AsyncWebServerRequest* request) {
        JsonDocument doc;
        JsonArray boardsArr = doc["boards"].to<JsonArray>();

        for (int i = 0; i < _carousel.boardCount(); i++) {
            JsonObject entry = boardsArr.add<JsonObject>();
            entry["index"]    = i;
            entry["name"]     = _carousel.getBoard(i)->getBoardName();
            entry["isActive"] = (i == _carousel.activeIndex() && _modal == nullptr);
            entry["isExpired"]= _carousel.getBoard(i)->isExpired();
        }

        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // --- POST /api/display/clockFontSize — set clock font size ---
    server.on("/api/display/clockFontSize", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t /*index*/, size_t /*total*/) {
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, data, len);
            if (err || !doc["value"].is<int>()) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON or missing 'value'\"}");
                return;
            }

            int value = doc["value"] | 2;
            setClockFontSize((uint8_t)value);

            // Persist via ConfigManager gateway
            if (_configManager != nullptr) {
                _configManager->saveModuleConfig(this);
            }

            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );

    // --- POST /api/display/show — force a specific board by index ---
    server.on("/api/display/show", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t /*index*/, size_t /*total*/) {
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, data, len);
            if (err || !doc["index"].is<int>()) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON or missing 'index'\"}");
                return;
            }

            int idx = doc["index"] | 0;
            if (idx < 0 || idx >= _carousel.boardCount()) {
                request->send(400, "application/json", "{\"error\":\"Index out of range\"}");
                return;
            }

            _carousel.show(_carousel.getBoard(idx), millis());
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );

    // --- GET /api/display/screenshot — raw framebuffer capture ---
    server.on("/api/display/screenshot", HTTP_GET, [this](AsyncWebServerRequest* request) {
        uint8_t* buf = _u8g2.getBufferPtr();
        if (buf == nullptr) {
            request->send(500, "application/json", "{\"error\":\"Framebuffer unavailable\"}");
            return;
        }

        // U8G2 buffer: 8 pixels per tile row × tileWidth × tileHeight
        size_t bufferSize = 8 * _u8g2.getBufferTileWidth() * _u8g2.getBufferTileHeight();

        AsyncWebServerResponse* response = request->beginResponse(
            200, "application/octet-stream", buf, bufferSize
        );
        // Prevent caching so each capture is fresh
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        response->addHeader("Pragma", "no-cache");
        response->addHeader("Expires", "0");
        // Pass display geometry so the JS client can render correctly
        response->addHeader("X-Display-Width", String(_displayWidth));
        response->addHeader("X-Display-Height", String(_displayHeight));
        request->send(response);
    });
#endif
}

// =============================================================================
// IMCPContributor — display_status and display_control MCP tools
// =============================================================================

/**
 * @brief Register display MCP tools: display_status (read) and display_control (write).
 *
 * display_status returns the current display state (brightness, flip, active board,
 * screensaver state and timeouts, carousel info). It is read-safe and available
 * even when MCP is in read-only mode.
 *
 * display_control allows remote agents to wake/sleep the display, change brightness,
 * toggle screensaver, and set dim/blank timeouts. It is tagged as mutating and
 * blocked when MCP is in read-only mode.
 *
 * @param registry The MCPToolRegistry to register tools with.
 */
void U8G2DisplayModule::registerMCPTools(MCPToolRegistry& registry) {

    // --- Tool 1: display_status (read-only) ---
    // Returns a JSON snapshot of display brightness, flip, active board,
    // screensaver state/timeouts, and carousel info. No parameters required.
    registry.addTool(
        "display_status",
        "Get current display state including brightness, active board, screensaver status, and timeout configuration",
        [this](JsonObject& params, JsonObject& result) {
            (void)params;

            result["brightness"] = _brightness;
            result["flip"]       = _flipScreen;

            // --- Active board name ---
            IDisplayBoard* active = visibleBoard();
            result["activeBoard"] = (active != nullptr) ? active->getBoardName() : "none";

            // --- Screensaver sub-object ---
            JsonObject ss = result["screensaver"].to<JsonObject>();
            ss["enabled"]      = _screensaver.isEnabled();

            // Map three-state screensaver: blanked > dimmed > active
            if (_screensaver.isBlanked()) {
                ss["state"] = "blanked";
            } else if (_screensaver.isDimmed()) {
                ss["state"] = "dimmed";
            } else {
                ss["state"] = "active";
            }

            ss["dimTimeout"]   = _screensaver.getDimTimeout();
            ss["blankTimeout"] = _screensaver.getBlankTimeout();

            // --- Carousel sub-object ---
            JsonObject car = result["carousel"].to<JsonObject>();
            car["boardCount"]        = _carousel.boardCount();
            car["rotationInterval"]  = _carousel.getInterval();
            car["suspended"]         = _carousel.isSuspended();
        },
        /* mutatesState = */ false
    );

    // --- Tool 2: display_control (write, mutates state) ---
    // Accepts optional parameters: action (wake/sleep), brightness (0-255),
    // screensaverEnabled (bool), dimTimeout (ms), blankTimeout (ms).
    // Persists config changes via ConfigManager when any setting is modified.
    JsonDocument controlSchema;
    controlSchema["type"] = "object";
    JsonObject props = controlSchema["properties"].to<JsonObject>();

    // action: immediate wake or force sleep
    props["action"]["type"]        = "string";
    props["action"]["description"] = "Immediate action: 'wake' to reset inactivity, 'sleep' to force screensaver";
    JsonArray actionEnum = props["action"]["enum"].to<JsonArray>();
    actionEnum.add("wake");
    actionEnum.add("sleep");

    // brightness: 0-255
    props["brightness"]["type"]        = "integer";
    props["brightness"]["description"] = "Display brightness level (0-255)";
    props["brightness"]["minimum"]     = 0;
    props["brightness"]["maximum"]     = 255;

    // screensaverEnabled: toggle screensaver on/off
    props["screensaverEnabled"]["type"]        = "boolean";
    props["screensaverEnabled"]["description"] = "Enable or disable the screensaver";

    // dimTimeout: milliseconds before dimming
    props["dimTimeout"]["type"]        = "integer";
    props["dimTimeout"]["description"] = "Milliseconds of inactivity before dimming";

    // blankTimeout: milliseconds before blanking/sleep
    props["blankTimeout"]["type"]        = "integer";
    props["blankTimeout"]["description"] = "Milliseconds of inactivity before blanking";

    registry.addTool(
        "display_control",
        "Control display settings: wake/sleep, enable/disable screensaver, set brightness and timeouts",
        controlSchema,
        [this](JsonObject& params, JsonObject& result) {
            bool needsPersist = false;

            // --- Handle immediate action (wake / sleep) ---
            if (params["action"].is<const char*>()) {
                const char* action = params["action"].as<const char*>();
                if (strcmp(action, "wake") == 0) {
                    resetActivity();
                } else if (strcmp(action, "sleep") == 0) {
                    _screensaver.forceTimeout();
                }
            }

            // --- Handle brightness ---
            if (params["brightness"].is<int>()) {
                int val = params["brightness"].as<int>();
                if (val < 0) val = 0;
                if (val > 255) val = 255;
                setBrightness(static_cast<uint8_t>(val));
                needsPersist = true;
            }

            // --- Handle screensaver enabled toggle ---
            if (params["screensaverEnabled"].is<bool>()) {
                _screensaver.setEnabled(params["screensaverEnabled"].as<bool>());
                needsPersist = true;
            }

            // --- Handle dim timeout ---
            if (params["dimTimeout"].is<int>()) {
                _screensaver.setDimTimeout(
                    static_cast<unsigned long>(params["dimTimeout"].as<int>()));
                needsPersist = true;
            }

            // --- Handle blank timeout ---
            if (params["blankTimeout"].is<int>()) {
                _screensaver.setBlankTimeout(
                    static_cast<unsigned long>(params["blankTimeout"].as<int>()));
                needsPersist = true;
            }

            // --- Persist configuration if any setting was changed ---
            if (needsPersist && _configManager != nullptr) {
                _configManager->saveModuleConfig(this);
            }

            result["status"] = "ok";
        },
        /* mutatesState = */ true
    );

    ESP_LOGI(TAG, "Registered MCP tools: display_status, display_control");
}

// =============================================================================
// EventBus static callbacks
// =============================================================================

/**
 * @brief Static callback for "ota.progress" EventBus events.
 *
 * Casts the context to U8G2DisplayModule*, the payload to OTAProgressPayload*,
 * and delegates to the OTAProgressBoard. Shows the OTA board on screen.
 *
 * @param topic   Topic string ("ota.progress").
 * @param payload Pointer to OTAProgressPayload.
 * @param ctx     The U8G2DisplayModule instance (passed during subscribe).
 */
void U8G2DisplayModule::_onOtaProgress(const char* /*topic*/, const void* payload, void* ctx) {
    auto* self = static_cast<U8G2DisplayModule*>(ctx);
    if (payload == nullptr || self == nullptr) {
        return;
    }

    const auto* ota = static_cast<const OTAProgressPayload*>(payload);

    if (ota->complete) {
        // --- OTA cycle finished — mark board as complete ---
        self->_otaProgress.setComplete(ota->success);
    } else {
        // --- OTA in progress — update progress bar ---
        self->_otaProgress.setProgress(ota->progress, ota->status);
    }

    // --- Push OTA board as modal overlay ---
    self->showModal(&self->_otaProgress);
}

/**
 * @brief Static callback for "display.message" EventBus events.
 *
 * Casts the context to U8G2DisplayModule*, the payload to
 * DisplayMessagePayload*, and shows the message via the BootBoard overlay.
 * Resets the inactivity timer to keep the display awake.
 *
 * @param topic   Topic string ("display.message").
 * @param payload Pointer to DisplayMessagePayload.
 * @param ctx     The U8G2DisplayModule instance (passed during subscribe).
 */
void U8G2DisplayModule::_onDisplayMessage(const char* /*topic*/, const void* payload, void* ctx) {
    auto* self = static_cast<U8G2DisplayModule*>(ctx);
    if (payload == nullptr || self == nullptr) {
        return;
    }

    const auto* msg = static_cast<const DisplayMessagePayload*>(payload);

    // --- Use the boot board as a temporary message overlay ---
    self->_bootBoard.setStatus(msg->message, -1.0f);

    // --- Push boot board as modal overlay ---
    self->showModal(&self->_bootBoard);
}



// =============================================================================
// Private carousel engine
// =============================================================================

/**
 * @brief Render the current board into the U8G2 display buffer.
 *
 * Uses clearBuffer() → render() → sendBuffer() inside NATIVE_BUILD guard.
 */
void U8G2DisplayModule::_renderCurrentBoard() {
#ifndef NATIVE_BUILD
    // --- Determine which board to render ---
    // Priority: Modal → Base → Screensaver (idle) → Carousel
    IDisplayBoard* boardToRender = nullptr;

    if (_modal != nullptr) {
        boardToRender = _modal;
    } else if (_baseBoard != nullptr) {
        boardToRender = _baseBoard;
    } else if (_screensaver.isActive() && _screensaver.board() != nullptr) {
        boardToRender = _screensaver.board();
    } else if (_carousel.currentBoard() != nullptr) {
        boardToRender = _carousel.currentBoard();
    }

    if (boardToRender == nullptr) {
        return;
    }

    // --- U8G2 draw loop ---
    _u8g2.clearBuffer();
    boardToRender->render(_u8g2);
    _u8g2.sendBuffer();
    vTaskDelay(1);  // Yield to IDLE task + WiFi stack after SPI transfer
#endif
}
