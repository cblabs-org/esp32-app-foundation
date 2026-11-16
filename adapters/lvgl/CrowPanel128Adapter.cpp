/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: adapters/lvgl/CrowPanel128Adapter.cpp
 * Description: CrowPanel128Adapter implementation.
 *
 */

#include "CrowPanel128Adapter.hpp"
#include "board.h"
#include <Arduino.h>
#include <cstring>
#include "EncoderModule.hpp"
#include "ButtonModule.hpp"
#include "EventBus.hpp"

// -------------------------------------------------------------------------
// LGFX Panel Configuration
// -------------------------------------------------------------------------
LGFX_CrowPanel128::LGFX_CrowPanel128() {
    {
        auto cfg = _bus_instance.config();
        cfg.spi_host = SPI2_HOST;
        cfg.spi_mode = 0;
        cfg.freq_write = 80000000;
        cfg.freq_read  = 16000000;
        cfg.spi_3wire  = true;
        cfg.use_lock   = true;
        cfg.dma_channel = SPI_DMA_CH_AUTO;
        cfg.pin_sclk = PIN_LCD_SPI_SCLK;
        cfg.pin_mosi = PIN_LCD_SPI_MOSI;
        cfg.pin_miso = -1;
        cfg.pin_dc   = PIN_LCD_DC;
        _bus_instance.config(cfg);
        _panel_instance.setBus(&_bus_instance);
    }
    {
        auto cfg = _panel_instance.config();
        cfg.pin_cs           = PIN_LCD_CS;
        cfg.pin_rst          = PIN_LCD_RST;
        cfg.pin_busy         = -1;
        cfg.panel_width      = 240;
        cfg.panel_height     = 240;
        cfg.offset_x         = 0;
        cfg.offset_y         = 0;
        cfg.offset_rotation  = 0;
        cfg.dummy_read_pixel = 8;
        cfg.dummy_read_bits  = 1;
        cfg.readable         = false;
        cfg.invert           = true;
        cfg.rgb_order        = false;
        cfg.dlen_16bit       = false;
        cfg.bus_shared       = true;
        _panel_instance.config(cfg);
    }
    {
        auto cfg = _light_instance.config();
        cfg.pin_bl = PIN_LCD_BL;
        cfg.invert = false;
        cfg.freq   = 44100;
        cfg.pwm_channel = 7;
        _light_instance.config(cfg);
        _panel_instance.setLight(&_light_instance);
    }
    {
        auto cfg = _touch_instance.config();
        cfg.x_min      = 0;
        cfg.x_max      = 239;
        cfg.y_min      = 0;
        cfg.y_max      = 239;
        cfg.pin_int    = PIN_TOUCH_INT;
        cfg.bus_shared = false;
        cfg.offset_rotation = 0;
        cfg.i2c_port   = 1;
        cfg.i2c_addr   = 0x15; // CST816D address
        cfg.pin_sda    = PIN_TOUCH_SDA;
        cfg.pin_scl    = PIN_TOUCH_SCL;
        cfg.freq       = 400000;
        _touch_instance.config(cfg);
        _panel_instance.setTouch(&_touch_instance);
    }
    setPanel(&_panel_instance);
}

// -------------------------------------------------------------------------
// CrowPanel128Adapter Implementation
// -------------------------------------------------------------------------
CrowPanel128Adapter::CrowPanel128Adapter() {}

CrowPanel128Adapter::~CrowPanel128Adapter() {
    if (_bus) {
        _bus->unsubscribe("button.press_down", _buttonEventCallback, this);
        _bus->unsubscribe("button.click", _buttonEventCallback, this);
        _bus->unsubscribe("button.double_click", _buttonEventCallback, this);
        _bus->unsubscribe("button.long_press", _buttonEventCallback, this);
    }
}

void CrowPanel128Adapter::setInputModules(EncoderModule* enc, ButtonModule* btn, EventBus* bus) {
    _encoderModule = enc;
    _buttonModule = btn;
    _bus = bus;
    if (_bus) {
        _bus->subscribe("button.press_down", _buttonEventCallback, this);
        _bus->subscribe("button.click", _buttonEventCallback, this);
        _bus->subscribe("button.double_click", _buttonEventCallback, this);
        _bus->subscribe("button.long_press", _buttonEventCallback, this);
    }
}

