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
 *              lifecycle, draw buffer allocation, tick updates, and input device
 *              focus groups via constructor-injected ILVGLHardwareAdapter.
 *
 * Exported Classes:
 * - LVGLDisplayModule: Generic LVGL display service module
 *
 *   Lifecycle:
 *   - LVGLDisplayModule(): Constructor taking adapter reference and optional system screens
 *   - ~LVGLDisplayModule(): Destructor freeing allocated draw buffers and handles
 *   - bootstrapHardware(): Phase 1 hardware initialization and buffer setup
 *   - begin():              Phase 2 framework integration and ui_init hook
 *
 *   ITickable:
 *   - tick():               Advances LVGL tick clock and calls lv_timer_handler()
 *
 *   Accessors:
 *   - getDisplay():         Return underlying lv_display_t pointer
 *   - getTouchInput():      Return touch input device pointer (or nullptr)
 *   - getEncoderInput():    Return encoder input device pointer (or nullptr)
 *   - getKeypadInput():     Return keypad input device pointer (or nullptr)
 *   - getGroup():           Return widget focus group pointer (or nullptr)
 *   - isBootstrapMode():    Return true if in Phase 1 bootstrap mode
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include "interfaces/ITickable.hpp"
#include "interfaces/ILVGLHardwareAdapter.hpp"
#include "interfaces/ILVGLSystemScreens.hpp"
#include "lvgl.h"

// Forward declarations
class EventBus;

// TODO: WO4 adds IConfigurable
// TODO: WO5 adds IStatusReporter
// TODO: WO6 adds EventBus subscriptions & screensaver
// TODO: WO7 adds IWebContributor
// TODO: WO8 adds IMCPContributor

/**
 * @brief Generic LVGL v9 display module implementing ITickable.
 *
 * Handles LVGL lifecycle (lv_init, lv_tick_inc, lv_timer_handler), draw buffer
 * allocation, input device discovery, and focus group management. Delegates
 * display and input initialization to an ILVGLHardwareAdapter instance.
 */
class LVGLDisplayModule : public ITickable {
public:
    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /**
     * @brief Construct a new LVGLDisplayModule with hardware adapter.
     *
     * @param adapter Reference to concrete ILVGLHardwareAdapter implementation.
     * @param screens Optional pointer to ILVGLSystemScreens provider (default nullptr).
     */
    explicit LVGLDisplayModule(ILVGLHardwareAdapter& adapter, ILVGLSystemScreens* screens = nullptr);

    /**
     * @brief Destructor. Cleans up draw buffers and LVGL handles.
     */
    ~LVGLDisplayModule();

    // Prevent copying and moving
    LVGLDisplayModule(const LVGLDisplayModule&) = delete;
    LVGLDisplayModule& operator=(const LVGLDisplayModule&) = delete;

    /**
     * @brief Phase 1: Bootstrap display hardware and LVGL runtime.
     *
     * Calls lv_init(), requests display handle from adapter, allocates partial
     * draw buffers in internal SRAM via DMA if not pre-configured (OQ-7), and
     * binds optional input devices (touch, encoder, keypad) to an lv_group_t.
     */
    void bootstrapHardware();

    /**
     * @brief Phase 2: Complete framework integration.
     *
     * Ensures Phase 1 bootstrapHardware() has run, calls optional weak ui_init()
     * hook if defined by application, and exits bootstrap mode.
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
     * then executes pending LVGL tasks via lv_timer_handler().
     *
     * @param now Current timestamp in milliseconds.
     */
    void tick(uint32_t now) override;

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    /** @brief Get the underlying LVGL display pointer. */
    lv_display_t* getDisplay() const { return _display; }

    /** @brief Get the touch input device pointer, or nullptr if unavailable. */
    lv_indev_t* getTouchInput() const { return _touchInDev; }

    /** @brief Get the encoder input device pointer, or nullptr if unavailable. */
    lv_indev_t* getEncoderInput() const { return _encoderInDev; }

    /** @brief Get the keypad input device pointer, or nullptr if unavailable. */
    lv_indev_t* getKeypadInput() const { return _keypadInDev; }

    /** @brief Get the input focus group pointer, or nullptr if no encoder/keypad. */
    lv_group_t* getGroup() const { return _group; }

    /** @brief Check if module is currently in Phase 1 bootstrap mode. */
    bool isBootstrapMode() const { return _bootstrapMode; }

private:
    ILVGLHardwareAdapter& _adapter;            ///< Injected hardware adapter
    ILVGLSystemScreens*   _screens = nullptr;  ///< Optional system screens provider

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
};
