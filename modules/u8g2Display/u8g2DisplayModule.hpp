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
 *              Error, Screensaver, Configuring) can bypass the carousel via showBoard().
 *              Owns all system board instances and activates the splash on begin().
 *
 *              Implements IStatusReporter by delegating to BootBoard and
 *              ErrorBoard. Subscribes to EventBus topics "ota.progress" and
 *              "display.message" for runtime event integration. Publishes
 *              "display.sleep", "display.wake", and "display.board.changed"
 *              events via EventBus. Implements iConfigurable via ConfigManager
 *              gateway for persistent display settings (namespace: "display",
 *              schema version: 1). Implements IWebContributor for REST API
 *              routes under /api/display/ and a top-level "Display" tab.
 *
 * Exported Classes:
 * - OtaProgressPayload:            EventBus payload struct for OTA progress events
 * - DisplayMessagePayload:         EventBus payload struct for display message events
 * - DisplayBoardChangedPayload:    EventBus payload struct for board change events
 * - U8g2DisplayModule: Display carousel engine and board slot pool
 *   - bootstrapHardware():   Phase 1 init — SPI + U8G2 only, zero framework deps
 *   - begin():              Phase 2 init — EventBus, config, pool lock (skips HW if bootstrapped)
 *   - tick():               Carousel rotation, sleep check, board tick, render
 *   - addBoard():           Pre-lock registration of boards into the carousel pool
 *   - showBoard():          Force-show a specific board (bypasses carousel timer)
 *   - resetActivity():      Reset the inactivity timer (called on user interaction)
 *   - getBoardCount():      Return the number of registered boards in the pool
 *   - getActiveBoard():     Return the currently rendering board (or nullptr)
 *   - getBootBoard():       Return reference to the owned BootBoard instance
 *   - getWifiWizard():      Return reference to the owned WiFiWizardBoard instance
 *   - getOtaProgress():     Return reference to the owned OtaProgressBoard instance
 *   - getError():           Return reference to the owned ErrorBoard instance
 *   - getConfiguringStatus(): Return reference to the owned ConfiguringStatusBoard instance
 *   - getScreensaver():     Return reference to the owned ScreensaverBoard instance
 *   - reportStatus():       Delegate to BootBoard and show it
 *   - reportError():        Delegate to ErrorBoard (fatal) and show it
 *   - clearStatus():        Mark BootBoard complete, advance if showing it
 *   - registerRoutes():     Register display REST API routes (/api/display/*)
 *   - getTabName():         Return "Display" for portal tab
 *   - getTabOrder():        Return 30 for portal tab ordering
 *   - getConfigNamespace(): Return "display"
 *   - getSchemaVersion():   Return 1
 *   - applyConfig():        Apply display settings from JSON section
 *   - serializeConfig():    Serialize display settings to JSON section
 *   - resetConfig():        Restore factory defaults
 *   - setBrightness():      Runtime brightness adjustment (0–255)
 *   - setFlipScreen():      Runtime screen flip toggle
 *   - setSleepTimeout():    Runtime sleep timeout adjustment
 *   - setRotationInterval(): Runtime carousel rotation interval adjustment
 */

#pragma once

#include <vector>
#include <cstdint>
#include "interfaces/ITickable.hpp"
#include "interfaces/IStatusReporter.hpp"
#include "interfaces/iConfigurable.hpp"
#include "interfaces/IWebContributor.hpp"
#include "interfaces/IDisplayBoard.hpp"
#include "boards/bootBoard.hpp"
#include "boards/wifiWizardBoard.hpp"
#include "boards/otaProgressBoard.hpp"
#include "boards/errorBoard.hpp"
#include "boards/screensaverBoard.hpp"
#include "boards/configuringStatusBoard.hpp"
#include "events/AppConfiguringPayload.hpp"

// --- Hardware-only includes (excluded from native GoogleTest builds) ---
#ifndef NATIVE_BUILD
#include <U8g2lib.h>
#include <ArduinoJson.h>
#endif

// --- Forward declarations (no header dependency in this WO) ---
class EventBus;
class Application;

// ---------------------------------------------------------------------------
// EventBus Payload Structs
// ---------------------------------------------------------------------------

/**
 * @brief EventBus payload for "ota.progress" topic.
 *
 * Published by the OTA manager during firmware update cycles. The display
 * module subscribes and delegates to OtaProgressBoard.
 */
struct OtaProgressPayload {
    float progress;       // 0.0 to 1.0 download fraction
    const char* status;   // Status message (e.g. "Downloading...")
    bool complete;        // True when OTA cycle is finished
    bool success;         // True if OTA succeeded (only valid when complete)
};

/**
 * @brief EventBus payload for "display.message" topic.
 *
 * Published by any module that wants to temporarily display a message
 * on the loading screen overlay.
 */
struct DisplayMessagePayload {
    const char* message;       // Text to display
    unsigned long durationMs;  // How long to display (0 = until cleared)
};

/**
 * @brief EventBus payload for "display.board.changed" topic.
 *
 * Published by the display module when the active rendering board changes,
 * either via carousel rotation or explicit showBoard() call.
 */
struct DisplayBoardChangedPayload {
    const char* boardName;  // Name of the newly active board
    int boardIndex;         // Pool index, or -1 for override (system) boards
};

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
 * status overlays (delegates to BootBoard/ErrorBoard), and iConfigurable
 * for persistent display settings via ConfigManager gateway (namespace: "display").
 *
 * The board slot pool is a std::vector<IDisplayBoard*> that accepts registrations
 * via addBoard() before begin() locks the pool. The carousel engine rotates
 * through registered boards at a configurable interval.
 *
 * Subscribes to EventBus topics "ota.progress" and "display.message" for
 * runtime integration with other framework modules.
 */
class U8g2DisplayModule : public ITickable, public IStatusReporter, public iConfigurable, public IWebContributor {
public:
    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /**
     * @brief Phase 1: Bootstrap display hardware with zero framework dependencies.
     *
     * Initializes the SPI bus and U8G2 driver, applies default contrast and
     * flip mode, activates the boot splash, and renders it immediately.
     * Enters bootstrap mode where reportStatus() performs inline blocking
     * animations instead of waiting for the tick loop.
     *
     * Call this BEFORE app.begin() so the display is ready to show boot progress.
     * Call begin(app) AFTER app.begin() to complete framework integration.
     */
    void bootstrapHardware();

    /**
     * @brief Phase 2: Complete framework integration.
     *
     * Subscribes to EventBus topics, locks the board pool, and exits
     * bootstrap mode. If bootstrapHardware() was called first, skips
     * hardware init (SPI + U8G2 already running). Otherwise performs
     * full hardware + framework init in one step.
     *
     * @param app Reference to the Application composition root.
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
     * @brief Return a reference to the owned BootBoard instance.
     * @return Mutable reference to _bootBoard.
     */
    BootBoard& getBootBoard();

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

    /**
     * @brief Return a reference to the owned ConfiguringStatusBoard instance.
     * @return Mutable reference to _configuringStatusBoard.
     */
    ConfiguringStatusBoard& getConfiguringStatus();

    // -------------------------------------------------------------------------
    // IStatusReporter — delegates to BootBoard and ErrorBoard
    // -------------------------------------------------------------------------

    /**
     * @brief Report a status message with optional progress.
     *
     * Delegates to BootBoard::setStatus(), shows the boot board if not
     * already active, and resets the inactivity timer.
     *
     * @param message  Null-terminated status text.
     * @param progress Fractional progress [0.0, 1.0]; negative = indeterminate.
     */
    void reportStatus(const char* message, float progress = -1.0f) override;

    /**
     * @brief Report a fatal error message.
     *
     * Delegates to ErrorBoard::showError() with fatal=true, shows the error
     * board, and resets the inactivity timer.
     *
     * @param errorMessage Null-terminated error description.
     */
    void reportError(const char* errorMessage) override;

    /**
     * @brief Clear the status overlay.
     *
     * Marks BootBoard as complete. If the boot board is currently
     * showing, advances the carousel to the next board.
     */
    void clearStatus() override;

    // -------------------------------------------------------------------------
    // iConfigurable — ConfigManager gateway (namespace: "display")
    // -------------------------------------------------------------------------

    /**
     * @brief Return the config namespace for display settings.
     * @return "display"
     */
    const char* getConfigNamespace() const override { return "display"; }

    /**
     * @brief Return the current schema version for display config.
     * @return 1
     */
    int getSchemaVersion() const override { return 1; }

    /**
     * @brief Apply display settings from a parsed JSON section.
     *
     * Reads brightness, flip, sleepTimeout, and rotationInterval from the
     * JSON object and applies them to internal state and hardware.
     *
     * @param section Read-only JSON object containing display config keys.
     */
    void applyConfig(const JsonObject& section) override;

    /**
     * @brief Serialize current display settings into a JSON section.
     *
     * Writes brightness, flip, sleepTimeout, and rotationInterval to the
     * JSON object.
     *
     * @param section Mutable JSON object to write config into.
     */
    void serializeConfig(JsonObject& section) const override;

    /**
     * @brief Reset display settings to factory defaults.
     *
     * Sets: brightness=255, flipScreen=false, sleepTimeout=300000ms,
     * rotationInterval=8000ms. Applies hardware settings.
     */
    void resetConfig() override;

    // -------------------------------------------------------------------------
    // IWebContributor — Display REST API and portal tab
    // -------------------------------------------------------------------------

    /**
     * @brief Register display REST API routes with the web server.
     *
     * Registers endpoints under /api/display/ for status, brightness,
     * flip, sleep, board listing, and board selection.
     *
     * @param server Reference to the AsyncWebServer instance.
     */
    void registerRoutes(AsyncWebServer& server) override;

    /**
     * @brief Get the UI tab name for this contributor.
     * @return "Display"
     */
    const char* getTabName() const override { return "Display"; }

    /**
     * @brief Get the tab display order.
     * @return 30 — Display tab appears before mid-range tabs.
     */
    int getTabOrder() const override { return 30; }

    // -------------------------------------------------------------------------
    // Runtime display configuration setters
    // -------------------------------------------------------------------------

    /**
     * @brief Set the display brightness and apply to hardware.
     * @param brightness Brightness level (0–255).
     */
    void setBrightness(uint8_t brightness);

    /**
     * @brief Set the screen flip mode and apply to hardware.
     * @param flip True to flip the display 180 degrees.
     */
    void setFlipScreen(bool flip);

    /**
     * @brief Set the inactivity sleep timeout.
     * @param timeoutMs Timeout in milliseconds (0 = sleep disabled).
     */
    void setSleepTimeout(unsigned long timeoutMs);

    /**
     * @brief Set the carousel rotation interval.
     * @param intervalMs Interval in milliseconds between board rotations.
     */
    void setRotationInterval(unsigned long intervalMs);

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
    // Private EventBus callback statics
    // -------------------------------------------------------------------------

    /**
     * @brief Static callback for "ota.progress" EventBus topic.
     * @param topic The topic string ("ota.progress").
     * @param payload Pointer to an OtaProgressPayload struct.
     * @param ctx Context pointer (this U8g2DisplayModule instance).
     */
    static void _onOtaProgress(const char* topic, const void* payload, void* ctx);

    /**
     * @brief Static callback for "display.message" EventBus topic.
     * @param topic The topic string ("display.message").
     * @param payload Pointer to a DisplayMessagePayload struct.
     * @param ctx Context pointer (this U8g2DisplayModule instance).
     */
    static void _onDisplayMessage(const char* topic, const void* payload, void* ctx);

    /**
     * @brief Static callback for "app.configuring" EventBus topic.
     * @param topic The topic string ("app.configuring").
     * @param payload Pointer to an AppConfiguringPayload struct.
     * @param ctx Context pointer (this U8g2DisplayModule instance).
     */
    static void _onAppConfiguring(const char* topic, const void* payload, void* ctx);

    /**
     * @brief Static callback for "app.running" EventBus topic.
     * @param topic The topic string ("app.running").
     * @param payload Unused (nullptr).
     * @param ctx Context pointer (this U8g2DisplayModule instance).
     */
    static void _onAppRunning(const char* topic, const void* payload, void* ctx);

    // -------------------------------------------------------------------------
    // Member variables
    // -------------------------------------------------------------------------

    // --- Hardware driver (excluded from native builds) ---
    // Pin configuration is injected via build_flags in platformio.ini.
    // SPI displays: define DISPLAY_CS_PIN, DISPLAY_DC_PIN (required),
    //               DISPLAY_RST_PIN (optional, defaults to U8X8_PIN_NONE).
    // I2C displays: no pin defines needed (U8G2 uses Arduino defaults).
#ifndef NATIVE_BUILD
  #ifdef DISPLAY_CS_PIN
    #ifndef DISPLAY_RST_PIN
    #define DISPLAY_RST_PIN U8X8_PIN_NONE
    #endif
    U8G2_DISPLAY_TYPE _u8g2{U8G2_R0, DISPLAY_CS_PIN, DISPLAY_DC_PIN, DISPLAY_RST_PIN};
  #else
    U8G2_DISPLAY_TYPE _u8g2{U8G2_R0};
  #endif

    // --- Display geometry pseudo-constants ---
    // Read once from the U8G2 driver descriptor at member-init time.
    // U8G2 populates tile dimensions in its constructor (before begin()),
    // and _u8g2 is declared above so it is fully constructed first.
    const uint16_t _displayWidth  = _u8g2.getDisplayWidth();
    const uint16_t _displayHeight = _u8g2.getDisplayHeight();
#else
    // --- Native build defaults (no U8G2 hardware driver) ---
    // Match the primary hardware target (SSD1322 256×64) for test consistency.
    const uint16_t _displayWidth  = 256;
    const uint16_t _displayHeight = 64;
#endif

    Application* _app = nullptr;                   ///< Cached reference to Application hub (for EventBus access)

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

    // --- Bootstrap state ---
    bool  _bootstrapMode    = false;               ///< True between bootstrapHardware() and begin()
    float _lastBootProgress = 0.0f;                ///< Cached progress for smooth inline interpolation

    // --- Owned system board instances ---
    BootBoard        _bootBoard{_displayWidth, _displayHeight};   ///< Unified boot board (splash + loading phases)
    WiFiWizardBoard  _wifiWizard{_displayWidth, _displayHeight};   ///< WiFi setup wizard screen (CONFIGURING phase)
    OtaProgressBoard _otaProgress{_displayWidth, _displayHeight};  ///< OTA firmware update progress screen
    ErrorBoard       _error{_displayWidth, _displayHeight};        ///< Error message display screen
    ScreensaverBoard _screensaver{_displayWidth, _displayHeight};  ///< Inactivity screensaver (burn-in prevention)
    ConfiguringStatusBoard _configuringStatusBoard{_displayWidth, _displayHeight}; ///< Gate progress during CONFIGURING phase
};
