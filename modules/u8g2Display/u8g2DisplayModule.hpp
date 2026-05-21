/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/u8g2DisplayModule.hpp
 * Description: U8G2 monochrome display module with carousel engine and board
 *              slot pool. Manages a vector of IDisplayBoard* entries, rotating
 *              through them on a configurable interval. Provides sleep/wake
 *              power management, brightness control, and screen flip support.
 *              System boards (BootSplash, Loading, WiFiWizard, OtaProgress,
 *              Error, Screensaver) can bypass the carousel via showBoard().
 *              Owns all system board instances and activates the splash on begin().
 *
 * Exported Classes:
 * - U8g2DisplayModule: Display carousel engine and board slot pool
 *   - begin():          Initialize hardware, apply settings, lock the board pool
 *   - tick():           Carousel rotation, sleep check, render current board
 *   - addBoard():       Pre-lock registration of boards into the carousel pool
 *   - showBoard():      Force-show a specific board (bypasses carousel timer)
 *   - resetActivity():  Reset the inactivity timer (called on user interaction)
 *   - getBoardCount():  Return the number of registered boards in the pool
 *   - getActiveBoard(): Return the currently rendering board (or nullptr)
 *   - getBootSplash():  Return reference to the owned BootSplashBoard instance
 *   - getLoading():     Return reference to the owned LoadingBoard instance
 *   - getWifiWizard():  Return reference to the owned WiFiWizardBoard instance
 *   - getOtaProgress(): Return reference to the owned OtaProgressBoard instance
 *   - getError():       Return reference to the owned ErrorBoard instance
 *   - getScreensaver(): Return reference to the owned ScreensaverBoard instance
 *   - reportStatus():   IStatusReporter stub (empty body — WO3)
 *   - reportError():    IStatusReporter stub (empty body — WO3)
 *   - clearStatus():    IStatusReporter stub (empty body — WO3)
 *   - loadConfig():     iConfigurable stub (returns true — WO4)
 *   - saveConfig():     iConfigurable stub (returns true — WO4)
 *   - resetConfig():    iConfigurable stub (no-op — WO4)
 */

#pragma once

#include <vector>
#include <cstdint>
#include "interfaces/ITickable.hpp"
#include "interfaces/IStatusReporter.hpp"
#include "interfaces/iConfigurable.hpp"
#include "interfaces/IDisplayBoard.hpp"
#include "boards/bootSplashBoard.hpp"
#include "boards/loadingBoard.hpp"
#include "boards/wifiWizardBoard.hpp"
#include "boards/otaProgressBoard.hpp"
#include "boards/errorBoard.hpp"
#include "boards/screensaverBoard.hpp"

// --- Hardware-only includes (excluded from native GoogleTest builds) ---
#ifndef NATIVE_BUILD
#include <U8g2lib.h>
#endif

// --- Forward declarations (no header dependency in this WO) ---
class EventBus;
class Application;

// ---------------------------------------------------------------------------
// Default U8G2 driver type — apps override this via build_flags
// e.g. -DU8G2_DISPLAY_TYPE=U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI
// ---------------------------------------------------------------------------
#ifndef NATIVE_BUILD
#ifndef U8G2_DISPLAY_TYPE
#define U8G2_DISPLAY_TYPE U8G2_SSD1306_128X64_NONAME_F_HW_I2C
#endif
#endif

/**
 * @brief U8G2 monochrome display module with carousel engine and board slot pool.
 *
 * Implements ITickable for main-loop rendering, IStatusReporter for boot-phase
 * status overlays (stubbed in this WO), and iConfigurable for persistent
 * settings (stubbed in this WO).
 *
 * The board slot pool is a std::vector<IDisplayBoard*> that accepts registrations
 * via addBoard() before begin() locks the pool. The carousel engine rotates
 * through registered boards at a configurable interval.
 */
class U8g2DisplayModule : public ITickable, public IStatusReporter, public iConfigurable {
public:
    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /**
     * @brief Initialize hardware, apply flip/brightness, and lock the board pool.
     *
     * After this call, addBoard() will reject new registrations. The U8G2
     * driver is started, contrast and flip mode are applied, and the inactivity
     * timer is reset.
     *
     * @param app Reference to the Application composition root (cached for WO4).
     */
    void begin(Application& app);

    /**
     * @brief Periodic update hook — carousel rotation, sleep check, render.
     *
     * Called by the Application tick loop on every iteration. Checks the sleep
     * timer, advances the carousel if the rotation interval has elapsed, and
     * renders the current board into the U8G2 display buffer.
     */
    void tick() override;

    // -------------------------------------------------------------------------
    // Board Pool Management
    // -------------------------------------------------------------------------

    /**
     * @brief Register a display board into the carousel pool (pre-lock only).
     *
     * Must be called before begin(). After begin() locks the pool, this method
     * logs an error and returns false.
     *
     * @param board Non-null pointer to the IDisplayBoard implementation.
     * @return true if the board was added; false if locked or nullptr.
     */
    bool addBoard(IDisplayBoard* board);

    /**
     * @brief Force-show a specific board, bypassing the carousel timer.
     *
     * If the board is found in the pool, the active index is updated and the
     * rotation timer is reset. If the board is NOT in the pool (e.g. a system
     * board), it is rendered directly via a special override index of -1.
     *
     * @param board Pointer to the board to display.
     */
    void showBoard(IDisplayBoard* board);

    /**
     * @brief Reset the inactivity timer (called on user interaction events).
     *
     * If the display was sleeping, this wakes it up and restores brightness.
     */
    void resetActivity();

