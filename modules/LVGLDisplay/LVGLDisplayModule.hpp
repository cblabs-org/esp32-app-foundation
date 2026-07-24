/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/LVGLDisplay/LVGLDisplayModule.hpp
 * Description: Generic LVGL v9 colour-TFT display module core. Manages LVGL
 *              lifecycle, draw buffer allocation, tick updates, input device
 *              focus groups, system screens, and two-stage boot splash.
 *              Implements IConfigurable via ConfigManager gateway (namespace "display")
 *              and IStatusReporter for boot status updates.
 *
 * Exported Classes:
 * - LVGLDisplayModule: Generic LVGL display service module
 *
 *   Lifecycle:
 *   - LVGLDisplayModule(): Constructor taking adapter reference, optional system screens, and boot logo
 *   - ~LVGLDisplayModule(): Destructor freeing allocated draw buffers and handles
 *   - bootstrapHardware(): Phase 1 hardware initialization, buffer setup, and Stage-1 boot splash
 *   - begin():              Phase 2 framework integration and ui_init hook
 *
 *   ITickable:
 *   - tick():               Advances LVGL tick clock and calls lv_timer_handler()
 *
 *   IStatusReporter:
 *   - reportStatus():       Update status message and progress bar on boot screen
 *   - reportError():        Display system error screen
 *   - clearStatus():        Clear status overlay and restore prior screen
 *
 *   Screen Management API:
 *   - showBoot():           Show Stage-1 boot splash / status progress screen
 *   - showError():          Show error screen (prefers ILVGLSystemScreens provider, falls back to built-in)
 *   - showOtaProgress():    Show OTA progress screen (prefers provider, falls back to built-in)
 *   - restorePriorScreen(): Restore screen active before error/OTA display
 *   - setBootLogo():        Set custom Stage-1 boot logo image descriptor
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <ArduinoJson.h>
#include "interfaces/ITickable.hpp"
#include "interfaces/IConfigurable.hpp"
#include "interfaces/IStatusReporter.hpp"
#include "interfaces/ILVGLHardwareAdapter.hpp"
#include "interfaces/ILVGLSystemScreens.hpp"
#include "assets/boot_logo.h"
#include "screens/BootScreen.hpp"
#include "screens/ErrorScreen.hpp"
#include "screens/OTAProgressScreen.hpp"
#include "ScreensaverController.hpp"
#include "lvgl.h"

// Forward declarations
class EventBus;
class ConfigManager;

#ifndef OTA_PROGRESS_PAYLOAD_DEFINED
#define OTA_PROGRESS_PAYLOAD_DEFINED
struct OTAProgressPayload {
    float progress;       // 0.0 to 1.0 download fraction
    const char* status;   // Status message (e.g. "Downloading...")
    bool complete;        // True when OTA cycle is finished
    bool success;         // True if OTA succeeded (only valid when complete)
};
#endif

#ifndef DISPLAY_MESSAGE_PAYLOAD_DEFINED
#define DISPLAY_MESSAGE_PAYLOAD_DEFINED
struct DisplayMessagePayload {
    const char* message;       // Text to display
    unsigned long durationMs;  // How long to display (0 = until cleared)
};
#endif

#ifndef SYSTEM_ERROR_PAYLOAD_DEFINED
#define SYSTEM_ERROR_PAYLOAD_DEFINED
struct SystemErrorPayload {
    const char* title;   // Error title string
    const char* message; // Error description string
    int code;            // Numeric error code (optional)
};
#endif

namespace LVGLDisplayEvents {
    constexpr const char* DISPLAY_SLEEP = "display.sleep";
    constexpr const char* DISPLAY_WAKE = "display.wake";
    constexpr const char* DISPLAY_MESSAGE = "display.message";
    constexpr const char* SYSTEM_ERROR = "system.error";
}

// TODO: WO7 adds IWebContributor
// TODO: WO8 adds IMCPContributor

/**
 * @brief Generic LVGL v9 display module implementing ITickable, IConfigurable, and IStatusReporter.
 *
 * Handles LVGL lifecycle (lv_init, lv_tick_inc, lv_timer_handler), draw buffer
 * allocation, input device discovery, focus group management, and framework system screens.
 */
