/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
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
 * - U8G2DisplayModule::getBrightness():     Return current contrast/brightness level
 * - U8G2DisplayModule::getFlipScreen():     Return display flip state
 * - U8G2DisplayModule::setBrightness():     Runtime brightness adjustment
 * - U8G2DisplayModule::setFlipScreen():     Runtime screen flip toggle
 * - U8G2DisplayModule::setSleepTimeout():   Runtime sleep timeout adjustment
 * - U8G2DisplayModule::setRotationInterval(): Runtime carousel interval adjustment
 * - U8G2DisplayModule::setTargetFps():      Runtime frame-rate target adjustment
 * - U8G2DisplayModule::isDimmed():          Return true if screensaver is in dimmed state
 * - U8G2DisplayModule::isSleeping():        Return true if screensaver is blanked/sleeping
 * - U8G2DisplayModule::capturePNG():         Capture current display frame to 1bpp PNG stream
 * - U8G2DisplayModule::showSystemState():   Display system overlay state (normal, error, configuring)
 * - U8G2DisplayModule::_onOtaProgress():    Static EventBus callback for OTA events
 * - U8G2DisplayModule::_onDisplayMessage(): Static EventBus callback for display messages
 */

#include "U8G2DisplayModule.hpp"
#include "web/serveGzipAsset.hpp"
#include "events/AppConfiguringPayload.hpp"
#include "platform/LogShim.h"
#include <cstring>
#include <cstdio>
#include <vector>
#include "utils/BufferStream.h"
#include <Print.h>
#include <mbedtls/base64.h>

static constexpr const char* TAG = "U8G2DisplayModule";

#include <PsychicHttp.h>
#include <ArduinoJson.h>
#include "Application.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
// SPI.h is required whenever explicit SCK+MOSI pins are provided, even on
// write-only displays where MISO is physically absent.
#if defined(PIN_DISPLAY_SCK) && defined(PIN_DISPLAY_MOSI)
  #include <SPI.h>
#endif

// Include EventBus directly — it is native-safe (no Arduino.h dependency).
#include "EventBus.hpp"
#ifdef WITH_OTA
#include "OTAUpdateManager.hpp"
#endif
#include "ConfigManager.hpp"
#include "MCPToolRegistry.hpp"
#include "fonts/Fonts.hpp"
#include "portal_assets/display_js.hpp"
#include "portal_assets/screenshot_js.hpp"
#include "portal_assets/screenshot_html.hpp"
#include "portal_assets/carousel_js.hpp"

// =============================================================================
// Lifecycle
// =============================================================================

U8G2DisplayModule::U8G2DisplayModule(const BootBranding* branding,
                                     const U8G2BootLogo* bootLogo,
                                     IU8G2SystemScreens* screens)
    : _branding(branding),
      _bootLogo(bootLogo),
      _screens(screens),
      _bootBoard(_displayWidth, _displayHeight, _bootLogo, _branding ? _branding->appTitle : nullptr)
{
}

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
    // --- Step 1: Initialize SPI bus ---
    // If explicit SCK+MOSI pins are defined in board.h, secure the SPI bus before U8G2
    // init. Write-only displays (e.g. SSD1322) may set MISO to PIN_UNASSIGNED (-1),
    // which is the correct "no MISO" value for SPI.begin().
    // CRITICAL: On ESP32-S3, the default Arduino pin mapping can collide with
    // the internal Flash/PSRAM bus. Explicit pins prevent watchdog crashes.
  #if defined(PIN_DISPLAY_SCK) && defined(PIN_DISPLAY_MOSI)
    ESP_LOGI(TAG, "Bootstrap: SPI.begin(SCK=%d, MISO=%d, MOSI=%d)",
             PIN_DISPLAY_SCK, PIN_DISPLAY_MISO, PIN_DISPLAY_MOSI);
    SPI.begin(PIN_DISPLAY_SCK, PIN_DISPLAY_MISO, PIN_DISPLAY_MOSI, -1);
  #else
    ESP_LOGW(TAG, "Bootstrap: No explicit SPI pins — using Arduino defaults");
    SPI.begin();
  #endif

    // --- Step 2: Initialize U8G2 driver ---
  #if defined(PIN_DISPLAY_CS) && PIN_IS_VALID(PIN_DISPLAY_CS)
    ESP_LOGI(TAG, "Bootstrap: U8G2 init (CS=%d, DC=%d, RST=%d)",
             PIN_DISPLAY_CS, PIN_DISPLAY_DC, PIN_DISPLAY_RST);
  #else
    ESP_LOGI(TAG, "Bootstrap: U8G2 init (I2C mode, no pin defines)");
  #endif

    _u8g2.begin();

    // --- Step 3: Configure draw mode ---
    // Standard U8G2 SPI display initialisation sequence.
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
        const unsigned long kStepMs = kFadeDurationMs / kFadeSteps;
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

    // --- Step 5: Enter bootstrap mode ---
    // reportStatus() will render inline until begin(app) exits bootstrap mode.
    _bootstrapMode = true;
    _lastBootProgress = 0.0f;
    ESP_LOGI(TAG, "Bootstrap: display hardware ready, inline rendering enabled");
}