    /**
     * @brief Return the number of boards currently in the carousel pool.
     * @return Pool size as int.
     */
    int getBoardCount() const;

    /**
     * @brief Return the board currently being rendered (or nullptr if empty).
     * @return Pointer to the active IDisplayBoard, or nullptr.
     */
    IDisplayBoard* getActiveBoard() const;

    // -------------------------------------------------------------------------
    // System board accessors
    // -------------------------------------------------------------------------

    /**
     * @brief Return a reference to the owned BootSplashBoard instance.
     * @return Mutable reference to _bootSplash.
     */
    BootSplashBoard& getBootSplash();

    /**
     * @brief Return a reference to the owned LoadingBoard instance.
     * @return Mutable reference to _loading.
     */
    LoadingBoard& getLoading();

    /**
     * @brief Return a reference to the owned WiFiWizardBoard instance.
     * @return Mutable reference to _wifiWizard.
     */
    WiFiWizardBoard& getWifiWizard();

    /**
     * @brief Return a reference to the owned OtaProgressBoard instance.
     * @return Mutable reference to _otaProgress.
     */
    OtaProgressBoard& getOtaProgress();

    /**
     * @brief Return a reference to the owned ErrorBoard instance.
     * @return Mutable reference to _error.
     */
    ErrorBoard& getError();

    /**
     * @brief Return a reference to the owned ScreensaverBoard instance.
     * @return Mutable reference to _screensaver.
     */
    ScreensaverBoard& getScreensaver();

    // -------------------------------------------------------------------------
    // IStatusReporter stubs (empty body — implementation deferred to WO2/3)
    // -------------------------------------------------------------------------

    /** @brief Stub — no-op in this WO. */
    void reportStatus(const char* message, float progress = -1.0f) override;

    /** @brief Stub — no-op in this WO. */
    void reportError(const char* errorMessage) override;

    /** @brief Stub — no-op in this WO. */
    void clearStatus() override;

    // -------------------------------------------------------------------------
    // iConfigurable stubs (return defaults — implementation deferred to WO4)
    // -------------------------------------------------------------------------

    /** @brief Stub — returns true (no config to load yet). */
    bool loadConfig() override;

    /** @brief Stub — returns true (no config to persist yet). */
    bool saveConfig() override;

    /** @brief Stub — no-op (no config to reset yet). */
    void resetConfig() override;

private:
    // -------------------------------------------------------------------------
    // Private carousel engine methods
    // -------------------------------------------------------------------------

    /**
     * @brief Advance the carousel to the next non-expired board.
     *
     * Removes any expired boards (isExpired() == true) from the vector,
     * then advances _activeBoardIndex to the next valid entry (wrapping).
     */
    void _advanceCarousel();

    /**
     * @brief Render the current board into the U8G2 display buffer.
     *
     * Calls clearBuffer() → board->render(u8g2) → sendBuffer() inside
     * a NATIVE_BUILD guard. No-op on the native test runner.
     */
    void _renderCurrentBoard();

    /**
     * @brief Check the inactivity timer and enter sleep if timed out.
     *
     * Compares millis() against _lastActivityMs + _sleepTimeoutMs.
     * If the timeout is 0, sleep is disabled.
     */
    void _checkSleep();

    /**
     * @brief Exit sleep mode and restore display brightness.
     */
    void _wake();

    /**
     * @brief Enter sleep mode and power-save the display.
     */
    void _sleep();

    // -------------------------------------------------------------------------
    // Member variables
    // -------------------------------------------------------------------------

    // --- Hardware driver (excluded from native builds) ---
#ifndef NATIVE_BUILD
    U8G2_DISPLAY_TYPE _u8g2;                      ///< The U8G2 hardware driver instance
#endif

    Application* _app = nullptr;                   ///< Cached reference to Application hub (for WO4 EventBus access)

    // --- Board slot pool ---
    std::vector<IDisplayBoard*> _boards;           ///< Carousel board pool — populated before begin()
    int _activeBoardIndex = 0;                     ///< Index of the currently rendering board in _boards

    // --- Carousel timing ---
    unsigned long _lastRotationMs = 0;             ///< Timestamp of last carousel rotation
    unsigned long _rotationIntervalMs = 8000;      ///< Carousel rotation interval (8 seconds default)

    // --- Pool lock ---
    bool _locked = false;                          ///< True after begin() — blocks addBoard()

    // --- Sleep / power management ---
    bool _sleeping = false;                        ///< True when the display is in power-save sleep
    unsigned long _lastActivityMs = 0;             ///< Timestamp of last user interaction
    unsigned long _sleepTimeoutMs = 300000;        ///< Inactivity timeout before sleep (5 minutes default)

    // --- Display settings ---
    uint8_t _brightness = 255;                     ///< Display brightness / contrast level (0–255)
    bool _flipScreen = false;                      ///< True if the display is flipped 180 degrees

    // --- System board override pointer ---
    IDisplayBoard* _overrideBoard = nullptr;       ///< Non-pool board shown via showBoard() when not in pool

    // --- Owned system board instances ---
    BootSplashBoard  _bootSplash;                  ///< Boot splash screen (BOOTING phase)
    LoadingBoard     _loading;                     ///< Loading progress screen (APP_SETUP phase)
    WiFiWizardBoard  _wifiWizard;                  ///< WiFi setup wizard screen (WIFI_SETUP phase)
    OtaProgressBoard _otaProgress;                 ///< OTA firmware update progress screen
    ErrorBoard       _error;                       ///< Error message display screen
    ScreensaverBoard _screensaver;                 ///< Inactivity screensaver (burn-in prevention)
};
