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
 *              ownership (BootSplash, Loading, WiFiWizard), and stub
 *              IStatusReporter / iConfigurable methods.
 *
 * Exported Functions:
 * - U8g2DisplayModule::begin():            Hardware init, pool lock, timer reset
 * - U8g2DisplayModule::tick():             Carousel rotation, sleep check, render
 * - U8g2DisplayModule::addBoard():         Pre-lock board registration
 * - U8g2DisplayModule::showBoard():        Force-show a specific board
 * - U8g2DisplayModule::resetActivity():    Reset inactivity timer, wake if sleeping
 * - U8g2DisplayModule::getBoardCount():    Return pool size
 * - U8g2DisplayModule::getActiveBoard():   Return current rendering board or nullptr
 * - U8g2DisplayModule::getBootSplash():    Return reference to BootSplashBoard
 * - U8g2DisplayModule::getLoading():       Return reference to LoadingBoard
 * - U8g2DisplayModule::getWifiWizard():    Return reference to WiFiWizardBoard
 * - U8g2DisplayModule::reportStatus():     IStatusReporter stub (no-op)
 * - U8g2DisplayModule::reportError():      IStatusReporter stub (no-op)
 * - U8g2DisplayModule::clearStatus():      IStatusReporter stub (no-op)
 * - U8g2DisplayModule::loadConfig():       iConfigurable stub (returns true)
 * - U8g2DisplayModule::saveConfig():       iConfigurable stub (returns true)
 * - U8g2DisplayModule::resetConfig():      iConfigurable stub (no-op)
 */

#include "u8g2DisplayModule.hpp"

#ifndef NATIVE_BUILD
#include <Arduino.h>
#endif

// =============================================================================
// Lifecycle
// =============================================================================

/**
 * @brief Initialize the U8G2 hardware, apply display settings, and lock the pool.
 * @param app Reference to the Application composition root.
 */
void U8g2DisplayModule::begin(Application& app) {
    _app = &app;

    // --- Step 1: Initialize hardware display driver ---
#ifndef NATIVE_BUILD
    _u8g2.begin();
    _u8g2.setContrast(_brightness);
    _u8g2.setFlipMode(_flipScreen ? 1 : 0);
#endif

    // --- Step 2: Lock the board pool ---
    // No more boards can be added after this point
    _locked = true;
    _boards.shrink_to_fit();

    // --- Step 3: Activate boot splash and show it ---
    _bootSplash.activate(APP_FOUNDATION_VERSION);
    showBoard(&_bootSplash);

    // --- Step 4: Reset timers to current time ---
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
        return false;
    }

    if (board == nullptr) {
        return false;
    }

    _boards.push_back(board);
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

// =============================================================================
// IStatusReporter stubs (empty body — deferred to WO2/3)
// =============================================================================

void U8g2DisplayModule::reportStatus(const char* /*message*/, float /*progress*/) {
    // Stub — no-op
}

void U8g2DisplayModule::reportError(const char* /*errorMessage*/) {
    // Stub — no-op
}

void U8g2DisplayModule::clearStatus() {
    // Stub — no-op
}

// =============================================================================
// iConfigurable stubs (return defaults — deferred to WO4)
// =============================================================================

bool U8g2DisplayModule::loadConfig() {
    return true;
}

bool U8g2DisplayModule::saveConfig() {
    return true;
}

void U8g2DisplayModule::resetConfig() {
    // Stub — no-op
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
    _sleeping = true;
#ifndef NATIVE_BUILD
    _u8g2.setPowerSave(1);
#endif
}

/**
 * @brief Exit sleep mode — restore display power and brightness.
 */
void U8g2DisplayModule::_wake() {
    _sleeping = false;
#ifndef NATIVE_BUILD
    _u8g2.setPowerSave(0);
    _u8g2.setContrast(_brightness);
#endif
}