class LVGLDisplayModule : public ITickable, public IConfigurable, public IStatusReporter {
public:
    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /**
     * @brief Construct a new LVGLDisplayModule with hardware adapter.
     *
     * @param adapter Reference to concrete ILVGLHardwareAdapter implementation.
     * @param screens Optional pointer to ILVGLSystemScreens provider (default nullptr).
     * @param bootLogo Optional pointer to compiled-in lv_image_dsc_t for Stage 1 boot splash (defaults to cb_labs_boot_logo).
     */
    explicit LVGLDisplayModule(ILVGLHardwareAdapter& adapter,
                               ILVGLSystemScreens* screens = nullptr,
                               const lv_image_dsc_t* bootLogo = nullptr);

    /**
     * @brief Destructor. Cleans up draw buffers and LVGL handles.
     */
    ~LVGLDisplayModule();

    // Prevent copying and moving
    LVGLDisplayModule(const LVGLDisplayModule&) = delete;
    LVGLDisplayModule& operator=(const LVGLDisplayModule&) = delete;

    /**
     * @brief Phase 1: Bootstrap display hardware, LVGL runtime, and Stage-1 boot splash.
     *
     * Calls lv_init(), requests display handle from adapter, allocates partial
     * draw buffers in internal SRAM via DMA if not pre-configured, binds optional
     * input devices, and loads Stage-1 boot splash.
     */
    void bootstrapHardware();

    /**
     * @brief Phase 2: Complete framework integration.
     *
     * Ensures Phase 1 bootstrapHardware() has run, calls optional weak ui_init()
     * hook if defined by application, subscribes to EventBus topics, and exits bootstrap mode.
     *
     * @param eventBus Pointer to framework EventBus instance.
     */
    void begin(EventBus* eventBus = nullptr);

    // -------------------------------------------------------------------------
    // ITickable
    // -------------------------------------------------------------------------

    /**
     * @brief Periodic main-loop update tick.
     *
     * Advances LVGL internal tick counter via lv_tick_inc() based on elapsed time,
     * ticks screensaver controller, and executes pending LVGL tasks via lv_timer_handler().
     *
     * @param now Current timestamp in milliseconds.
     */
    void tick(uint32_t now) override;

    // -------------------------------------------------------------------------
    // IStatusReporter
    // -------------------------------------------------------------------------

    /**
     * @brief Update status message and progress bar on boot screen.
     *
     * @param message  Null-terminated status string.
     * @param progress Fractional progress in [0.0, 1.0]; negative = indeterminate.
     */
    void reportStatus(const char* message, float progress = -1.0f) override;

    /**
     * @brief Report a fatal system error and display error screen.
     *
     * @param errorMessage Null-terminated error description text.
     */
    void reportError(const char* errorMessage) override;

    /**
     * @brief Remove active status/system screen overlay and restore prior screen.
     */
    void clearStatus() override;

    // -------------------------------------------------------------------------
    // System Screen Management API
    // -------------------------------------------------------------------------

    /**
     * @brief Display Stage-1 boot splash / progress screen.
     */
    void showBoot();

    /**
     * @brief Display system error screen.
     *
     * Uses ILVGLSystemScreens provider if non-null and supplying an error screen,
     * otherwise falls back to built-in ErrorScreen.
     *
     * @param title   Error title string.
     * @param message Error description string.
     */
    void showError(const char* title, const char* message);

    /**
     * @brief Display system error screen with numeric error code.
     *
     * @param code    Numeric error code.
     * @param message Error description string.
     */
    void showError(int code, const char* message);

    /**
     * @brief Display OTA firmware update progress screen.
     *
     * Uses ILVGLSystemScreens provider if non-null and supplying an OTA screen,
     * otherwise falls back to built-in OTAProgressScreen.
     *
     * @param progress Fractional progress ratio in [0.0, 1.0].
     * @param status   Status phase text string.
     */
    void showOtaProgress(float progress, const char* status);

    /**
     * @brief Restore screen active prior to error or OTA display.
     */
    void restorePriorScreen();

    /**
     * @brief Override compiled-in Stage-1 boot logo.
     *
     * @param bootLogo Pointer to compiled-in lv_image_dsc_t asset.
     */
    void setBootLogo(const lv_image_dsc_t* bootLogo);

    /**
     * @brief Reset inactivity timer and wake screensaver if dimmed or blanked.
     */
    void resetActivity();

    // -------------------------------------------------------------------------
    // IConfigurable — ConfigManager gateway (namespace: "display")
    // -------------------------------------------------------------------------