void CrowPanel128Adapter::_buttonEventCallback(const char* topic, const void* payload, void* ctx) {
    auto* adapter = static_cast<CrowPanel128Adapter*>(ctx);
    if (!adapter) return;
    if (topic && strcmp(topic, "button.press_down") == 0) {
        adapter->_buttonPressed.store(true);
    } else {
        adapter->_buttonPressed.store(false);
    }
}

lv_display_t* CrowPanel128Adapter::createDisplay() {
    // Assert the panel power-enable rails BEFORE initialising the GC9A01.
    // The CrowPanel 1.28" gates LCD power behind GPIO1/GPIO2 (PIN_LCD_PWR_EN1/2);
    // both must be driven HIGH or the panel stays unpowered and the screen is
    // black at any backlight level. Mirrors the Elecrow factory example, which
    // sets GPIO1/GPIO2 HIGH immediately before gfx.init(). (RFC #50 must-fix.)
    pinMode(PIN_LCD_PWR_EN1, OUTPUT);
    digitalWrite(PIN_LCD_PWR_EN1, HIGH);
    pinMode(PIN_LCD_PWR_EN2, OUTPUT);
    digitalWrite(PIN_LCD_PWR_EN2, HIGH);

    _gfx.init();

    lv_display_t* disp = lv_display_create(240, 240);
    lv_display_set_user_data(disp, this);
    lv_display_set_flush_cb(disp, _flushCallback);
    
    return disp;
}

void CrowPanel128Adapter::_flushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    auto* adapter = static_cast<CrowPanel128Adapter*>(lv_display_get_user_data(disp));
    if (adapter) {
        // LVGL 9 renders RGB565 in native (little-endian) byte order and lv_conf.h sets no
        // colour swap, so the buffer is lgfx::rgb565_t. Pushing it as swap565_t byte-swaps every
        // pixel and scrambles the colours on-panel. Matches the Elecrow factory example, which
        // pushes (lgfx::rgb565_t*). (WO7 D3.)
        adapter->_gfx.pushImage(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (lgfx::rgb565_t*)px_map);
    }
    lv_display_flush_ready(disp);
}

void CrowPanel128Adapter::setBrightness(uint8_t level) {
    _gfx.setBrightness(level);
}

lv_indev_t* CrowPanel128Adapter::createTouchInput() {
    lv_indev_t* indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_user_data(indev, this);
    lv_indev_set_read_cb(indev, _touchReadCallback);
    return indev;
}

void CrowPanel128Adapter::_touchReadCallback(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* adapter = static_cast<CrowPanel128Adapter*>(lv_indev_get_user_data(indev));
    if (!adapter) return;
    
    uint16_t x, y;
    if (adapter->_gfx.getTouch(&x, &y)) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x;
        data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

lv_indev_t* CrowPanel128Adapter::createEncoderInput() {
    lv_indev_t* indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_user_data(indev, this);
    lv_indev_set_read_cb(indev, _encoderReadCallback);
    return indev;
}

void CrowPanel128Adapter::_encoderReadCallback(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* adapter = static_cast<CrowPanel128Adapter*>(lv_indev_get_user_data(indev));
    if (!adapter) return;
    
    // Read rotation via framework EncoderModule (RAW mode, no acceleration).
    // The PCNT quadrature encoder produces 2 counts per physical detent,
    // so divide by 2 for a clean 1 click = 1 carousel step.
    static constexpr int32_t ENC_DIVISOR = 2;
    if (adapter->_encoderModule) {
        adapter->_encoderAccum += adapter->_encoderModule->consumeDelta(EncoderDelta::RAW);
        data->enc_diff = adapter->_encoderAccum / ENC_DIVISOR;
        adapter->_encoderAccum %= ENC_DIVISOR;
    } else {
        data->enc_diff = 0;
    }
    
    // Read button cached state from ButtonModule EventBus subscription
    if (adapter->_buttonPressed.load()) {
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void CrowPanel128Adapter::pollTouch() {
    uint16_t x, y;
    bool touching = _gfx.getTouch(&x, &y);
    if (touching && !_touchActive) {
        // Rising edge — finger just made contact
        if (_bus) {
            _bus->publish("touch.press_down");
        }
    }
    _touchActive = touching;
}
