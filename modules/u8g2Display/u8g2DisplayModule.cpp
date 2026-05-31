/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/u8g2DisplayModule.cpp
 * Description: Implementation of the U8G2 display module with two-layer
 *              base+modal architecture, carousel engine, sleep/wake power
 *              management, phase-aware brightness dimming, system board
 *              ownership, IStatusReporter delegation (Boot/Error boards),
 *              EventBus subscriptions (ota.progress, display.message),
 *              EventBus publishing (display.sleep, display.wake,
 *              display.board.changed), IWebContributor REST API routes
 *              under /api/display/, and iConfigurable persistence at
 *              /mod/display.json.
 *
 * Exported Functions:
 * - U8g2DisplayModule::begin():              Hardware init, EventBus subscribe, pool lock
 * - U8g2DisplayModule::tick():               Carousel rotation, sleep check, board tick, render
 * - U8g2DisplayModule::addBoard():           Pre-lock board registration
 * - U8g2DisplayModule::showBoard():          Force-show a specific board
 * - U8g2DisplayModule::resetActivity():      Reset inactivity timer, wake if sleeping
 * - U8g2DisplayModule::getBoardCount():      Return pool size
 * - U8g2DisplayModule::getActiveBoard():     Return current rendering board or nullptr
 * - U8g2DisplayModule::getBootBoard():       Return reference to BootBoard
 * - U8g2DisplayModule::getWifiWizard():      Return reference to WiFiWizardBoard
 * - U8g2DisplayModule::getOtaProgress():     Return reference to OtaProgressBoard
 * - U8g2DisplayModule::getError():           Return reference to ErrorBoard
 * - U8g2DisplayModule::getScreensaver():     Return reference to ScreensaverBoard
 * - U8g2DisplayModule::reportStatus():       Delegate to BootBoard, show it
 * - U8g2DisplayModule::reportError():        Delegate to ErrorBoard (fatal), show it
 * - U8g2DisplayModule::clearStatus():        Mark loading complete, advance if showing
 * - U8g2DisplayModule::registerRoutes():     Register /api/display/* REST endpoints
 * - U8g2DisplayModule::applyConfig():        Apply display settings from JSON section
 * - U8g2DisplayModule::serializeConfig():    Serialize display settings to JSON section
 * - U8g2DisplayModule::resetConfig():        Reset to factory defaults
 * - U8g2DisplayModule::setBrightness():      Runtime brightness adjustment
 * - U8g2DisplayModule::setFlipScreen():      Runtime screen flip toggle
 * - U8g2DisplayModule::setSleepTimeout():    Runtime sleep timeout adjustment
 * - U8g2DisplayModule::setRotationInterval(): Runtime carousel interval adjustment
 * - U8g2DisplayModule::_onOtaProgress():     Static EventBus callback for OTA events
 * - U8g2DisplayModule::_onDisplayMessage():  Static EventBus callback for display messages
 * - U8g2DisplayModule::_onStateChanged():    State-polling lifecycle handler (replaces EventBus callbacks)
 */

#include "u8g2DisplayModule.hpp"
#include "platform/LogShim.h"
#include <cstring>

static const char* TAG = "U8g2Display";

#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "../application/Application.hpp"
  #if defined(DISPLAY_SPI_SCK) && defined(DISPLAY_SPI_MISO) && defined(DISPLAY_SPI_MOSI)
  #include <SPI.h>
  #endif
#endif

