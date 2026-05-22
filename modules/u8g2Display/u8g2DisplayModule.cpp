/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/u8g2DisplayModule.cpp
 * Description: Implementation of the U8G2 display module with carousel engine,
 *              board slot pool, sleep/wake power management, system board
 *              ownership, IStatusReporter delegation (Loading/Error boards),
 *              EventBus subscriptions (ota.progress, display.message), and
 *              iConfigurable persistence at /mod/display.json.
 *
 * Exported Functions:
 * - U8g2DisplayModule::begin():              Hardware init, EventBus subscribe, pool lock
 * - U8g2DisplayModule::tick():               Carousel rotation, sleep check, render
 * - U8g2DisplayModule::addBoard():           Pre-lock board registration
 * - U8g2DisplayModule::showBoard():          Force-show a specific board
 * - U8g2DisplayModule::resetActivity():      Reset inactivity timer, wake if sleeping
 * - U8g2DisplayModule::getBoardCount():      Return pool size
 * - U8g2DisplayModule::getActiveBoard():     Return current rendering board or nullptr
 * - U8g2DisplayModule::getBootSplash():      Return reference to BootSplashBoard
 * - U8g2DisplayModule::getLoading():         Return reference to LoadingBoard
 * - U8g2DisplayModule::getWifiWizard():      Return reference to WiFiWizardBoard
 * - U8g2DisplayModule::getOtaProgress():     Return reference to OtaProgressBoard
 * - U8g2DisplayModule::getError():           Return reference to ErrorBoard
 * - U8g2DisplayModule::getScreensaver():     Return reference to ScreensaverBoard
 * - U8g2DisplayModule::reportStatus():       Delegate to LoadingBoard, show it
 * - U8g2DisplayModule::reportError():        Delegate to ErrorBoard (fatal), show it
 * - U8g2DisplayModule::clearStatus():        Mark loading complete, advance if showing
 * - U8g2DisplayModule::loadConfig():         Load display settings from LittleFS JSON
 * - U8g2DisplayModule::saveConfig():         Persist display settings to LittleFS JSON
 * - U8g2DisplayModule::resetConfig():        Reset to factory defaults, persist, apply
 * - U8g2DisplayModule::setBrightness():      Runtime brightness adjustment
 * - U8g2DisplayModule::setFlipScreen():      Runtime screen flip toggle
 * - U8g2DisplayModule::setSleepTimeout():    Runtime sleep timeout adjustment
 * - U8g2DisplayModule::setRotationInterval(): Runtime carousel interval adjustment
 * - U8g2DisplayModule::_onOtaProgress():     Static EventBus callback for OTA events
 * - U8g2DisplayModule::_onDisplayMessage():  Static EventBus callback for display messages
 */

#include "u8g2DisplayModule.hpp"
#include "platform/LogShim.h"

static const char* TAG = "U8g2Display";

#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <LittleFS.h>
#include "../application/Application.hpp"
#endif

// Include EventBus directly — it is native-safe (no Arduino.h dependency).
#include "../eventBus/EventBus.hpp"

// =============================================================================
// Lifecycle
// =============================================================================

/**
 * @brief Initialize the U8G2 hardware, subscribe to EventBus, and lock the pool.
 * @param app Reference to the Application composition root.
 */
void U8g2DisplayModule::begin(Application& app) {
    _app = &app;

    // --- Step 1: Initialize hardware display driver ---
#ifndef NATIVE_BUILD
    ESP_LOGI(TAG, "Initializing hardware display driver");
    _u8g2.begin();
    _u8g2.setContrast(_brightness);
    _u8g2.setFlipMode(_flipScreen ? 1 : 0);
#endif

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

    // --- Step 4: Activate boot splash and show it ---
    _bootSplash.activate(APP_FOUNDATION_VERSION);
    showBoard(&_bootSplash);

    // --- Step 5: Reset timers to current time ---
#ifndef NATIVE_BUILD
    _lastActivityMs = millis();
    _lastRotationMs = millis();
#endif
}

/**
 * @brief Periodic tick — carousel rotation, sleep check, render.
 *
 * Called by the Application tick loop. If sleeping, returns early.
 * Otherwise checks the sleep timer, evaluates the carousel rotation
 * interval, and renders the current board.
 */
