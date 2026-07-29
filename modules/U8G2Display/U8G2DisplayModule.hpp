/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/U8G2Display/U8G2DisplayModule.hpp
 * Description: Pure-service U8G2 monochrome display module with three-layer
 *              rendering architecture (Modal → Base/Carousel → Screensaver).
 *              Accepts commands from callers; does NOT poll Application state
 *              or make lifecycle decisions (see design spec §7.8.5, D34).
 *
 *              Exposes a hierarchical COM-style sub-object API (D35):
 *                display.carousel    — CarouselEngine (rotating board pool)
 *                display.boards      — BoardCatalog (concrete board refs)
 *                display.screensaver — ScreensaverController (inactivity policy)
 *
 *              Implements IStatusReporter by delegating to BootBoard and
 *              ErrorBoard. Subscribes to EventBus topics "ota.progress" and
 *              "display.message" for async runtime events. Publishes
 *              "display.sleep", "display.wake", and "display.board.changed"
 *              events via EventBus. Implements IConfigurable via ConfigManager
 *              gateway for persistent display settings (namespace: "display",
 *              schema version: 3). Implements IWebContributor for REST API
 *              routes under /api/display/ and portal tab "App" (order 30).
 *              Implements IMCPContributor for display_status (read-safe)
 *              and display_control (mutating) MCP tools.
 *
 * Exported Classes:
 * - OTAProgressPayload:            EventBus payload struct for OTA progress events
 * - DisplayMessagePayload:         EventBus payload struct for display message events
 * - DisplayBoardChangedPayload:    EventBus payload struct for board change events
 * - U8G2DisplayModule: Display carousel engine and board slot pool
 *
 *   Lifecycle:
 *   - bootstrapHardware():   Phase 1 init — SPI + U8G2 only, zero framework deps
 *   - begin():              Phase 2 init — EventBus, config, pool lock (skips HW if bootstrapped)
 *   - tick():               Screensaver, modal expiry, carousel, render  [ITickable]
 *
 *   Layer Management:
 *   - showModal():          Push a modal overlay board (auto-clears on expiry)
 *   - showBase():           Show a persistent base layer board
 *   - clearModal():         Manually clear the active modal overlay
 *   - resetActivity():      Reset the inactivity timer (called on user interaction)
 *   - visibleBoard():       Return the board currently being rendered
 *
 *   IStatusReporter:
 *   - reportStatus():       Delegate to BootBoard and show it
 *   - reportError():        Delegate to ErrorBoard (fatal) and show it
 *   - clearStatus():        Mark BootBoard complete, advance if showing it
 *
 *   IConfigurable (namespace: "display", schema v3):
 *   - getConfigNamespace(): Return "display"
 *   - getSchemaVersion():   Return 3
 *   - applyConfig():        Apply display settings from JSON section
 *   - serializeConfig():    Serialize display settings to JSON section
 *   - resetConfig():        Restore factory defaults
 *
 *   IWebContributor (tab: "App", order: 30):
 *   - registerRoutes():     Register display REST API routes (/api/display/*)
 *   - getTabName():         Return "App" for portal tab
 *   - getTabOrder():        Return 50 for portal tab ordering
 *
 *   IMCPContributor:
 *   - registerMCPTools():   Register display_status/display_control MCP tools
 *
 *   Runtime Display Setters:
 *   - setBrightness():      Runtime brightness adjustment (0–255)
 *   - setFlipScreen():      Runtime screen flip toggle
 *   - setSleepTimeout():    Runtime sleep timeout adjustment
 *   - setRotationInterval(): Runtime carousel rotation interval adjustment
 *   - setTargetFps():        Runtime frame-rate target adjustment (default 30 FPS)
 */

#pragma once

#include <vector>
#include <cstdint>
#include "interfaces/ITickable.hpp"
#include "interfaces/IStatusReporter.hpp"
#include "interfaces/IConfigurable.hpp"
#include "interfaces/IWebContributor.hpp"
#include "interfaces/IMCPContributor.hpp"
#include "interfaces/IDisplayBoard.hpp"
#include "boards/BootBoard.hpp"
#include "CarouselEngine.hpp"
#include "ScreensaverController.hpp"
#include "BoardCatalog.hpp"
#include "boards/WiFiWizardBoard.hpp"
#include "boards/OTAProgressBoard.hpp"
#include "boards/ErrorBoard.hpp"
#include "boards/ScreensaverBoard.hpp"
#include "boards/ConfiguringStatusBoard.hpp"
#include "../../include/types/DisplayInfo.hpp"

// --- Hardware-only includes (excluded from native GoogleTest builds) ---
#include <U8g2lib.h>
#include <ArduinoJson.h>

// --- Forward declarations ---
class EventBus;
class ConfigManager;
class MCPToolRegistry;

// ---------------------------------------------------------------------------
// EventBus Payload Structs
// ---------------------------------------------------------------------------

/**
 * @brief EventBus payload for "ota.progress" topic.
 *
 * Published by the OTA manager during firmware update cycles. The display
 * module subscribes and delegates to OTAProgressBoard.
 */
struct OTAProgressPayload {
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
 * either via carousel rotation, showModal()/clearModal(), or showBase().
 */
struct DisplayBoardChangedPayload {
    const char* boardName;  // Name of the newly active board
    int boardIndex;         // Pool index, or -1 for modal/system boards
};

// ---------------------------------------------------------------------------
// Display Events
// ---------------------------------------------------------------------------

namespace U8G2DisplayEvents {
    /// @brief Published when display enters sleep mode.
    /// Payload: nullptr
    constexpr const char* DISPLAY_SLEEP = "display.sleep";

    /// @brief Published when display wakes up.
    /// Payload: nullptr
    constexpr const char* DISPLAY_WAKE = "display.wake";

    /// @brief Published when active screen/board changes.
    /// Payload: const DisplayBoardChangedPayload* (or nullptr)
    constexpr const char* DISPLAY_BOARD_CHANGED = "display.board.changed";

    /// @brief Subscribed to by the display to show scroll messages.
    /// Payload: const DisplayMessagePayload*
    constexpr const char* DISPLAY_MESSAGE = "display.message";
}

// ---------------------------------------------------------------------------
// Default U8G2 driver type — apps override this via build_flags
// e.g. -DU8G2_DISPLAY_TYPE=U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI
// ---------------------------------------------------------------------------
#ifndef U8G2_DISPLAY_TYPE
#define U8G2_DISPLAY_TYPE U8G2_SSD1306_128X64_NONAME_F_HW_I2C
#endif

/**
 * @brief U8G2 monochrome display module with carousel engine and board slot pool.
 *
 * Implements ITickable for main-loop rendering, IStatusReporter for boot-phase
 * status overlays (delegates to BootBoard/ErrorBoard), IConfigurable
 * for persistent display settings via ConfigManager gateway (namespace: "display"),
 * and IMCPContributor for display_status / display_control MCP tools.
 *
 * The carousel pool accepts registrations via display.carousel.add() and
 * supports runtime mutation (add/remove/clear). The carousel engine rotates
 * through registered boards at a configurable interval.
 *
 * Subscribes to EventBus topics "ota.progress" and "display.message" for
 * runtime integration with other framework modules.
 */
class U8G2DisplayModule : public ITickable, public IStatusReporter, public IConfigurable, public IWebContributor, public IMCPContributor {
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
     * @param eventBus Pointer to the EventBus instance for messaging.
     */
    void begin(EventBus* eventBus);

    /**
     * @brief Periodic update hook — carousel rotation, sleep check, render.
     *
     * Called by the Application tick loop on every iteration. Checks the sleep
     * timer, advances the carousel if the rotation interval has elapsed, and
     * renders the current board into the U8G2 display buffer.
     */
    void tick(uint32_t currentMillis) override;

    // -------------------------------------------------------------------------
    // Layer Management
    // -------------------------------------------------------------------------

    /**
     * @brief Set the base persistent board (e.g., configuring status).
     *
     * @param board Pointer to the base board, or nullptr to revert to carousel.
     */
    void showBase(IDisplayBoard* board);

    /**
     * @brief Push a modal overlay board onto the display.
     *
     * The modal board is rendered on top of the base layer until it is
     * manually cleared via clearModal() or auto-cleared when isExpired()
     * returns true. Deactivates any existing modal, activates the new one.
     *
     * @param board Pointer to the modal board to display.
     */
    void showModal(IDisplayBoard* board);

    /**
     * @brief Manually clear the active modal overlay.
     *
     * Deactivates the current modal board, restores display brightness to
     * the configured level, and returns rendering to the phase-determined
     * base layer.
     */
    void clearModal();

    /**
     * @brief Reset the inactivity timer (called on user interaction events).
     *
     * If the display was sleeping, this wakes it up and restores brightness.
     */
    void resetActivity();

    /**
     * @brief Return the board currently being rendered, considering all layers.
     *
     * Priority: modal → base → screensaver (idle) → carousel.
     * Returns nullptr if no board is active on any layer.
     *
     * @return Pointer to the visible IDisplayBoard, or nullptr.
     */
    IDisplayBoard* visibleBoard() const;

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
    // IConfigurable — ConfigManager gateway (namespace: "display")
    // -------------------------------------------------------------------------

    /**
     * @brief Return the config namespace for display settings.
     * @return "display"
     */
    const char* getConfigNamespace() const override { return "display"; }

    /**
     * @brief Return the current schema version for display config.
     * @return 3
     */
    int getSchemaVersion() const override { return 3; }

    /**
     * @brief Apply display settings from a parsed JSON section.
     *
     * Reads brightness, flip, screensaverEnabled, dimTimeout, blankTimeout,
     * rotationInterval, and targetFps from the JSON object and applies them
     * to internal state and hardware. Migrates legacy sleepTimeout config.
     *
     * @param section Read-only JSON object containing display config keys.
     */
    void applyConfig(const JsonObject& section) override;

    /**
     * @brief Serialize current display settings into a JSON section.
     *
     * Writes brightness, flip, screensaverEnabled, dimTimeout, blankTimeout,
     * rotationInterval, targetFps, and clockFontSize to the JSON object.
     *
     * @param section Mutable JSON object to write config into.
     */
    void serializeConfig(JsonObject& section) const override;

    /**
     * @brief Reset display settings to factory defaults.
     *
     * Sets: brightness=255, flipScreen=false, screensaverEnabled=true,
     * dimTimeout=150000ms, blankTimeout=300000ms, rotationInterval=8000ms.
     * Applies hardware settings.
     */
    void resetConfig() override;

    // -------------------------------------------------------------------------
    // IWebContributor — Display REST API and portal tab
    // -------------------------------------------------------------------------

    /**
     * @brief Register the module's REST API routes with the web server.
     * @param server Reference to the PsychicHttpServer instance.
     */
    void registerRoutes(PsychicHttpServer& server) override;

    /**
     * @brief Get the UI tab name for this contributor.
     * @return "Display"
     */
    const char* getTabName() const override { return "Display"; }

    /**
     * @brief Get the tab display order.
     * @return 30 — Display tab appears before mid-range tabs.
     */
    int getTabOrder() const override { return 50; }

    /**
     * @brief Return the API path prefix for body-size middleware matching.
     * @return "/api/display"
     */
    const char* getApiPrefix() const override { return "/api/display"; }

    /**
     * @brief Return SVG inner content for the Display bottom-nav icon.
     * @return Feather-style "monitor" icon SVG body string.
     */
    const char* getTabIcon() const override;

    /**
     * @brief Return the Settings sub-pill stub for registration by Application.
     * @return Pointer to the Settings IWebContributor stub.
     */
    IWebContributor* settingsTabStub()   { return &_settingsTabStub;   }

    /**
     * @brief Return the Screenshot sub-pill stub for registration by Application.
     * @return Pointer to the Screenshot IWebContributor stub.
     */
    IWebContributor* screenshotTabStub() { return &_screenshotTabStub; }

    // -------------------------------------------------------------------------
    // IMCPContributor — MCP display tools
    // -------------------------------------------------------------------------

    /**
     * @brief Register display_status and display_control MCP tools.
     *
     * display_status (read-safe) returns brightness, flip, active board,
     * screensaver state, and carousel info. display_control (mutating)
     * supports wake/sleep, brightness, screensaver toggle, and timeouts.
     *
     * @param registry The MCPToolRegistry to register tools with.
     */
    void registerMCPTools(MCPToolRegistry& registry) override;

    // -------------------------------------------------------------------------
    // Runtime display configuration setters
    // -------------------------------------------------------------------------

    /**
     * @brief Set the ConfigManager reference for saving config changes.
     * @param cm Pointer to the ConfigManager.
     */
    void setConfigManager(ConfigManager* cm) { _configManager = cm; }

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

    /**
     * @brief Set the target render frame rate.
     *
     * The display will skip rendering if fewer than (1000/fps) milliseconds
     * have elapsed since the last sendBuffer(). This reduces SPI bus load
     * and prevents IDLE-task starvation on the ESP32.
     *
     * @param fps Target frames per second (1–60). 0 treated as 30.
     */
    void setTargetFps(uint8_t fps);

    /**
     * @brief Set the clock font size.
     * @param size Font size level (1=small, 2=medium, 3=large). Clamped to range.
     */
    void setClockFontSize(uint8_t size);

    /**
     * @brief Get the current display brightness level.
     * @return Brightness level (0–255).
     */
    uint8_t getBrightness() const { return _brightness; }

    /**
     * @brief Get the current screen flip state.
     * @return True if the display is flipped 180 degrees.
     */
    bool getFlipScreen() const { return _flipScreen; }

    /**
     * @brief Get the current clock font size.
     * @return Font size level (1=small, 2=medium, 3=large).
     */
    uint8_t getClockFontSize() const;

    /**
     * @brief Get the display info struct for MCP capture_display wiring.
     *
     * Returns a populated DisplayInfo with the U8G2 framebuffer pointer,
     * size, and dimensions. Only valid after bootstrapHardware() has been
     * called (the buffer is allocated during U8G2 begin()).
     *
     * @return DisplayInfo struct with live framebuffer pointer.
     */
    DisplayInfo getDisplayInfo();

private:
    // -------------------------------------------------------------------------
    // Sub-tab navigation stubs — provide "Settings" and "Screenshot" child tab
    // entries under the "Display" parent in /api/tabs. They carry no routes;
    // all REST routes remain registered by the main module via registerRoutes().
    // Owned as members; exposed via settingsTabStub() / screenshotTabStub()
    // for registration by Application::begin().
    // -------------------------------------------------------------------------

    struct SettingsTabStub : public IWebContributor {
        void        registerRoutes(PsychicHttpServer& /*server*/) override {}
        const char* getTabName()   const override { return "Settings"; }
        const char* getParentTab() const override { return "Display";  }
        int         getTabOrder()  const override { return 10;         }
    };

    struct ScreenshotTabStub : public IWebContributor {
        void        registerRoutes(PsychicHttpServer& /*server*/) override {}
        const char* getTabName()   const override { return "Screenshot"; }
        const char* getParentTab() const override { return "Display";    }
        int         getTabOrder()  const override { return 20;           }
    };

    SettingsTabStub    _settingsTabStub;    ///< Nav stub: Settings sub-pill under Display
    ScreenshotTabStub  _screenshotTabStub;  ///< Nav stub: Screenshot sub-pill under Display

    // -------------------------------------------------------------------------
    // Private carousel engine methods
    // -------------------------------------------------------------------------

    /**
     * @brief Render the current board into the U8G2 display buffer.
     *
     * Calls clearBuffer() → board->render(u8g2) → sendBuffer() inside
     * a native test stub. No-op on the native test runner.
     */
    void _renderCurrentBoard();

    // -------------------------------------------------------------------------
    // Private EventBus callback statics
    // -------------------------------------------------------------------------

    /**
     * @brief Static callback for "ota.progress" EventBus topic.
     * @param topic The topic string ("ota.progress").
     * @param payload Pointer to an OTAProgressPayload struct.
     * @param ctx Context pointer (this U8G2DisplayModule instance).
     */
    static void _onOtaProgress(const char* topic, const void* payload, void* ctx);

    /**
     * @brief Static callback for "display.message" EventBus topic.
     * @param topic The topic string ("display.message").
     * @param payload Pointer to a DisplayMessagePayload struct.
     * @param ctx Context pointer (this U8G2DisplayModule instance).
     */
    static void _onDisplayMessage(const char* topic, const void* payload, void* ctx);


    // -------------------------------------------------------------------------
    // Member variables
    // -------------------------------------------------------------------------

    // --- Hardware driver (excluded from native builds) ---
    // Pin configuration is injected via build_flags in platformio.ini.
    // SPI displays: define DISPLAY_CS_PIN, DISPLAY_DC_PIN (required),
    //               DISPLAY_RST_PIN (optional, defaults to U8X8_PIN_NONE).
    // I2C displays: no pin defines needed (U8G2 uses Arduino defaults).
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

    EventBus* _eventBus = nullptr;                 ///< Cached reference to EventBus (for publishing events)
    ConfigManager* _configManager = nullptr;       ///< Cached reference to ConfigManager (for saving config)

    // --- Carousel Sub-system ---
    CarouselEngine _carousel;                      ///< Manages the rotating pool of display boards

    // --- Frame-rate limiting ---
    Periodic _renderTimer{33};                     ///< Timer for frame rate limiting
    unsigned long _targetFrameIntervalMs = 33;     ///< Minimum ms between renders (default 30 FPS: 1000/30≈33)

    // --- Sleep / power management ---
    ScreensaverController _screensaver;            ///< Inactivity dim/blank/sleep state machine

    // --- Display settings ---
    uint8_t _brightness = 255;                     ///< Display brightness / contrast level (0–255)
    bool _flipScreen = false;                      ///< True if the display is flipped 180 degrees
    uint8_t _clockFontSize = 2;                    ///< Clock font size (1=small, 2=medium, 3=large)

    // --- Modal layer ---
    IDisplayBoard* _baseBoard = nullptr;           ///< Base board override (if nullptr, carousel is used)
    IDisplayBoard* _modal = nullptr;               ///< Active modal board (auto-clears on expired)

    // --- Bootstrap state ---
    bool  _bootstrapMode    = false;               ///< True between bootstrapHardware() and begin()
    float _lastBootProgress = 0.0f;                ///< Cached progress for smooth inline interpolation

    // --- Owned system board instances ---
    BootBoard        _bootBoard{_displayWidth, _displayHeight};   ///< Unified boot board (splash + loading phases)
    WiFiWizardBoard  _wifiWizard{_displayWidth, _displayHeight};   ///< WiFi setup wizard screen (CONFIGURING phase)
    OTAProgressBoard _otaProgress{_displayWidth, _displayHeight};  ///< OTA firmware update progress screen
    ErrorBoard       _error{_displayWidth, _displayHeight};        ///< Error message display screen
    ScreensaverBoard _screensaverBoard{_displayWidth, _displayHeight};  ///< Inactivity screensaver (burn-in prevention)
    ConfiguringStatusBoard _configuringStatusBoard{_displayWidth, _displayHeight}; ///< Gate progress during CONFIGURING phase

public:
    // -------------------------------------------------------------------------
    // Sub-object API (design spec §7.8.1, D35)
    // -------------------------------------------------------------------------

    /** @brief Carousel sub-object — manages the rotating pool of display boards. */
    CarouselEngine& carousel = _carousel;

    /** @brief Screensaver sub-object — display-intrinsic inactivity dim/blank policy. */
    ScreensaverController& screensaver = _screensaver;

    /** @brief Board catalog sub-object — named, concrete references to user-surfaced boards (§7.8.4). */
    BoardCatalog boards{_bootBoard, _configuringStatusBoard, _wifiWizard, _otaProgress, _error};
};
