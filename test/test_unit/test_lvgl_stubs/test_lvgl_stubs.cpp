/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_lvgl_stubs/test_lvgl_stubs.cpp
 * Description: Unit tests for LVGL native build stubs and interface definitions.
 */

#include <gtest/gtest.h>
#include "lvgl.h"
#include "esp_lcd.h"
#include "esp_lvgl_port.h"
#include "interfaces/ILVGLHardwareAdapter.hpp"
#include "interfaces/ILVGLSystemScreens.hpp"

class MockLVGLHardwareAdapter : public ILVGLHardwareAdapter {
public:
    uint8_t brightness = 255;

    lv_display_t* createDisplay() override {
        return lv_display_create(240, 240);
    }

    void setBrightness(uint8_t level) override {
        brightness = level;
    }

    uint16_t getScreenWidth() const override { return 240; }
    uint16_t getScreenHeight() const override { return 240; }
    uint16_t getBufferLineCount() const override { return 24; }

    lv_indev_t* createTouchInput() override {
        lv_indev_t* touch = lv_indev_create();
        lv_indev_set_type(touch, LV_INDEV_TYPE_POINTER);
        return touch;
    }

    lv_indev_t* createEncoderInput() override {
        lv_indev_t* enc = lv_indev_create();
        lv_indev_set_type(enc, LV_INDEV_TYPE_ENCODER);
        return enc;
    }
};

class MockSystemScreens : public ILVGLSystemScreens {
public:
    bool otaCalled = false;
    bool errorCalled = false;

    void onOtaProgress(float progress, const char* status) override {
        (void)progress;
        (void)status;
        otaCalled = true;
    }

    void onError(const char* title, const char* message) override {
        (void)title;
        (void)message;
        errorCalled = true;
    }
};

TEST(LVGLStubsTest, LifecycleAndDisplayCreation) {
    lv_init();
    lv_tick_inc(10);
    EXPECT_EQ(lv_timer_handler(), 5u);

    lv_display_t* disp = lv_display_create(240, 240);
    ASSERT_NE(disp, nullptr);
    EXPECT_EQ(lv_display_get_horizontal_resolution(disp), 240);
    EXPECT_EQ(lv_display_get_vertical_resolution(disp), 240);

    lv_display_set_brightness(disp, 128);
    EXPECT_EQ(lv_display_get_brightness(disp), 128);

    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);
    EXPECT_EQ(lv_display_get_rotation(disp), LV_DISPLAY_ROTATION_90);

    lv_display_delete(disp);
}

TEST(LVGLStubsTest, InputDevicesAndGroups) {
    lv_indev_t* indev = lv_indev_create();
    ASSERT_NE(indev, nullptr);

    lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
    lv_group_t* group = lv_group_create();
    ASSERT_NE(group, nullptr);

    lv_indev_set_group(indev, group);
    EXPECT_EQ(indev->group, group);

    lv_obj_t* obj = lv_obj_create(nullptr);
    ASSERT_NE(obj, nullptr);

    lv_group_add_obj(group, obj);
    lv_obj_delete(obj);
    lv_group_delete(group);
    lv_indev_delete(indev);
}

TEST(LVGLStubsTest, ObjectsAndWidgets) {
    lv_obj_t* scr = lv_scr_act();
    ASSERT_NE(scr, nullptr);

    lv_obj_t* label = lv_label_create(scr);
    ASSERT_NE(label, nullptr);
    lv_label_set_text(label, "Hello LVGL Native Stub");

    lv_obj_t* arc = lv_arc_create(scr);
    ASSERT_NE(arc, nullptr);
    lv_arc_set_value(arc, 50);

    lv_obj_t* bar = lv_bar_create(scr);
    ASSERT_NE(bar, nullptr);
    lv_bar_set_value(bar, 75, 0);

    lv_obj_t* img = lv_image_create(scr);
    ASSERT_NE(img, nullptr);

    lv_obj_delete(label);
    lv_obj_delete(arc);
    lv_obj_delete(bar);
    lv_obj_delete(img);
}

TEST(LVGLStubsTest, HardwareAdapterInterface) {
    MockLVGLHardwareAdapter adapter;
    EXPECT_EQ(adapter.getScreenWidth(), 240);
    EXPECT_EQ(adapter.getScreenHeight(), 240);
    EXPECT_EQ(adapter.getBufferLineCount(), 24);

    lv_display_t* disp = adapter.createDisplay();
    ASSERT_NE(disp, nullptr);
    lv_display_delete(disp);

    adapter.setBrightness(200);
    EXPECT_EQ(adapter.brightness, 200);

    lv_indev_t* touch = adapter.createTouchInput();
    ASSERT_NE(touch, nullptr);
    EXPECT_EQ(touch->type, static_cast<uint8_t>(LV_INDEV_TYPE_POINTER));
    lv_indev_delete(touch);

    lv_indev_t* enc = adapter.createEncoderInput();
    ASSERT_NE(enc, nullptr);
    EXPECT_EQ(enc->type, static_cast<uint8_t>(LV_INDEV_TYPE_ENCODER));
    lv_indev_delete(enc);
}

TEST(LVGLStubsTest, SystemScreensInterface) {
    MockSystemScreens screens;
    EXPECT_EQ(screens.otaScreen(), nullptr);
    EXPECT_EQ(screens.errorScreen(), nullptr);

    screens.onOtaProgress(50.0f, "Downloading...");
    EXPECT_TRUE(screens.otaCalled);

    screens.onError("WiFi Error", "Connection failed");
    EXPECT_TRUE(screens.errorCalled);
}

TEST(LVGLStubsTest, ESPLCDPortStubs) {
    esp_lcd_panel_io_handle_t io_handle = nullptr;
    esp_lcd_panel_dev_config_t dev_config = {};
    esp_lcd_panel_io_spi_config_t spi_config = {};

    EXPECT_EQ(esp_lcd_new_panel_io_spi(nullptr, &spi_config, &io_handle), ESP_OK);
    ASSERT_NE(io_handle, nullptr);

    lvgl_port_cfg_t port_cfg = {};
    EXPECT_EQ(lvgl_port_init(&port_cfg), ESP_OK);

    lvgl_port_disp_config_t disp_cfg = {};
    disp_cfg.hres = 240;
    disp_cfg.vres = 240;
    lv_display_t* disp = lvgl_port_add_disp(&disp_cfg);
    ASSERT_NE(disp, nullptr);
    lv_display_delete(disp);
}
