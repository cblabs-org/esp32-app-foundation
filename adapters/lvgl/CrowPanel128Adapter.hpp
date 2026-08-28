/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: adapters/lvgl/CrowPanel128Adapter.hpp
 * Description: Implementation of EncoderModule, ButtonModule, EventBus classes.
 *
 * Exported Classes:
 * - EncoderModule
 * - ButtonModule
 * - EventBus
 * - LGFX_CrowPanel128
 * - CrowPanel128Adapter
 */

#pragma once
#include "interfaces/ILVGLHardwareAdapter.hpp"
#include <LovyanGFX.hpp>
#include <atomic>

class EncoderModule;
class ButtonModule;
class EventBus;

class LGFX_CrowPanel128 : public lgfx::LGFX_Device {
    lgfx::Panel_GC9A01      _panel_instance;
    lgfx::Bus_SPI           _bus_instance;
    lgfx::Light_PWM         _light_instance;
    lgfx::Touch_CST816S     _touch_instance; // CST816D is compatible with CST816S driver in LovyanGFX
public:
    /** @brief Construct a LGFX_CrowPanel128 instance. */
    LGFX_CrowPanel128();
};

class CrowPanel128Adapter : public ILVGLHardwareAdapter {
public:
    /** @brief Construct a CrowPanel128Adapter instance. */
    CrowPanel128Adapter();
    ~CrowPanel128Adapter() override;

    /** @brief Set the input modules. */
    void setInputModules(EncoderModule* enc, ButtonModule* btn, EventBus* bus);

    /** @brief Create Display. */
    lv_display_t* createDisplay() override;
    /** @brief Set the brightness. */
    void setBrightness(uint8_t level) override;
    /** @brief Return screen width in pixels. */
    uint16_t getScreenWidth() const override { return 240; }
    /** @brief Return screen height in pixels. */
    uint16_t getScreenHeight() const override { return 240; }
    /** @brief Return number of lines per draw buffer. */
    uint16_t getBufferLineCount() const override { return 40; } // 40 lines of 240 pixels is 9600px. x2 bytes = 19.2KB

    /** @brief Create Touch Input. */
    lv_indev_t* createTouchInput() override;
    /** @brief Create Encoder Input. */
    lv_indev_t* createEncoderInput() override;
    lv_indev_t* createKeypadInput() override { return nullptr; }
    /** @brief Poll Touch. */
    void pollTouch() override;

private:
    LGFX_CrowPanel128    _gfx;
    EncoderModule*       _encoderModule = nullptr;
    ButtonModule*        _buttonModule  = nullptr;
    EventBus*            _bus           = nullptr;
    std::atomic<bool>    _buttonPressed{false};
    bool                 _touchActive = false;  ///< Edge detector for touch.press_down
    int32_t              _encoderAccum = 0;     ///< Accumulator for encoder divisor (10:1)

    static void _flushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);
    static void _touchReadCallback(lv_indev_t* indev, lv_indev_data_t* data);
    static void _encoderReadCallback(lv_indev_t* indev, lv_indev_data_t* data);
    static void _buttonEventCallback(const char* topic, const void* payload, void* ctx);
};