bool U8G2DisplayModule::beginEarly() {
    bootstrapHardware();
    return true;
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
    _serviceHelper.setSaveCallback([this]() {
        if (_configManager) _configManager->saveModuleConfig(this);
    });
    
    _screensaver.setBoard(&_screensaverBoard);

    // --- Step 1: Initialize hardware (skip if already bootstrapped) ---
    if (!_bootstrapMode) {
      #if defined(PIN_DISPLAY_SCK) && defined(PIN_DISPLAY_MOSI)
        SPI.begin(PIN_DISPLAY_SCK, PIN_DISPLAY_MISO, PIN_DISPLAY_MOSI, -1);
      #endif

        ESP_LOGI(TAG, "Initializing hardware display driver");
        _u8g2.begin();
        _u8g2.setContrast(_brightness);
        _u8g2.setFlipMode(_flipScreen ? 1 : 0);
    } else {
        ESP_LOGI(TAG, "Hardware already bootstrapped — skipping SPI/U8G2 init");
    }

    // --- Step 2: Subscribe to EventBus topics ---
    // Guarded: getEventBus() requires Application.hpp (unavailable in native)
    if (_eventBus != nullptr) {
#ifdef WITH_OTA
        _eventBus->subscribe(ApplicationEvents::OTA_PROGRESS, _onOtaProgress, this);
#endif
        _eventBus->subscribe(U8G2DisplayEvents::DISPLAY_MESSAGE, _onDisplayMessage, this);
        _eventBus->subscribe(ApplicationEvents::APP_CONFIGURING, _onAppConfiguring, this);
        _eventBus->subscribe(ApplicationEvents::APP_RUNNING, _onAppRunning, this);
        _eventBus->subscribe(ApplicationEvents::PRECONDITION_HEALTH, _onPreconditionHealth, this);
    }

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
    resetActivity();
    _renderTimer.setInterval(_targetFrameIntervalMs);
    _renderTimer.reset(millis());

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
void U8G2DisplayModule::tick(uint32_t currentMillis) {
    // --- 1. Tick Screensaver ---
    unsigned long currentMs = currentMillis;
    bool wasBlanked = _screensaver.isBlanked();
    _screensaver.tick(currentMs, _u8g2, _brightness);
    bool isBlanked = _screensaver.isBlanked();

    if (wasBlanked != isBlanked && _eventBus != nullptr) {
        _eventBus->publish(isBlanked ? U8G2DisplayEvents::DISPLAY_SLEEP : U8G2DisplayEvents::DISPLAY_WAKE);
    }
    
    // If sleeping, skip carousel and rendering.
    // IMPORTANT: vTaskDelay ensures loopTask yields for ~1 frame even when blanked.
    // Without this, tick() returns in microseconds (no rendering), loopTask spins
    // at MHz speed, starves IDLE1 on CPU 1, and the Task Watchdog fires after 5s.
    if (isBlanked) {
        vTaskDelay(pdMS_TO_TICKS(_targetFrameIntervalMs));
        return;
    }

    // --- 2. Auto-clear expired modals ---
    if (_modal != nullptr && _modal->isExpired()) {
        clearModal();
    }

    // --- 3. Tick Carousel ---
    _carousel.tick(currentMs);

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
    visible->tick(currentMillis);
    if (_renderTimer.check(currentMillis)) {
        _renderCurrentBoard();
        _renderTimer.reset(currentMillis);
    }
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
        _eventBus->publish(U8G2DisplayEvents::DISPLAY_BOARD_CHANGED);
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
        _u8g2.setContrast(_brightness);
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
    IDisplayBoard* errBoard = (_screens != nullptr && _screens->errorBoard() != nullptr)
                              ? _screens->errorBoard()
                              : &_error;

    if (errBoard == &_error) {
        _error.showError(errorMessage, true);
    }

    showModal(errBoard);

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
    // --- Step 1: Parse brightness and screen orientation flags from JSON ---
    _brightness         = section["brightness"]      | (uint8_t)255;
    _flipScreen         = section["flip"]            | false;

    // --- Step 2: Configure screensaver timeouts and handle legacy sleepTimeout migration ---
    bool screensaverEnabled = section["screensaverEnabled"] | true;
    _screensaver.setEnabled(screensaverEnabled);

    if (!section["dimTimeout"].isNull()) {
        // --- New-style config: independent timeouts ---
        unsigned long dimTimeoutMs   = section["dimTimeout"]   | (unsigned long)150000;
        unsigned long blankTimeoutMs = section["blankTimeout"]  | (unsigned long)300000;
        _screensaver.setDimTimeout(dimTimeoutMs);
        _screensaver.setBlankTimeout(blankTimeoutMs);
    } else if (!section["sleepTimeout"].isNull()) {
        // --- Migration: derive from legacy sleepTimeout ---
        unsigned long sleepTimeoutMs = section["sleepTimeout"] | (unsigned long)300000;
        _screensaver.setDimTimeout(sleepTimeoutMs / 2);
        _screensaver.setBlankTimeout(sleepTimeoutMs);
    } else {
        // --- Defaults (no config present) ---
        _screensaver.setDimTimeout(150000);
        _screensaver.setBlankTimeout(300000);
    }

    // --- Step 3: Configure carousel interval, target frame rate, and clock font selection ---
    unsigned long rotationInterval = section["rotationInterval"] | (unsigned long)8000;
    _carousel.setInterval(rotationInterval);
    _targetFrameIntervalMs = 1000UL / (section["targetFps"] | (uint8_t)30);
    _clockFontSize      = section["clockFontSize"]    | (uint8_t)2;

    const uint8_t* font = AppFonts::defaultMedium();
    if (_clockFontSize == 1) font = AppFonts::defaultSmall();
    else if (_clockFontSize == 3) font = AppFonts::defaultLarge();
    _screensaverBoard.setClockFont(font);

    // --- Step 4: Apply contrast and flip settings directly to U8G2 hardware driver ---
    _u8g2.setContrast(_brightness);
    _u8g2.setFlipMode(_flipScreen ? 1 : 0);

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
    _u8g2.setContrast(_brightness);
    _u8g2.setFlipMode(_flipScreen ? 1 : 0);
}

// =============================================================================
// MCP display info accessor
// =============================================================================
// IDisplayModule interface implementation
// =============================================================================

uint16_t U8G2DisplayModule::getDisplayWidth() const {
    return _displayWidth;
}

uint16_t U8G2DisplayModule::getDisplayHeight() const {
    return _displayHeight;
}

bool U8G2DisplayModule::isDimmed() const {
    return _screensaver.isDimmed();
}

bool U8G2DisplayModule::isSleeping() const {
    return _screensaver.isBlanked();
}

bool U8G2DisplayModule::capturePNG(Print& sink) {
    const uint8_t* buf = _u8g2.getBufferPtr();
    if (!buf) {
        return false;
    }
    return PngWriter::writeGrayscale1bppFromTiles(sink, _displayWidth, _displayHeight, buf);
}

void U8G2DisplayModule::showSystemState(const SystemDisplayParams& params) {
    // --- Step 1: Evaluate requested system display state (Normal, Error, Configuring, or Generic) ---
    if (params.state != nullptr && strcmp(params.state, DisplayState::Normal) == 0) {
        clearModal();
    } else if (params.state != nullptr && strcmp(params.state, DisplayState::Error) == 0) {
        _error.showError(params.message ? params.message : (params.title ? params.title : "Error"), true);
        showModal(&_error);
    } else if (params.state != nullptr && strcmp(params.state, DisplayState::Configuring) == 0) {
        if (params.title != nullptr) {
            _configuringStatusBoard.setActionableGateName(params.title);
        }
        if (params.message != nullptr) {
            _configuringStatusBoard.setConnectionInfo("", params.message);
        }
        showBase(&_configuringStatusBoard);
    } else {
        _error.showError(params.message ? params.message : (params.title ? params.title : "System"), false);
        showModal(&_error);
    }

    // --- Step 2: Reset activity timer to wake display and prevent screensaver blanking ---
    resetActivity();
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
    _u8g2.setContrast(_brightness);
}

/**
 * @brief Set the screen flip mode and apply immediately.
 * @param flip True to flip the display 180 degrees.
 */
void U8G2DisplayModule::setFlipScreen(bool flip) {
    _flipScreen = flip;
    _u8g2.setFlipMode(_flipScreen ? 1 : 0);
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
 * @brief Return SVG inner content for the Display bottom-nav icon.
 *
 * Uses a Feather-style "monitor" icon (24×24 viewBox).
 * Only the inner SVG body is returned; the outer <svg> wrapper is
 * added by app.js.
 */
const char* U8G2DisplayModule::getTabIcon() const {
    return "<rect x=\"2\" y=\"3\" width=\"20\" height=\"14\" rx=\"2\" ry=\"2\"/>"
           "<line x1=\"8\" y1=\"21\" x2=\"16\" y2=\"21\"/>"
           "<line x1=\"12\" y1=\"17\" x2=\"12\" y2=\"21\"/>";
}

/**
 * @brief Register display REST API routes with the web server.
 *
 * All route handler bodies are using native test stubs since
 * ESPAsyncWebServer is unavailable in the native GoogleTest runner.
 *
 * @param server Reference to the AsyncWebServer instance.
 */
void U8G2DisplayModule::registerRoutes(PsychicHttpServer& server) {
    server.on("/display.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        return serveGzipAsset(request, response, display_js_gz, display_js_gz_len, display_js_gz_etag, "application/javascript");
    });

    server.on("/screenshot.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        return serveGzipAsset(request, response, screenshot_js_gz, screenshot_js_gz_len, screenshot_js_gz_etag, "application/javascript");
    });

    server.on("/screenshot.html", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        return serveGzipAsset(request, response, screenshot_html_gz, screenshot_html_gz_len, screenshot_html_gz_etag, "text/html");
    });

    server.on("/carousel.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        return serveGzipAsset(request, response, carousel_js_gz, carousel_js_gz_len, carousel_js_gz_etag, "application/javascript");
    });

    // --- Step 1: Register shared service helper endpoints (brightness, screenshot) ---