// Include EventBus directly — it is native-safe (no Arduino.h dependency).
#include "../eventBus/EventBus.hpp"
#include "../configManager/configManager.hpp"

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
void U8g2DisplayModule::bootstrapHardware() {
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
    _bootBoard.activate(APP_FOUNDATION_VERSION);
    pushModal(&_bootBoard);
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
void U8g2DisplayModule::begin(Application& app) {
    _app = &app;

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
    ESP_LOGI(TAG, "Subscribing to EventBus topics: ota.progress, display.message");
    _app->getEventBus().subscribe("ota.progress", _onOtaProgress, this);
    _app->getEventBus().subscribe("display.message", _onDisplayMessage, this);
#endif

    // --- Step 3: Lock the board pool ---
    // No more boards can be added after this point
    _locked = true;
    _boards.shrink_to_fit();
    ESP_LOGI(TAG, "Board pool locked with %d board(s)", (int)_boards.size());

    // --- Step 4: Activate boot splash (skip if bootstrapped — already shown) ---
    if (!_bootstrapMode) {
        _bootBoard.activate(APP_FOUNDATION_VERSION);
        pushModal(&_bootBoard);
    }

    // --- Step 5: Exit bootstrap mode ---
    // From this point, tick() handles rendering via the main loop.
    _bootstrapMode = false;
    _lastBootProgress = 0.0f;

    // --- Step 6: Reset timers to current time ---
#ifndef NATIVE_BUILD
    _lastActivityMs = millis();
    _lastRotationMs = millis();
#endif

    ESP_LOGI(TAG, "Display module fully initialized — tick-driven rendering active");
}

/**
 * @brief Periodic tick — two-layer base+modal architecture.
 *
 * Called by the Application tick loop. If sleeping, returns early.
 * Flow:
 *   1. State polling (detect AppState changes)
 *   2. Auto-clear expired modals
 *   3. Phase-aware behaviour:
 *      - BOOT: always on, no dimming
 *      - CONFIGURING or modal active: dim after timeout, never blank
 *      - RUNNING + no modal: carousel rotation + sleep check
 *   4. Determine visible board: modal if active, else base layer
 *   5. Tick + render visible board
 */
void U8g2DisplayModule::tick() {
    // --- Early exit if sleeping ---
    if (_sleeping) {
        return;
    }

    // --- 1. State-driven lifecycle polling ---
    // Single code path for all transitions (boot, configure, regression).
    // Replaces EventBus subscriptions for app.configuring / app.running.
#ifndef NATIVE_BUILD
    if (_app != nullptr) {
        AppState currentState = _app->getState();
        if (currentState != _lastKnownState) {
            _onStateChanged(currentState);
            _lastKnownState = currentState;
        }
    }
#endif

    // --- 2. Auto-clear expired modals ---
    if (_modal != nullptr && _modal->isExpired()) {
        clearModal();
    }

    // --- 2b. Re-evaluate modal relevance during CONFIGURING ---
    // If the WiFi wizard is the active modal but WiFi is now connected,
    // clear it so the configuring status board (base layer) is visible.
    // This ensures the user sees the status board before RUNNING.
#ifndef NATIVE_BUILD
    if (_modal == &_wifiWizard && _app != nullptr &&
        _app->getState() == AppState::CONFIGURING) {
        // Check if the WiFi gate is now satisfied
        for (const auto* gate : _app->getConfigGates()) {
            if (strcmp(gate->getGateName(), "WiFi Station") == 0 && gate->isSatisfied()) {
                ESP_LOGI(TAG, "WiFi gate satisfied — clearing wizard modal");
                clearModal();
                break;
            }
        }
    }
#endif

    // --- 3. Phase-aware behaviour ---
#ifndef NATIVE_BUILD
    if (_app != nullptr) {
        if (_app->getState() == AppState::RUNNING && _modal == nullptr) {
            // RUNNING with no modal: carousel rotation + screensaver/sleep
            _checkSleep();
            if (!_boards.empty() && _activeBoardIndex >= 0) {
                unsigned long now = millis();
                if ((now - _lastRotationMs) >= _rotationIntervalMs) {
                    _advanceCarousel();
                    _lastRotationMs = now;
                }
            }
        } else if (_app->getState() != AppState::BOOT) {
            // CONFIGURING (base) or modal active (any phase): dim, never blank
            _checkDim();
        }
        // BOOT: no dimming, no sleep — always full brightness
    }
#endif

    // --- 4. Determine visible board ---
    IDisplayBoard* visible = nullptr;
    if (_modal != nullptr) {
        visible = _modal;
    } else {
        visible = _getBaseBoard();
        // Fallback: if _getBaseBoard() returns nullptr (native build or early boot),
        // try the carousel pool directly
        if (visible == nullptr && !_boards.empty() && _activeBoardIndex >= 0 &&
            _activeBoardIndex < (int)_boards.size()) {
            visible = _boards[_activeBoardIndex];
        }
    }

    // --- Guard: nothing to render ---
    if (visible == nullptr) {
        return;
    }

    // --- 5. Tick + render the visible board ---
#ifndef NATIVE_BUILD
    visible->tick(millis());
#else
    visible->tick(0);
#endif

    _renderCurrentBoard();
}

// =============================================================================
// Board Pool Management
// =============================================================================

/**
 * @brief Register a board into the carousel pool.
 * @param board Non-null pointer to an IDisplayBoard implementation.
 * @return true if added; false if locked or nullptr.
 */
bool U8g2DisplayModule::addBoard(IDisplayBoard* board) {
    if (_locked) {
        // Pool is locked after begin() — reject late registrations
        ESP_LOGW(TAG, "addBoard() rejected: pool is locked after begin()");
        return false;
    }

    if (board == nullptr) {
        ESP_LOGW(TAG, "addBoard() rejected: null board pointer");
        return false;
    }

    _boards.push_back(board);
    ESP_LOGD(TAG, "Board added (pool size: %d)", (int)_boards.size());
    return true;
}

/**
 * @brief Force-show a specific pool board, bypassing the carousel timer.
 *
 * Searches the pool for the board and updates the active index. Resets the
 * rotation timer. This method is carousel-only — for system/modal boards,
 * use pushModal() instead.
 *
 * Calls onDeactivate() on the previous board and onActivate() on the new one.
 * Publishes a "display.board.changed" event via EventBus.
 *
 * @param board Pointer to a pool board to display.
 */
void U8g2DisplayModule::showBoard(IDisplayBoard* board) {
    // --- Notify the previous board that it is being deactivated ---
    IDisplayBoard* previousBoard = getActiveBoard();
    if (previousBoard != nullptr && previousBoard != board) {
        previousBoard->onDeactivate();
    }

    // --- Search for the board in the pool ---
    for (int i = 0; i < (int)_boards.size(); i++) {
        if (_boards[i] == board) {
            // Found in pool — set as active and reset rotation timer
            _activeBoardIndex = i;
#ifndef NATIVE_BUILD
            _lastRotationMs = millis();
#endif
            break;
        }
    }

    // --- Notify the new board that it is now active ---
    if (board != nullptr && board != previousBoard) {
        board->onActivate();
    }

    // --- Publish board-changed event ---
#ifndef NATIVE_BUILD
    if (_app != nullptr && board != nullptr) {
        DisplayBoardChangedPayload payload;
        payload.boardName = board->getBoardName();
        payload.boardIndex = _activeBoardIndex;
        _app->getEventBus().publish("display.board.changed", &payload);
    }
#endif

    resetActivity();
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
void U8g2DisplayModule::pushModal(IDisplayBoard* board) {
    if (_modal != nullptr && _modal != board) {
        _modal->onDeactivate();
    }
    _modal = board;
    _modalDimmed = false;
    if (board != nullptr) {
        board->onActivate();
    }
    resetActivity();
}

/**
 * @brief Manually clear the active modal overlay.
 *
 * Deactivates the current modal, restores brightness to the configured
 * level, and returns rendering to the phase-determined base layer.
 */
void U8g2DisplayModule::clearModal() {
    if (_modal != nullptr) {
        _modal->onDeactivate();
        _modal = nullptr;
        _modalDimmed = false;
#ifndef NATIVE_BUILD
        _u8g2.setContrast(_brightness);
#endif
    }
}

/**
 * @brief Reset the inactivity timer. If sleeping, wake the display.
 */
void U8g2DisplayModule::resetActivity() {
#ifndef NATIVE_BUILD
    _lastActivityMs = millis();
#endif
    _modalDimmed = false;
    _sleepDiagLogged = false;
    if (_sleeping) {
        _wake();
    }

    // --- Dismiss screensaver if it's the active modal ---
    if (_modal == &_screensaver) {
        _screensaver.deactivate();
        // Modal auto-clears on next tick via isExpired() check
        ESP_LOGI(TAG, "Screensaver dismissed — activity detected");
    }
}

/**
 * @brief Return the number of boards in the carousel pool.
 * @return Pool size.
 */
int U8g2DisplayModule::getBoardCount() const {
    return (int)_boards.size();
}

/**
 * @brief Return the currently rendering board, or nullptr if none.
 * @return Pointer to the active board.
 */
IDisplayBoard* U8g2DisplayModule::getActiveBoard() const {
    // Modal layer takes priority
    if (_modal != nullptr) {
        return _modal;
    }

    // Bounds check on the pool
    if (_activeBoardIndex < 0 || _activeBoardIndex >= (int)_boards.size()) {
        return nullptr;
    }

    return _boards[_activeBoardIndex];
}

// =============================================================================
// System board accessors
// =============================================================================

/**
 * @brief Return a reference to the owned BootBoard instance.
 * @return Mutable reference to _bootBoard.
 */
BootBoard& U8g2DisplayModule::getBootBoard() {
    return _bootBoard;
}

/**
 * @brief Return a reference to the owned WiFiWizardBoard instance.
 * @return Mutable reference to _wifiWizard.
 */
WiFiWizardBoard& U8g2DisplayModule::getWifiWizard() {
    return _wifiWizard;
}

/**
 * @brief Return a reference to the owned OtaProgressBoard instance.
 * @return Mutable reference to _otaProgress.
 */
OtaProgressBoard& U8g2DisplayModule::getOtaProgress() {
    return _otaProgress;
}

/**
 * @brief Return a reference to the owned ErrorBoard instance.
 * @return Mutable reference to _error.
 */
ErrorBoard& U8g2DisplayModule::getError() {
    return _error;
}

/**
 * @brief Return a reference to the owned ScreensaverBoard instance.
 * @return Mutable reference to _screensaver.
 */
ScreensaverBoard& U8g2DisplayModule::getScreensaver() {
    return _screensaver;
}

/**
 * @brief Return a reference to the owned ConfiguringStatusBoard instance.
 * @return Mutable reference to _configuringStatusBoard.
 */
ConfiguringStatusBoard& U8g2DisplayModule::getConfiguringStatus() {
    return _configuringStatusBoard;
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
void U8g2DisplayModule::reportStatus(const char* message, float progress) {
    if (getActiveBoard() != &_bootBoard) {
        pushModal(&_bootBoard);
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
void U8g2DisplayModule::reportError(const char* errorMessage) {
    // --- Delegate to ErrorBoard with fatal flag ---
    _error.showError(errorMessage, true);

    // --- Force-show the error board ---
    pushModal(&_error);

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
void U8g2DisplayModule::clearStatus() {
    _bootBoard.setComplete();

    // --- If currently showing the boot board, advance carousel ---
    if (getActiveBoard() == &_bootBoard) {
        _advanceCarousel();
    }

    // --- Immediate render in bootstrap mode (show whatever board advanced to) ---
    if (_bootstrapMode) {
        _renderCurrentBoard();
    }
}

// =============================================================================
// iConfigurable — ConfigManager gateway (namespace: "display")
// =============================================================================

/**
 * @brief Apply display settings from a parsed JSON section.
 *
 * Called by ConfigManager during beginAll(). Reads brightness, flip,
 * sleepTimeout, and rotationInterval from the JSON object and applies
 * them to internal state and hardware.
 *
 * @param section Read-only JSON object containing display config keys.
 */
void U8g2DisplayModule::applyConfig(const JsonObject& section) {
    _brightness         = section["brightness"]      | (uint8_t)255;
    _flipScreen         = section["flip"]            | false;
    _sleepTimeoutMs     = section["sleepTimeout"]     | (unsigned long)300000;
    _rotationIntervalMs = section["rotationInterval"] | (unsigned long)8000;

    // --- Apply hardware settings ---
#ifndef NATIVE_BUILD
    _u8g2.setContrast(_brightness);
    _u8g2.setFlipMode(_flipScreen ? 1 : 0);
#endif

    ESP_LOGI(TAG, "Config applied (brightness=%d, flip=%d, sleep=%lu, rotation=%lu)",
             _brightness, _flipScreen, _sleepTimeoutMs, _rotationIntervalMs);
}

/**
 * @brief Serialize current display settings into a JSON section.
 *
 * Called by ConfigManager during saveModuleConfig(). Writes the four
 * display config keys into the provided JSON object. ConfigManager
 * appends the _v field separately.
 *
 * @param section Mutable JSON object to write config into.
 */
void U8g2DisplayModule::serializeConfig(JsonObject& section) const {
    section["brightness"]       = _brightness;
    section["flip"]             = _flipScreen;
    section["sleepTimeout"]     = _sleepTimeoutMs;
    section["rotationInterval"] = _rotationIntervalMs;
}

/**
 * @brief Reset display settings to factory defaults.
 *
 * Sets: brightness=255, flipScreen=false, sleepTimeout=300000ms,
 * rotationInterval=8000ms. Applies hardware settings. ConfigManager
 * will call saveModuleConfig() afterwards to persist.
 */
void U8g2DisplayModule::resetConfig() {
    // --- Set factory defaults ---
    _brightness         = 255;
    _flipScreen         = false;
    _sleepTimeoutMs     = 300000;
    _rotationIntervalMs = 8000;

    // --- Apply hardware settings ---
#ifndef NATIVE_BUILD
    _u8g2.setContrast(_brightness);
    _u8g2.setFlipMode(_flipScreen ? 1 : 0);
#endif
}

// =============================================================================
// Runtime display configuration setters
// =============================================================================

/**
 * @brief Set brightness and apply immediately to hardware.
 * @param brightness Display contrast/brightness level (0–255).
 */
void U8g2DisplayModule::setBrightness(uint8_t brightness) {
    _brightness = brightness;
#ifndef NATIVE_BUILD
    _u8g2.setContrast(_brightness);
#endif
}

/**
 * @brief Set the screen flip mode and apply immediately.
 * @param flip True to flip the display 180 degrees.
 */
void U8g2DisplayModule::setFlipScreen(bool flip) {
    _flipScreen = flip;
#ifndef NATIVE_BUILD
    _u8g2.setFlipMode(_flipScreen ? 1 : 0);
#endif
}

/**
 * @brief Set the inactivity sleep timeout.
 * @param timeoutMs Timeout in milliseconds (0 disables sleep).
 */
void U8g2DisplayModule::setSleepTimeout(unsigned long timeoutMs) {
    _sleepTimeoutMs = timeoutMs;
}

/**
 * @brief Set the carousel rotation interval.
 * @param intervalMs Interval in milliseconds between board rotations.
 */
void U8g2DisplayModule::setRotationInterval(unsigned long intervalMs) {
    _rotationIntervalMs = intervalMs;
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
void U8g2DisplayModule::registerRoutes(AsyncWebServer& server) {
#ifndef NATIVE_BUILD
    // --- GET /api/display/status — current display state ---
    server.on("/api/display/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["brightness"]       = _brightness;
        doc["flip"]             = _flipScreen;
        doc["sleeping"]         = _sleeping;
        doc["sleepTimeout"]     = _sleepTimeoutMs;
        doc["rotationInterval"] = _rotationIntervalMs;
        doc["boardCount"]       = (int)_boards.size();

        IDisplayBoard* active = getActiveBoard();
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
            if (_app != nullptr) {
                _app->getConfigManager().saveModuleConfig(this);
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
            if (_app != nullptr) {
                _app->getConfigManager().saveModuleConfig(this);
            }

            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );

    // --- POST /api/display/sleep — control sleep behaviour ---
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

            // Handle immediate sleep/wake toggle
            if (doc["action"].is<const char*>()) {
                const char* action = doc["action"];
                if (strcmp(action, "sleep") == 0) {
                    _sleep();
                } else if (strcmp(action, "wake") == 0) {
                    resetActivity();
                }
            }

            // Handle timeout configuration
            if (doc["timeout"].is<unsigned long>()) {
                setSleepTimeout(doc["timeout"] | (unsigned long)300000);
                if (_app != nullptr) {
                    _app->getConfigManager().saveModuleConfig(this);
                }
            }

            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );

    // --- GET /api/display/boards — list all pool boards ---
    server.on("/api/display/boards", HTTP_GET, [this](AsyncWebServerRequest* request) {
        JsonDocument doc;
        JsonArray boardsArr = doc["boards"].to<JsonArray>();

        for (int i = 0; i < (int)_boards.size(); i++) {
            JsonObject entry = boardsArr.add<JsonObject>();
            entry["index"]    = i;
            entry["name"]     = _boards[i]->getBoardName();
            entry["isActive"] = (i == _activeBoardIndex && _modal == nullptr);
            entry["isExpired"]= _boards[i]->isExpired();
        }

        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

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
            if (idx < 0 || idx >= (int)_boards.size()) {
                request->send(400, "application/json", "{\"error\":\"Index out of range\"}");
                return;
            }

            showBoard(_boards[idx]);
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );
#endif
}

// =============================================================================
// EventBus static callbacks
// =============================================================================

/**
 * @brief Static callback for "ota.progress" EventBus events.
 *
 * Casts the context to U8g2DisplayModule*, the payload to OtaProgressPayload*,
 * and delegates to the OtaProgressBoard. Shows the OTA board on screen.
 *
 * @param topic   Topic string ("ota.progress").
 * @param payload Pointer to OtaProgressPayload.
 * @param ctx     The U8g2DisplayModule instance (passed during subscribe).
 */
void U8g2DisplayModule::_onOtaProgress(const char* /*topic*/, const void* payload, void* ctx) {
    auto* self = static_cast<U8g2DisplayModule*>(ctx);
    if (payload == nullptr || self == nullptr) {
        return;
    }

    const auto* ota = static_cast<const OtaProgressPayload*>(payload);

    if (ota->complete) {
        // --- OTA cycle finished — mark board as complete ---
        self->_otaProgress.setComplete(ota->success);
    } else {
        // --- OTA in progress — update progress bar ---
        self->_otaProgress.setProgress(ota->progress, ota->status);
    }

    // --- Push OTA board as modal overlay ---
    self->pushModal(&self->_otaProgress);
}

/**
 * @brief Static callback for "display.message" EventBus events.
 *
 * Casts the context to U8g2DisplayModule*, the payload to
 * DisplayMessagePayload*, and shows the message via the BootBoard overlay.
 * Resets the inactivity timer to keep the display awake.
 *
 * @param topic   Topic string ("display.message").
 * @param payload Pointer to DisplayMessagePayload.
 * @param ctx     The U8g2DisplayModule instance (passed during subscribe).
 */
void U8g2DisplayModule::_onDisplayMessage(const char* /*topic*/, const void* payload, void* ctx) {
    auto* self = static_cast<U8g2DisplayModule*>(ctx);
    if (payload == nullptr || self == nullptr) {
        return;
    }

    const auto* msg = static_cast<const DisplayMessagePayload*>(payload);

    // --- Use the boot board as a temporary message overlay ---
    self->_bootBoard.setStatus(msg->message, -1.0f);

    // --- Push boot board as modal overlay ---
    self->pushModal(&self->_bootBoard);
}

// =============================================================================
// State-polling lifecycle handler
// =============================================================================

/**
 * @brief React to an Application state transition detected by tick().
 *
 * Single code path for all lifecycle transitions — replaces both the
 * EventBus callbacks (_onAppConfiguring, _onAppRunning) and the late
 * catch-up block that was previously in begin().
 *
 * @param newState The newly detected Application state.
 */
void U8g2DisplayModule::_onStateChanged(AppState newState) {
#ifndef NATIVE_BUILD
    if (newState == AppState::CONFIGURING) {
        _configuringStatusBoard.setGates(&_app->getConfigGates());

        // Check if WiFi gate needs a modal wizard
        for (const auto* gate : _app->getConfigGates()) {
            if (!gate->isSatisfied() && strcmp(gate->getGateName(), "WiFi Station") == 0) {
                // WiFi gate unsatisfied — device is/will-be in AP mode.
                // AP always broadcasts as hostname.
                _wifiWizard.configure(_app->getConfig().hostname);
                pushModal(&_wifiWizard);
                ESP_LOGI(TAG, "State → CONFIGURING: WiFi wizard modal pushed");
                return;
            }
        }

        // Non-WiFi unsatisfied gates: set connection info on configuring board.
        // The base layer (_getBaseBoard) auto-shows configuringStatusBoard.
        for (const auto* gate : _app->getConfigGates()) {
            if (!gate->isSatisfied()) {
                _configuringStatusBoard.setConnectionInfo(
                    _app->getWifiManager().getSSID(),
                    _app->getWifiManager().getMyUrl()
                );
                break;
            }
        }
        // No modal needed — base layer (configuring status) shows automatically
        ESP_LOGI(TAG, "State → CONFIGURING: base layer active");

    } else if (newState == AppState::RUNNING) {
        // --- Step 1: Clear any active modal (e.g. WiFi wizard) ---
        clearModal();

        // --- Step 2: Flash the configuring status board showing all-green ---
        // Force one render cycle so the user sees the completed state
        // before the carousel takes over.
        _configuringStatusBoard.setGates(&_app->getConfigGates());
        _u8g2.clearBuffer();
        _configuringStatusBoard.render(_u8g2);
        _u8g2.sendBuffer();

        // --- Step 3: Dismiss system boards and switch to carousel ---
        _wifiWizard.dismiss();
        _configuringStatusBoard.dismiss();
        ESP_LOGI(TAG, "State → RUNNING: configuring complete, carousel starting");
    }
#endif
}

// =============================================================================
// Private carousel engine
// =============================================================================

/**
 * @brief Advance carousel to next non-expired board.
 *
 * Calls onDeactivate() on the current board, removes expired boards
 * (isExpired() == true) from the pool, advances _activeBoardIndex to
 * the next valid entry with wrap-around, calls onActivate() on the new
 * board, and publishes a "display.board.changed" event.
 */
void U8g2DisplayModule::_advanceCarousel() {
    // --- Step 1: Deactivate the current board ---
    IDisplayBoard* previousBoard = getActiveBoard();
    if (previousBoard != nullptr) {
        previousBoard->onDeactivate();
    }

    // --- Step 2: Evict expired boards ---
    // Iterate backwards to safely erase without invalidating indices
    for (int i = (int)_boards.size() - 1; i >= 0; i--) {
        if (_boards[i]->isExpired()) {
            _boards.erase(_boards.begin() + i);
            // Adjust active index if it was at or past the erased position
            if (_activeBoardIndex >= (int)_boards.size()) {
                _activeBoardIndex = 0;
            }
        }
    }

    // --- Step 3: Guard empty pool ---
    if (_boards.empty()) {
        _activeBoardIndex = 0;

        // Clear stale modal if it's an expired system board
        if (_modal != nullptr && _modal->isExpired()) {
            _modal = nullptr;
        }
        return;
    }

    // --- Step 4: Advance to next board (wrap around) ---
    _activeBoardIndex = (_activeBoardIndex + 1) % (int)_boards.size();

    // --- Step 5: Activate the new board ---
    IDisplayBoard* newBoard = getActiveBoard();
    if (newBoard != nullptr) {
        newBoard->onActivate();

        // --- Publish board-changed event ---
#ifndef NATIVE_BUILD
        if (_app != nullptr) {
            DisplayBoardChangedPayload payload;
            payload.boardName = newBoard->getBoardName();
            payload.boardIndex = _activeBoardIndex;
            _app->getEventBus().publish("display.board.changed", &payload);
        }
#endif
    }
}

/**
 * @brief Render the current board into the U8G2 display buffer.
 *
 * Uses clearBuffer() → render() → sendBuffer() inside NATIVE_BUILD guard.
 */
void U8g2DisplayModule::_renderCurrentBoard() {
#ifndef NATIVE_BUILD
    // --- Determine which board to render ---
    IDisplayBoard* boardToRender = nullptr;

    if (_modal != nullptr) {
        // Modal layer takes priority
        boardToRender = _modal;
    } else if (_activeBoardIndex >= 0 && _activeBoardIndex < (int)_boards.size()) {
        // Normal pool board (carousel base layer for RUNNING)
        boardToRender = _boards[_activeBoardIndex];
    } else {
        // Base layer for non-RUNNING phases
        boardToRender = _getBaseBoard();
    }

    if (boardToRender == nullptr) {
        return;
    }

    // --- U8G2 draw loop ---
    _u8g2.clearBuffer();
    boardToRender->render(_u8g2);
    _u8g2.sendBuffer();
#endif
}

/**
 * @brief Check inactivity timer and enter sleep if timed out.
 */
void U8g2DisplayModule::_checkSleep() {
#ifndef NATIVE_BUILD
    // Sleep disabled if timeout is 0
    if (_sleepTimeoutMs == 0) {
        return;
    }

    unsigned long elapsed = millis() - _lastActivityMs;

    // --- Diagnostic: one-shot log at 50% of timeout ---
    if (!_sleepDiagLogged && elapsed >= (_sleepTimeoutMs / 2)) {
        ESP_LOGW(TAG, "Sleep diagnostic: elapsed=%lu timeout=%lu (50%% reached)",
                 elapsed, _sleepTimeoutMs);
        _sleepDiagLogged = true;
    }

    if (elapsed >= _sleepTimeoutMs) {
        _sleep();
    }
#endif
}

/**
 * @brief Derive the base-layer board from the current Application state.
 * @return Phase-appropriate base board, or nullptr.
 */
IDisplayBoard* U8g2DisplayModule::_getBaseBoard() {
#ifndef NATIVE_BUILD
    if (_app == nullptr) return nullptr;
    switch (_app->getState()) {
        case AppState::BOOT:
            return &_bootBoard;
        case AppState::CONFIGURING:
            return &_configuringStatusBoard;
        case AppState::RUNNING: {
            // Return the current carousel pool board
            if (!_boards.empty()) {
                // Reset index if it was left invalid (e.g. after modal clear)
                if (_activeBoardIndex < 0 || _activeBoardIndex >= (int)_boards.size()) {
                    _activeBoardIndex = 0;
                }
                return _boards[_activeBoardIndex];
            }
            return nullptr;
        }
        default:
            return nullptr;
    }
#else
    return nullptr;
#endif
}

/**
 * @brief Check inactivity timer and dim display if timed out.
 * Dims to minimum brightness (1) — never blanks. Used during
 * CONFIGURING and when a modal is active.
 */
void U8g2DisplayModule::_checkDim() {
#ifndef NATIVE_BUILD
    if (_sleepTimeoutMs == 0) return;  // Dimming disabled

    unsigned long elapsed = millis() - _lastActivityMs;
    if (elapsed >= _sleepTimeoutMs && !_modalDimmed) {
        _u8g2.setContrast(1);  // Minimum non-zero brightness
        _modalDimmed = true;
        ESP_LOGI(TAG, "Display dimmed (inactivity during configuring/modal)");
    }
#endif
}

/**
 * @brief Enter sleep mode — activate the screensaver for burn-in prevention.
 *
 * Instead of powering off the OLED, this pushes the screensaver as a modal
 * overlay. The display stays awake (_sleeping is NOT set) so the tick loop
 * continues running to tick/render the screensaver. The screensaver's
 * isExpired() returns !_active, so while active it won't auto-clear.
 * _checkSleep() won't fire again because _modal != nullptr routes tick()
 * to the dim path rather than the RUNNING+no-modal sleep path.
 *
 * Dismissal: resetActivity() deactivates the screensaver, and the modal
 * auto-clears on the next tick via the isExpired() check.
 */
void U8g2DisplayModule::_sleep() {
    ESP_LOGI(TAG, "Display entering idle — activating screensaver");

    // --- Activate screensaver as a modal overlay ---
    // Do NOT set _sleeping = true — the tick loop must continue
    // running to tick/render the screensaver modal.
    // Do NOT call setPowerSave(1) — OLED stays on for the screensaver.
    _screensaver.activate();
    pushModal(&_screensaver);
}

/**
 * @brief Exit sleep mode — restore display power and brightness.
 *
 * Clears the sleeping flag, restores OLED power and brightness,
 * and publishes a "display.wake" event.
 */
void U8g2DisplayModule::_wake() {
    ESP_LOGI(TAG, "Display waking from sleep");
    _sleeping = false;

    // --- Deactivate the screensaver (if it was running) ---
    _screensaver.deactivate();

    // --- Restore OLED power and brightness ---
#ifndef NATIVE_BUILD
    _u8g2.setPowerSave(0);
    _u8g2.setContrast(_brightness);
#endif

    // --- Publish wake event ---
#ifndef NATIVE_BUILD
    if (_app != nullptr) {
        _app->getEventBus().publish("display.wake");
    }
#endif
}
