/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_lvglDisplay/test_lifecycle.cpp
 * Description: Unit tests for LVGLDisplayModule two-phase lifecycle, buffer
 *              allocation strategies (OQ-7), tick cadence, and input/group wiring.
 */

#include <gtest/gtest.h>
#include "MockLVGLHardwareAdapter.hpp"
#include "LVGLDisplay.hpp"

class LVGLDisplayLifecycleTest : public ::testing::Test {
protected:
    MockLVGLHardwareAdapter adapter;
};

// -----------------------------------------------------------------------------
// Two-Phase Lifecycle Tests
// -----------------------------------------------------------------------------

TEST_F(LVGLDisplayLifecycleTest, TwoPhaseInitializationPhase1And2) {
    LVGLDisplayModule module(adapter);

    EXPECT_FALSE(module.isBootstrapMode());
    EXPECT_EQ(module.getDisplay(), nullptr);

    // Phase 1: bootstrapHardware()
    module.bootstrapHardware();

    EXPECT_TRUE(module.isBootstrapMode());
    EXPECT_NE(module.getDisplay(), nullptr);

    // Phase 2: begin()
    module.begin(nullptr);

    EXPECT_FALSE(module.isBootstrapMode());
    EXPECT_NE(module.getDisplay(), nullptr);
}

TEST_F(LVGLDisplayLifecycleTest, BeginCallsBootstrapIfSkipped) {
    LVGLDisplayModule module(adapter);

    EXPECT_EQ(module.getDisplay(), nullptr);

    // Call begin directly without bootstrapHardware()
    module.begin(nullptr);

    EXPECT_FALSE(module.isBootstrapMode());
    EXPECT_NE(module.getDisplay(), nullptr);
}

// -----------------------------------------------------------------------------
// Buffer Allocation Tests (OQ-7)
// -----------------------------------------------------------------------------

TEST_F(LVGLDisplayLifecycleTest, BufferAllocationDefaultModuleAllocated) {
    adapter.screenWidth = 240;
    adapter.bufferLineCount = 24;
    adapter.preConfigureBuffers = false;

    LVGLDisplayModule module(adapter);
    module.bootstrapHardware();

    lv_display_t* disp = module.getDisplay();
    ASSERT_NE(disp, nullptr);

    // Verify module allocated buffers using getBufferLineCount()
    EXPECT_NE(lv_display_get_buf_1(disp), nullptr);
    EXPECT_NE(lv_display_get_buf_2(disp), nullptr);

    size_t expectedSize = 240 * 24 * sizeof(lv_color_t);
    EXPECT_EQ(disp->buf_size, expectedSize);
}

TEST_F(LVGLDisplayLifecycleTest, BufferAllocationOQ7ModuleAlwaysAllocates) {
    // OQ-7: With _buffersConfigured flag (LVGL v9 fix), the module always
    // allocates its own draw buffers. Adapter pre-configuration is overridden.
    adapter.preConfigureBuffers = true;

    LVGLDisplayModule module(adapter);
    module.bootstrapHardware();

    lv_display_t* disp = module.getDisplay();
    ASSERT_NE(disp, nullptr);

    // Module allocates its own buffers based on width * lineCount * sizeof(lv_color_t)
    size_t expectedSize = 240 * 24 * sizeof(lv_color_t);
    EXPECT_EQ(disp->buf_size, expectedSize);
}

// -----------------------------------------------------------------------------
// Tick Cadence Tests
// -----------------------------------------------------------------------------

TEST_F(LVGLDisplayLifecycleTest, TickAdvancesCadenceWithoutCrash) {
    LVGLDisplayModule module(adapter);
    module.bootstrapHardware();

    // First tick caches initial timestamp
    module.tick(1000);

    // Subsequent ticks advance elapsed delta
    module.tick(1033);
    module.tick(1066);
    module.tick(1100);

    SUCCEED();
}

// -----------------------------------------------------------------------------
// Input Device & Focus Group Wiring Tests
// -----------------------------------------------------------------------------

TEST_F(LVGLDisplayLifecycleTest, InputGroupWiringNullWhenNoInputs) {
    LVGLDisplayModule module(adapter);
    module.bootstrapHardware();

    EXPECT_EQ(module.getTouchInput(), nullptr);
    EXPECT_EQ(module.getEncoderInput(), nullptr);
    EXPECT_EQ(module.getKeypadInput(), nullptr);
    EXPECT_EQ(module.getGroup(), nullptr);
}

TEST_F(LVGLDisplayLifecycleTest, InputGroupWiringEncoderCreatesGroup) {
    adapter.enableMockEncoder();

    LVGLDisplayModule module(adapter);
    module.bootstrapHardware();

    EXPECT_NE(module.getEncoderInput(), nullptr);
    EXPECT_NE(module.getGroup(), nullptr);
    EXPECT_EQ(module.getEncoderInput()->group, module.getGroup());
}

TEST_F(LVGLDisplayLifecycleTest, InputGroupWiringTouchDoesNotRequireGroup) {
    adapter.enableMockTouch();

    LVGLDisplayModule module(adapter);
    module.bootstrapHardware();

    EXPECT_NE(module.getTouchInput(), nullptr);
    EXPECT_EQ(module.getEncoderInput(), nullptr);
    EXPECT_EQ(module.getGroup(), nullptr);
}

TEST_F(LVGLDisplayLifecycleTest, InputGroupWiringMultipleInputs) {
    adapter.enableMockTouch();
    adapter.enableMockEncoder();
    adapter.enableMockKeypad();

    LVGLDisplayModule module(adapter);
    module.bootstrapHardware();

    EXPECT_NE(module.getTouchInput(), nullptr);
    EXPECT_NE(module.getEncoderInput(), nullptr);
    EXPECT_NE(module.getKeypadInput(), nullptr);
    EXPECT_NE(module.getGroup(), nullptr);

    EXPECT_EQ(module.getEncoderInput()->group, module.getGroup());
    EXPECT_EQ(module.getKeypadInput()->group, module.getGroup());
}