void U8g2DisplayModule::tick() {
    // --- Early exit if sleeping ---
    if (_sleeping) {
        return;
    }

    // --- Check inactivity timer ---
    _checkSleep();

    // --- Guard: no boards to render ---
    if (_boards.empty() && _overrideBoard == nullptr) {
        return;
    }

    // --- Carousel rotation ---
    // Only rotate if we have pool boards and no override is active
    if (!_boards.empty() && _activeBoardIndex >= 0) {
#ifndef NATIVE_BUILD
        unsigned long now = millis();
#else
        unsigned long now = 0;
#endif
        if ((now - _lastRotationMs) >= _rotationIntervalMs) {
            _advanceCarousel();
            _lastRotationMs = now;
        }
    }

    // --- Render ---
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
 * @brief Force-show a specific board, bypassing the carousel timer.
 * @param board Pointer to the board to display.
 */
void U8g2DisplayModule::showBoard(IDisplayBoard* board) {
    // --- Search for the board in the pool ---
    for (int i = 0; i < (int)_boards.size(); i++) {
        if (_boards[i] == board) {
            // Found in pool — set as active and reset rotation timer
            _activeBoardIndex = i;
            _overrideBoard = nullptr;
#ifndef NATIVE_BUILD
            _lastRotationMs = millis();
#endif
            resetActivity();
            return;
        }
    }

    // --- Board not in pool (system board override) ---
    // Render it directly without affecting the carousel index
    ESP_LOGD(TAG, "Override board set (not in pool)");
    _activeBoardIndex = -1;
    _overrideBoard = board;
    resetActivity();
}

/**
 * @brief Reset the inactivity timer. If sleeping, wake the display.
 */
void U8g2DisplayModule::resetActivity() {
#ifndef NATIVE_BUILD
    _lastActivityMs = millis();
#endif
    if (_sleeping) {
        _wake();
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
    // If an override board is active, return it
    if (_activeBoardIndex < 0 && _overrideBoard != nullptr) {
        return _overrideBoard;
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
 * @brief Return a reference to the owned BootSplashBoard instance.
 * @return Mutable reference to _bootSplash.
 */
BootSplashBoard& U8g2DisplayModule::getBootSplash() {
    return _bootSplash;
}

/**
 * @brief Return a reference to the owned LoadingBoard instance.
 * @return Mutable reference to _loading.
 */
LoadingBoard& U8g2DisplayModule::getLoading() {
    return _loading;
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

// =============================================================================
// IStatusReporter — real implementations delegating to system boards
// =============================================================================

/**
 * @brief Report status by delegating to LoadingBoard.
 *
 * Updates the loading board's message and progress, shows the loading board
 * if it isn't already the active board, and resets the inactivity timer.
 *
 * @param message  Status text for the loading board.
 * @param progress Fractional progress [0.0, 1.0]; negative = indeterminate.
 */
void U8g2DisplayModule::reportStatus(const char* message, float progress) {
    // --- Delegate to LoadingBoard ---
    _loading.setStatus(message, progress);

    // --- Show the loading board if not already active ---
    if (getActiveBoard() != &_loading) {
        showBoard(&_loading);
    }

    resetActivity();
}

/**
 * @brief Report a fatal error by delegating to ErrorBoard.
 *
 * Displays the error as fatal (no auto-dismiss), forces the error board
 * onto the screen, and resets the inactivity timer.
 *
 * @param errorMessage Error description text.
 */
void U8g2DisplayModule::reportError(const char* errorMessage) {
    // --- Delegate to ErrorBoard with fatal flag ---
    _error.showError(errorMessage, true);

    // --- Force-show the error board ---
    showBoard(&_error);

    resetActivity();
}

/**
 * @brief Clear the active status overlay.
 *
 * Marks the LoadingBoard as complete (triggers isExpired()). If the loading
 * board is currently the active board, advances the carousel to move past it.
 */
void U8g2DisplayModule::clearStatus() {
    _loading.setComplete();

    // --- If currently showing the loading board, advance carousel ---
    if (getActiveBoard() == &_loading) {
        _advanceCarousel();
    }
}

// =============================================================================
// iConfigurable — persistent display settings at /mod/display.json
// =============================================================================

/**
 * @brief Load display configuration from /mod/display.json.
 *
 * On hardware: Opens the JSON file, parses keys (brightness, flip,
 * sleepTimeout, rotationInterval), and applies defaults for missing keys.
 * In NATIVE_BUILD: Returns true immediately (no LittleFS available).
 *
 * @return true if config loaded or defaults applied; false on parse error.
 */
bool U8g2DisplayModule::loadConfig() {
#ifndef NATIVE_BUILD
    ESP_LOGI(TAG, "Loading config from /mod/display.json");
    // --- Open the config file from LittleFS ---
    File configFile = LittleFS.open("/mod/display.json", "r");
    if (!configFile) {
        // File not found — apply defaults (already set by member initialisers)
        ESP_LOGW(TAG, "Config file /mod/display.json not found, using defaults");
        return false;
    }

    // --- Parse JSON with a fixed-size document (rule 10: no DynamicJsonDocument) ---
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, configFile);
    configFile.close();

    if (err) {
        // Parse failure — defaults remain in place
        ESP_LOGE(TAG, "Failed to parse /mod/display.json");
        return false;
    }

    // --- Read keys with fallback defaults ---
    _brightness         = doc["brightness"]         | (uint8_t)255;
    _flipScreen         = doc["flip"]               | false;
    _sleepTimeoutMs     = doc["sleepTimeout"]        | (unsigned long)300000;
    _rotationIntervalMs = doc["rotationInterval"]    | (unsigned long)8000;

    // --- Apply hardware settings ---
    _u8g2.setContrast(_brightness);
    _u8g2.setFlipMode(_flipScreen ? 1 : 0);

    ESP_LOGI(TAG, "Config loaded successfully (brightness=%d, flip=%d)", _brightness, _flipScreen);
    return true;
#else
    // NATIVE_BUILD — no LittleFS, defaults are fine
    return true;
#endif
}

/**
 * @brief Persist display configuration to /mod/display.json.
 *
 * On hardware: Creates /mod/ directory if needed, serializes current settings
 * to JSON, and writes to LittleFS.
 * In NATIVE_BUILD: Returns true immediately.
 *
 * @return true if write succeeded.
 */
bool U8g2DisplayModule::saveConfig() {
#ifndef NATIVE_BUILD
    ESP_LOGI(TAG, "Saving config to /mod/display.json");
    // --- Ensure the /mod/ directory exists ---
    if (!LittleFS.exists("/mod")) {
        LittleFS.mkdir("/mod");
    }

    // --- Serialize settings to JSON ---
    JsonDocument doc;
    doc["brightness"]       = _brightness;
    doc["flip"]             = _flipScreen;
    doc["sleepTimeout"]     = _sleepTimeoutMs;
    doc["rotationInterval"] = _rotationIntervalMs;

    // --- Write to LittleFS ---
    File configFile = LittleFS.open("/mod/display.json", "w");
    if (!configFile) {
        ESP_LOGE(TAG, "Failed to open /mod/display.json for writing");
        return false;
    }

    serializeJson(doc, configFile);
    configFile.close();
    ESP_LOGI(TAG, "Config saved successfully");
    return true;
#else
    // NATIVE_BUILD — no LittleFS
    return true;
#endif
}

/**
 * @brief Reset display settings to factory defaults, persist, and apply.
 *
 * Sets: brightness=255, flipScreen=false, sleepTimeout=300000ms,
 * rotationInterval=8000ms. Applies hardware contrast/flip and saves to disk.
 */
void U8g2DisplayModule::resetConfig() {
    // --- Set factory defaults ---
    _brightness         = 255;
    _flipScreen         = false;
    _sleepTimeoutMs     = 300000;
    _rotationIntervalMs = 8000;

    // --- Persist to disk ---
    saveConfig();

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
        // --- OTA cycle finished — mark board complete ---
        self->_otaProgress.setComplete(ota->success);
    } else {
        // --- OTA in progress — update progress bar ---
        self->_otaProgress.setProgress(ota->progress, ota->status);
    }

    // --- Show the OTA board ---
    self->showBoard(&self->_otaProgress);
}

/**
 * @brief Static callback for "display.message" EventBus events.
 *
 * Casts the context to U8g2DisplayModule*, the payload to
 * DisplayMessagePayload*, and shows the message via the LoadingBoard overlay.
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

    // --- Use the loading board as a temporary message overlay ---
    self->_loading.setStatus(msg->message, -1.0f);

    // --- Show the loading board ---
    self->showBoard(&self->_loading);

    self->resetActivity();
}

// =============================================================================
// Private carousel engine
// =============================================================================

/**
 * @brief Advance carousel to next non-expired board.
 *
 * Removes expired boards (isExpired() == true) from the pool, then
 * advances _activeBoardIndex to the next valid entry with wrap-around.
 */
void U8g2DisplayModule::_advanceCarousel() {
    // --- Step 1: Evict expired boards ---
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

    // --- Step 2: Guard empty pool ---
    if (_boards.empty()) {
        _activeBoardIndex = 0;
        return;
    }

    // --- Step 3: Advance to next board (wrap around) ---
    _activeBoardIndex = (_activeBoardIndex + 1) % (int)_boards.size();
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

    if (_activeBoardIndex < 0 && _overrideBoard != nullptr) {
        // System board override — render the override board directly
        boardToRender = _overrideBoard;
    } else if (_activeBoardIndex >= 0 && _activeBoardIndex < (int)_boards.size()) {
        // Normal pool board
        boardToRender = _boards[_activeBoardIndex];
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
    if (elapsed >= _sleepTimeoutMs) {
        _sleep();
    }
#endif
}

/**
 * @brief Enter sleep mode — power-save the display.
 */
void U8g2DisplayModule::_sleep() {
    ESP_LOGI(TAG, "Display entering sleep mode");
    _sleeping = true;

    // --- Activate the screensaver and show it ---
    _screensaver.activate();
    showBoard(&_screensaver);

#ifndef NATIVE_BUILD
    _u8g2.setPowerSave(1);
#endif
}

/**
 * @brief Exit sleep mode — restore display power and brightness.
 */
void U8g2DisplayModule::_wake() {
    ESP_LOGI(TAG, "Display waking from sleep");
    _sleeping = false;

    // --- Deactivate the screensaver ---
    _screensaver.deactivate();

#ifndef NATIVE_BUILD
    _u8g2.setPowerSave(0);
    _u8g2.setContrast(_brightness);
#endif
}
