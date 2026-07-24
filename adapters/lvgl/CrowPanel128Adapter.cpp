#include "CrowPanel128Adapter.hpp"
#include <Arduino.h>

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
        cfg.pin_sclk = LCD_SPI_SCLK;
        cfg.pin_mosi = LCD_SPI_MOSI;
        cfg.pin_miso = -1;
        cfg.pin_dc   = LCD_DC;
        _bus_instance.config(cfg);
        _panel_instance.setBus(&_bus_instance);
    }
    {
        auto cfg = _panel_instance.config();
        cfg.pin_cs           = LCD_CS;
        cfg.pin_rst          = LCD_RST;
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
        cfg.pin_bl = LCD_BL;
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
        cfg.pin_int    = TOUCH_INT;
        cfg.bus_shared = false;
        cfg.offset_rotation = 0;
        cfg.i2c_port   = 1;
        cfg.i2c_addr   = 0x15; // CST816D address
        cfg.pin_sda    = TOUCH_SDA;
        cfg.pin_scl    = TOUCH_SCL;
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
CrowPanel128Adapter::~CrowPanel128Adapter() {}

lv_display_t* CrowPanel128Adapter::createDisplay() {
    _gfx.init();
    
    lv_display_t* disp = lv_display_create(240, 240);
    lv_display_set_user_data(disp, this);
    lv_display_set_flush_cb(disp, _flushCallback);
    
    return disp;
}

void CrowPanel128Adapter::_flushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    auto* adapter = static_cast<CrowPanel128Adapter*>(lv_display_get_user_data(disp));
    if (adapter) {
        adapter->_gfx.pushImage(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (lgfx::swap565_t*)px_map);
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
    // Setup ESP32Encoder
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    _encoder.attachHalfQuad(ENCODER_A, ENCODER_B);
    _encoder.clearCount();
    
    // Setup button
    pinMode(ENCODER_SW, INPUT_PULLUP);
    
    lv_indev_t* indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_user_data(indev, this);
    lv_indev_set_read_cb(indev, _encoderReadCallback);
    return indev;
}

void CrowPanel128Adapter::_encoderReadCallback(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* adapter = static_cast<CrowPanel128Adapter*>(lv_indev_get_user_data(indev));
    if (!adapter) return;
    
    // Read rotation
    int64_t count = adapter->_encoder.getCount();
    if (count != 0) {
        data->enc_diff = count;
        adapter->_encoder.clearCount();
    } else {
        data->enc_diff = 0;
    }
    
    // Read button
    if (digitalRead(ENCODER_SW) == LOW) {
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