    const char* getConfigNamespace() const override { return "display"; }
    int getSchemaVersion() const override { return 1; }
    void applyConfig(const JsonObject& section) override;
    void serializeConfig(JsonObject& section) const override;
    void resetConfig() override;

    // -------------------------------------------------------------------------
    // Runtime Setters & Getters
    // -------------------------------------------------------------------------

    void setConfigManager(ConfigManager* cm) { _configManager = cm; }
    void setBrightness(uint8_t brightness);
    uint8_t getBrightness() const { return _brightness; }
    void setRotation(uint16_t degrees);
    uint16_t getRotation() const { return _rotation; }
    void setScreensaverEnabled(bool enabled);
    bool getScreensaverEnabled() const { return _screensaverEnabled; }
    void setDimTimeout(unsigned long timeoutMs);
    unsigned long getDimTimeout() const { return _dimTimeoutMs; }
    void setBlankTimeout(unsigned long timeoutMs);
    unsigned long getBlankTimeout() const { return _blankTimeoutMs; }

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    lv_display_t* getDisplay() const { return _display; }
    lv_indev_t* getTouchInput() const { return _touchInDev; }
    lv_indev_t* getEncoderInput() const { return _encoderInDev; }
    lv_indev_t* getKeypadInput() const { return _keypadInDev; }
    lv_group_t* getGroup() const { return _group; }
    bool isBootstrapMode() const { return _bootstrapMode; }
    EventBus* getEventBus() const { return _eventBus; }

    BootScreen& getBootScreen() { return _builtInBootScreen; }
    ErrorScreen& getErrorScreen() { return _builtInErrorScreen; }
    OTAProgressScreen& getOtaScreen() { return _builtInOtaScreen; }
    ScreensaverController& getScreensaver() { return _screensaver; }
    const ScreensaverController& getScreensaver() const { return _screensaver; }

private:
    static void _onOtaProgress(const char* topic, const void* payload, void* ctx);
    static void _onDisplayMessage(const char* topic, const void* payload, void* ctx);
    static void _onSystemError(const char* topic, const void* payload, void* ctx);

    ILVGLHardwareAdapter& _adapter;            ///< Injected hardware adapter
    ILVGLSystemScreens*   _screens = nullptr;  ///< Optional system screens provider
    ConfigManager*        _configManager = nullptr; ///< Cached ConfigManager reference
    EventBus*             _eventBus = nullptr; ///< Framework EventBus reference

    lv_display_t* _display = nullptr;          ///< LVGL display instance handle
    lv_indev_t*   _touchInDev = nullptr;       ///< Touch pointer input device
    lv_indev_t*   _encoderInDev = nullptr;     ///< Rotary encoder input device
    lv_indev_t*   _keypadInDev = nullptr;      ///< Keypad/button input device
    lv_group_t*   _group = nullptr;            ///< Widget focus group for encoder/keypad

    uint32_t _lastTickMs = 0;                  ///< Timestamp of last tick() execution
    void*    _buf1 = nullptr;                  ///< Primary draw buffer allocation
    void*    _buf2 = nullptr;                  ///< Secondary draw buffer allocation
    size_t   _bufSize = 0;                     ///< Allocated size per draw buffer in bytes
    bool     _bootstrapMode = false;           ///< True while in Phase 1 bootstrap mode
    bool     _pauseRendering = false;          ///< Pause rendering flag during OTA flash writes
    uint32_t _lastIndevInactiveMs = 0;         ///< Inactivity tracking timestamp

    // --- System Screens & Assets ---
    const lv_image_dsc_t* _bootLogo = nullptr;
    BootScreen            _builtInBootScreen;
    ErrorScreen           _builtInErrorScreen;
    OTAProgressScreen     _builtInOtaScreen;
    lv_obj_t*             _priorScreen = nullptr;
    bool                  _inSystemScreen = false;

    // --- Configuration state ---
    uint8_t               _brightness = 255;          ///< Backlight brightness level (0–255)
    uint16_t              _rotation = 0;              ///< Display rotation in degrees (0, 90, 180, 270)
    bool                  _screensaverEnabled = true; ///< Inactivity screensaver enabled flag
    unsigned long         _dimTimeoutMs = 150000;     ///< Inactivity dim timeout in ms (default 2.5 min)
    unsigned long         _blankTimeoutMs = 300000;   ///< Inactivity blank timeout in ms (default 5 min)
    ScreensaverController _screensaver;               ///< Screensaver state machine
};

