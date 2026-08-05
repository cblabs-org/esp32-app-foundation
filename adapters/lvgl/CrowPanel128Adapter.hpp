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
    LGFX_CrowPanel128();
};

class CrowPanel128Adapter : public ILVGLHardwareAdapter {
public:
    CrowPanel128Adapter();
    ~CrowPanel128Adapter() override;

    void setInputModules(EncoderModule* enc, ButtonModule* btn, EventBus* bus);

    lv_display_t* createDisplay() override;
    void setBrightness(uint8_t level) override;
    uint16_t getScreenWidth() const override { return 240; }
    uint16_t getScreenHeight() const override { return 240; }
    uint16_t getBufferLineCount() const override { return 40; } // 40 lines of 240 pixels is 9600px. x2 bytes = 19.2KB

    lv_indev_t* createTouchInput() override;
    lv_indev_t* createEncoderInput() override;
    lv_indev_t* createKeypadInput() override { return nullptr; }

private:
    LGFX_CrowPanel128    _gfx;
    EncoderModule*       _encoderModule = nullptr;
    ButtonModule*        _buttonModule  = nullptr;
    EventBus*            _bus           = nullptr;
    std::atomic<bool>    _buttonPressed{false};

    static void _flushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);
    static void _touchReadCallback(lv_indev_t* indev, lv_indev_data_t* data);
    static void _encoderReadCallback(lv_indev_t* indev, lv_indev_data_t* data);
    static void _buttonEventCallback(const char* topic, const void* payload, void* ctx);
};