#ifndef UNIT_TEST
#ifdef WITH_OTA
    // KNOWN GAP — tracked in issue #138. This static call into OTAUpdateManager
    // is the coupling #120 set out to remove, and LVGLDisplayModule now uses a
    // different (also imperfect) predicate. Both should move to an injected
    // OTA state provider.
    _serviceHelper.setOtaPredicate([]() {
        return OTAUpdateManager::getState() != OTAState::Idle;
    });
#endif
#endif
    _serviceHelper.registerBrightnessRoute(server);
    _serviceHelper.registerScreenshotRoute(server);

    // --- Step 2: Register GET /api/display/status endpoint for active display state snapshot ---
    server.on("/api/display/status", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        JsonDocument doc;
        _serviceHelper.populateCommonStatus(doc);
        
        doc["flip"]                = _flipScreen;
        doc["screensaverEnabled"]  = _screensaver.isEnabled();
        doc["dimTimeout"]          = _screensaver.getDimTimeout();
        doc["blankTimeout"]        = _screensaver.getBlankTimeout();
        doc["rotationInterval"]    = _carousel.getInterval();
        doc["boardCount"]          = _carousel.boardCount();
        doc["clockFontSize"]       = _clockFontSize;

        IDisplayBoard* active = visibleBoard();
        doc["activeBoard"] = (active != nullptr) ? active->getBoardName() : "none";

        String output;
        serializeJson(doc, output);
        response->send(200, "application/json", output.c_str());
        return ESP_OK;
    });

    // --- Step 3: Register POST /api/display/flip endpoint for runtime orientation control ---
    server.on("/api/display/flip", HTTP_POST,
        [this](PsychicRequest *request, PsychicResponse* response) {
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

            setFlipScreen(doc["value"] | false);

            if (_configManager != nullptr) {
                _configManager->saveModuleConfig(this);
            }

            response->send(200, "application/json", "{\"status\":\"ok\"}");
            return ESP_OK;
        }
    );

    // --- Step 4: Register POST /api/display/sleep endpoint for screensaver mode and timeout control ---
    server.on("/api/display/sleep", HTTP_POST,
        [this](PsychicRequest *request, PsychicResponse* response) {
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

            response->send(200, "application/json", "{\"status\":\"ok\"}");
            return ESP_OK;
        }
    );

    // --- Step 5: Register GET /api/display/boards endpoint to inspect registered carousel boards ---
    server.on("/api/display/boards", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
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
        response->send(200, "application/json", output.c_str());
        return ESP_OK;
    });

    // --- Step 6: Register POST /api/display/interval endpoint for carousel rotation interval adjustment ---
    server.on("/api/display/interval", HTTP_POST,
        [this](PsychicRequest *request, PsychicResponse* response) {
            const char* body = request->bodyCStr();
            if (!body || strlen(body) == 0) {
                response->send(400, "application/json", "{\"error\":\"body_required\"}");
                return ESP_FAIL;
            }
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, body);
            if (err || !doc["value"].is<unsigned long>()) {
                response->send(400, "application/json", "{\"error\":\"Invalid JSON or missing 'value'\"}");
                return ESP_FAIL;
            }

            setRotationInterval(doc["value"].as<unsigned long>());

            if (_configManager != nullptr) {
                _configManager->saveModuleConfig(this);
            }

            response->send(200, "application/json", "{\"status\":\"ok\"}");
            return ESP_OK;
        }
    );

    // --- Step 7: Register POST /api/display/clockFontSize endpoint for screensaver clock styling ---
    server.on("/api/display/clockFontSize", HTTP_POST,
        [this](PsychicRequest *request, PsychicResponse* response) {
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

            int value = doc["value"] | 2;
            setClockFontSize((uint8_t)value);

            if (_configManager != nullptr) {
                _configManager->saveModuleConfig(this);
            }

            response->send(200, "application/json", "{\"status\":\"ok\"}");
            return ESP_OK;
        }
    );

    // --- Step 8: Register POST /api/display/show endpoint to force selection of specific board ---
    server.on("/api/display/show", HTTP_POST,
        [this](PsychicRequest *request, PsychicResponse* response) {
            const char* body = request->bodyCStr();
            if (!body || strlen(body) == 0) {
                response->send(400, "application/json", "{\"error\":\"body_required\"}");
                return ESP_FAIL;
            }
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, body);
            if (err || !doc["index"].is<int>()) {
                response->send(400, "application/json", "{\"error\":\"Invalid JSON or missing 'index'\"}");
                return ESP_FAIL;
            }

            int idx = doc["index"] | 0;
            if (idx < 0 || idx >= _carousel.boardCount()) {
                response->send(400, "application/json", "{\"error\":\"Index out of range\"}");
                return ESP_FAIL;
            }

            _carousel.show(_carousel.getBoard(idx), millis());
            response->send(200, "application/json", "{\"status\":\"ok\"}");
            return ESP_OK;
        }
    );

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

    _serviceHelper.registerCaptureTool(registry);

    // --- Tool 1: display_status (read-only) ---
    // Returns a JSON snapshot of display brightness, flip, active board,
    // screensaver state/timeouts, and carousel info. No parameters required.
    registry.addTool(
        "display_status",
        "Get current display state including brightness, active board, screensaver status, and timeout configuration",
        [](void* ctx, JsonObject& params, JsonObject& result) {
            auto* self = static_cast<U8G2DisplayModule*>(ctx);
            (void)params;

            self->_serviceHelper.populateCommonMCPStatus(result);

            result["flip"]       = self->_flipScreen;

            // --- Active board name ---
            IDisplayBoard* active = self->visibleBoard();
            result["activeBoard"] = (active != nullptr) ? active->getBoardName() : "none";

            // --- Screensaver sub-object ---
            JsonObject ss = result["screensaver"].to<JsonObject>();
            ss["enabled"]      = self->_screensaver.isEnabled();

            // Map three-state screensaver: blanked > dimmed > active
            if (self->_screensaver.isBlanked()) {
                ss["state"] = "blanked";
            } else if (self->_screensaver.isDimmed()) {
                ss["state"] = "dimmed";
            } else {
                ss["state"] = "active";
            }

            ss["dimTimeout"]   = self->_screensaver.getDimTimeout();
            ss["blankTimeout"] = self->_screensaver.getBlankTimeout();

            // --- Carousel sub-object ---
            JsonObject car = result["carousel"].to<JsonObject>();
            car["boardCount"]        = self->_carousel.boardCount();
            car["rotationInterval"]  = self->_carousel.getInterval();
            car["suspended"]         = self->_carousel.isSuspended();
        },
        this,  // context: U8G2DisplayModule*
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
        [](void* ctx, JsonObject& params, JsonObject& result) {
            auto* self = static_cast<U8G2DisplayModule*>(ctx);
            bool needsPersist = false;

            // --- Handle immediate action (wake / sleep) ---
            if (params["action"].is<const char*>()) {
                const char* action = params["action"].as<const char*>();
                if (strcmp(action, "wake") == 0) {
                    self->resetActivity();
                } else if (strcmp(action, "sleep") == 0) {
                    self->_screensaver.forceTimeout();
                }
            }

            // --- Handle brightness ---
            if (params["brightness"].is<int>()) {
                int val = params["brightness"].as<int>();
                if (val < 0) val = 0;
                if (val > 255) val = 255;
                self->setBrightness(static_cast<uint8_t>(val));
                needsPersist = true;
            }

            // --- Handle screensaver enabled toggle ---
            if (params["screensaverEnabled"].is<bool>()) {
                self->_screensaver.setEnabled(params["screensaverEnabled"].as<bool>());
                needsPersist = true;
            }

            // --- Handle dim timeout ---
            if (params["dimTimeout"].is<int>()) {
                self->_screensaver.setDimTimeout(
                    static_cast<unsigned long>(params["dimTimeout"].as<int>()));
                needsPersist = true;
            }

            // --- Handle blank timeout ---
            if (params["blankTimeout"].is<int>()) {
                self->_screensaver.setBlankTimeout(
                    static_cast<unsigned long>(params["blankTimeout"].as<int>()));
                needsPersist = true;
            }

            // --- Persist configuration if any setting was changed ---
            if (needsPersist && self->_configManager != nullptr) {
                self->_configManager->saveModuleConfig(self);
            }

            result["status"] = "ok";
        },
        this,  // context: U8G2DisplayModule*
        /* mutatesState = */ true
    );

    ESP_LOGI(TAG, "Registered MCP tools: display_status, display_control, capture_display");
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
#ifdef WITH_OTA
void U8G2DisplayModule::_onOtaProgress(const char* /*topic*/, const void* payload, void* ctx) {
    auto* self = static_cast<U8G2DisplayModule*>(ctx);
    if (payload == nullptr || self == nullptr) return;

    const auto* ota = static_cast<const ApplicationEvents::OtaProgressPayload*>(payload);

    IDisplayBoard* otaBoard = (self->_screens != nullptr && self->_screens->otaProgressBoard() != nullptr)
                              ? self->_screens->otaProgressBoard()
                              : &self->_otaProgress;

    if (otaBoard == &self->_otaProgress) {
        if (ota->complete) {
            self->_otaProgress.setComplete(ota->success);
        } else {
            self->_otaProgress.setProgress(ota->progress, ota->status);
        }
    }

    self->showModal(otaBoard);
    self->resetActivity();
}
#endif

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

