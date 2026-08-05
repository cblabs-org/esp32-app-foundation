/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_lvglDisplay/MockLVGLHardwareAdapter.hpp
 * Description: Mock hardware adapter implementation for LVGLDisplayModule unit testing.
 *
 * Exported Classes:
 * - MockLVGLHardwareAdapter: Configurable test double for ILVGLHardwareAdapter
 */

#pragma once

#include "interfaces/ILVGLHardwareAdapter.hpp"
#include "lvgl.h"

/**
 * @brief Mock implementation of ILVGLHardwareAdapter for unit testing.
 */
class MockLVGLHardwareAdapter : public ILVGLHardwareAdapter {
public:
    uint16_t screenWidth = 240;
    uint16_t screenHeight = 240;
    uint16_t bufferLineCount = 24;
    uint8_t brightnessLevel = 255;
    bool preConfigureBuffers = false;

    lv_display_t* createdDisplay = nullptr;
    lv_indev_t* touchInDev = nullptr;
    lv_indev_t* encoderInDev = nullptr;
    lv_indev_t* keypadInDev = nullptr;

    ~MockLVGLHardwareAdapter() override {
        if (touchInDev != nullptr) {
            lv_indev_delete(touchInDev);
            touchInDev = nullptr;
        }
        if (encoderInDev != nullptr) {
            lv_indev_delete(encoderInDev);
            encoderInDev = nullptr;
        }
        if (keypadInDev != nullptr) {
            lv_indev_delete(keypadInDev);
            keypadInDev = nullptr;
        }
    }

    lv_display_t* createDisplay() override {
        createdDisplay = lv_display_create(screenWidth, screenHeight);
        if (preConfigureBuffers && createdDisplay != nullptr) {
            static uint8_t mockBuf1[5760];
            static uint8_t mockBuf2[5760];
            lv_display_set_buffers(createdDisplay, mockBuf1, mockBuf2, sizeof(mockBuf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
        }
        return createdDisplay;
    }

    void setBrightness(uint8_t level) override {
        brightnessLevel = level;
    }

    uint16_t getScreenWidth() const override { return screenWidth; }
    uint16_t getScreenHeight() const override { return screenHeight; }
    uint16_t getBufferLineCount() const override { return bufferLineCount; }

    lv_indev_t* createTouchInput() override {
        return touchInDev;
    }

    lv_indev_t* createEncoderInput() override {
        return encoderInDev;
    }

    lv_indev_t* createKeypadInput() override {
        return keypadInDev;
    }

    void enableMockEncoder() {
        if (encoderInDev == nullptr) {
            encoderInDev = lv_indev_create();
            lv_indev_set_type(encoderInDev, LV_INDEV_TYPE_ENCODER);
        }
    }

    void enableMockTouch() {
        if (touchInDev == nullptr) {
            touchInDev = lv_indev_create();
            lv_indev_set_type(touchInDev, LV_INDEV_TYPE_POINTER);
        }
    }

    void enableMockKeypad() {
        if (keypadInDev == nullptr) {
            keypadInDev = lv_indev_create();
            lv_indev_set_type(keypadInDev, LV_INDEV_TYPE_KEYPAD);
        }
    }
};
