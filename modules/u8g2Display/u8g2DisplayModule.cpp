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
 * - U8g2DisplayModule::getBootSplash():      Return reference to BootSplashBoard
 * - U8g2DisplayModule::getLoading():         Return reference to LoadingBoard
 * - U8g2DisplayModule::getWifiWizard():      Return reference to WiFiWizardBoard
 * - U8g2DisplayModule::getOtaProgress():     Return reference to OtaProgressBoard
 * - U8g2DisplayModule::getError():           Return reference to ErrorBoard
 * - U8g2DisplayModule::getScreensaver():     Return reference to ScreensaverBoard
 * - U8g2DisplayModule::reportStatus():       Delegate to LoadingBoard, show it
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
 */

#include "u8g2DisplayModule.hpp"
#include "platform/LogShim.h"

static const char* TAG = "U8g2Display";

#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "../application/Application.hpp"
#endif

// Include EventBus directly — it is native-safe (no Arduino.h dependency).
#include "../eventBus/EventBus.hpp"
#include "../configManager/configManager.hpp"

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
 * @brief Periodic tick — carousel rotation, sleep check, board tick, render.
 *
 * Called by the Application tick loop. If sleeping, returns early.
 * Otherwise checks the sleep timer, delegates tick() to the active board,
 * evaluates the carousel rotation interval, and renders the current board.
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

    // --- Delegate tick() to the active board ---
    // This allows boards to update scroll positions, animation timers, etc.
    IDisplayBoard* activeBoard = getActiveBoard();
    if (activeBoard != nullptr) {
#ifndef NATIVE_BUILD
        activeBoard->tick(millis());
#else
        activeBoard->tick(0);
#endif
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
 *
 * Calls onDeactivate() on the previous board and onActivate() on the new one.
 * Publishes a "display.board.changed" event via EventBus.
 *
 * @param board Pointer to the board to display.
 */
void U8g2DisplayModule::showBoard(IDisplayBoard* board) {
    // --- Notify the previous board that it is being deactivated ---
    IDisplayBoard* previousBoard = getActiveBoard();
    if (previousBoard != nullptr && previousBoard != board) {
        previousBoard->onDeactivate();
    }

    // --- Search for the board in the pool ---
    int newIndex = -1;
    for (int i = 0; i < (int)_boards.size(); i++) {
        if (_boards[i] == board) {
            // Found in pool — set as active and reset rotation timer
            _activeBoardIndex = i;
            _overrideBoard = nullptr;
            newIndex = i;
#ifndef NATIVE_BUILD
            _lastRotationMs = millis();
#endif
            break;
        }
    }

    if (newIndex < 0) {
        // --- Board not in pool (system board override) ---
        // Render it directly without affecting the carousel index
        ESP_LOGD(TAG, "Override board set (not in pool)");
        _activeBoardIndex = -1;
        _overrideBoard = board;
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
        payload.boardIndex = newIndex;
        _app->getEventBus().publish("display.board.changed", &payload);
    }
#endif

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
            entry["isActive"] = (i == _activeBoardIndex && _overrideBoard == nullptr);
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
 *
 * CRITICAL: The order of operations prevents the race condition where
 * showBoard() → resetActivity() → _wake() would immediately undo sleep.
 * We set _sleeping=true FIRST, then power-save the OLED, then publish.
 * The screensaver is NOT shown here because the OLED is being powered off;
 * screensaver logic should be handled separately if a visible screensaver
 * phase is desired before full power-off.
 */
void U8g2DisplayModule::_sleep() {
    ESP_LOGI(TAG, "Display entering sleep mode");

    // --- Step 1: Set sleeping flag FIRST to prevent re-entry ---
    _sleeping = true;

    // --- Step 2: Power-save the OLED hardware ---
#ifndef NATIVE_BUILD
    _u8g2.setPowerSave(1);
#endif

    // --- Step 3: Publish sleep event ---
#ifndef NATIVE_BUILD
    if (_app != nullptr) {
        _app->getEventBus().publish("display.sleep");
    }
#endif
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