/**
 * @brief Static callback for "app.configuring" EventBus events.
 *
 * Suspends carousel rotation, sets the configuring status board as base layer,
 * tears down any previous modal, then applies three-tier modal dispatch:
 *   Tier 1: Consumer override via IU8G2SystemScreens::gateHelperBoard()
 *   Tier 2: Built-in WiFi wizard for PreconditionNames::WIFI_STATION
 *   Tier 3: Generic fallback via GenericGateHelperBoard
 *
 * Only shows a modal when severity == NeedsUser.
 */
void U8G2DisplayModule::_onAppConfiguring(const char* /*topic*/, const void* payload, void* ctx) {
    auto* self = static_cast<U8G2DisplayModule*>(ctx);
    if (self == nullptr) {
        return;
    }

    // --- 1. Suspend carousel and show base configuring board ---
    self->carousel.suspend();
    IDisplayBoard* cfgBoard = (self->_screens != nullptr && self->_screens->configuringBoard() != nullptr)
                              ? self->_screens->configuringBoard()
                              : &self->boards.configuringStatus;
    self->showBase(cfgBoard);

    // --- 2. Always tear down previous modal first ---
    self->clearModal();

    // --- 3. Parse payload ---
    if (payload == nullptr) {
        return;
    }
    auto* p = static_cast<const AppConfiguringPayload*>(payload);

    // --- 4. Update configuring status board with connection info ---
    self->boards.configuringStatus.setActionableGateName(p->preconditionName);
    char urlBuf[48] = "";
    if (p->deviceIP != nullptr) {
        snprintf(urlBuf, sizeof(urlBuf), "http://%s", p->deviceIP);
    }
    self->boards.configuringStatus.setConnectionInfo(
        p->hostname ? p->hostname : "",
        p->deviceIP ? urlBuf : ""
    );

    // --- 5. If not NeedsUser, no modal needed ---
    if (p->severity != Severity::NeedsUser) {
        return;
    }

    // --- 6. Three-tier modal dispatch ---
    IDisplayBoard* modalBoard = nullptr;

    // Tier 1: Consumer override via IU8G2SystemScreens
    if (self->_screens != nullptr) {
        modalBoard = self->_screens->gateHelperBoard(p);
    }

    // Tier 2: Built-in WiFi wizard
    if (modalBoard == nullptr && p->preconditionName != nullptr &&
        strcmp(p->preconditionName, PreconditionNames::WIFI_STATION) == 0) {
        // Resolve wizard board: prefer IU8G2SystemScreens wifiWizardBoard override
        IDisplayBoard* wizardBoard = (self->_screens != nullptr && self->_screens->wifiWizardBoard() != nullptr)
                                     ? self->_screens->wifiWizardBoard()
                                     : &self->boards.wifiWizard;

        // Configure built-in board with hostname + IP (custom boards manage their own state)
        if (wizardBoard == &self->boards.wifiWizard) {
            self->boards.wifiWizard.configure(p->hostname, p->deviceIP ? p->deviceIP : "192.168.4.1");
        }
        modalBoard = wizardBoard;
    }

    // Tier 3: Generic fallback
    if (modalBoard == nullptr) {
        self->boards.genericHelper.configure(
            p->preconditionName ? p->preconditionName : "Configuration",
            p->helpText ? p->helpText : "",
            p->deviceIP ? p->deviceIP : ""
        );
        modalBoard = &self->boards.genericHelper;
    }

    // --- 7. Push resolved modal ---
    self->showModal(modalBoard);
}

/**
 * @brief Static callback for "app.running" EventBus events.
 *
 * Clears modal overlays, clears base layer override, and resumes carousel rotation.
 */
void U8G2DisplayModule::_onAppRunning(const char* /*topic*/, const void* /*payload*/, void* ctx) {
    auto* self = static_cast<U8G2DisplayModule*>(ctx);
    if (self == nullptr) {
        return;
    }

    self->clearModal();
    self->showBase(nullptr);
    self->carousel.resume();
}

/**
 * @brief Static callback for "precondition.health" EventBus events.
 *
 * Updates ConfiguringStatusBoard cached precondition state.
 */
void U8G2DisplayModule::_onPreconditionHealth(const char* /*topic*/, const void* payload, void* ctx) {
    auto* self = static_cast<U8G2DisplayModule*>(ctx);
    if (self == nullptr || payload == nullptr) {
        return;
    }

    const auto* p = static_cast<const PreconditionHealthPayload*>(payload);
    self->boards.configuringStatus.updatePrecondition(*p);
}



// =============================================================================
// Private carousel engine
// =============================================================================

/**
 * @brief Render the current board into the U8G2 display buffer.
 *
 * Uses clearBuffer() → render() → sendBuffer() inside native test stub.
 */
void U8G2DisplayModule::_renderCurrentBoard() {
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
}
