/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_u8g2Display/test_module.cpp
 * Description: GoogleTest unit-test suite for U8G2DisplayModule constructor options
 *              and system screen provider overrides (IU8G2SystemScreens).
 *
 * Exported Functions:
 * - (GoogleTest fixtures — discovered automatically by gtest_main)
 */

#include <gtest/gtest.h>
#include "NativeU8G2DisplayType.h"
#include "U8G2Display.hpp"
#include "modules/U8G2Display/IU8G2SystemScreens.hpp"
#include "modules/U8G2Display/IDisplayBoard.hpp"

// =============================================================================
// Mocks for System Screen Overrides
// =============================================================================

/**
 * @brief Dummy custom display board for provider testing.
 */
class DummyCustomBoard : public IDisplayBoard {
public:
    void render(U8G2& /*u8g2*/) override {}
    bool isExpired() const override { return false; }
    const char* getBoardName() const override { return "Custom Error Board"; }
};

/**
 * @brief Mock IU8G2SystemScreens implementation returning custom screens.
 */
class MockSystemScreens : public IU8G2SystemScreens {
public:
    IDisplayBoard* errorBoard() override {
        return &_customErrorBoard;
    }

    DummyCustomBoard _customErrorBoard;
};

// =============================================================================
// Test Group: U8G2DisplayModule System Screens
// =============================================================================

/**
 * Test 4: U8G2DisplayModule constructed with IU8G2SystemScreens returns the custom
 * board from the provider when reportError() is invoked.
 */
TEST(U8G2DisplayModuleTest, SystemScreens_CustomProvider_ReturnsCustomBoard) {
    MockSystemScreens customScreens;
    U8G2DisplayModule display(nullptr, nullptr, &customScreens);

    display.reportError("Testing custom error screen");

    EXPECT_EQ(display.visibleBoard(), &customScreens._customErrorBoard);
    EXPECT_STREQ(display.visibleBoard()->getBoardName(), "Custom Error Board");
}

/**
 * Test 5: U8G2DisplayModule constructed with nullptr screens provider falls back to
 * the built-in default system error board.
 */
TEST(U8G2DisplayModuleTest, SystemScreens_NullProvider_FallsBackToDefaultBoard) {
    U8G2DisplayModule display(nullptr, nullptr, nullptr);

    display.reportError("Testing default error screen");

    EXPECT_EQ(display.visibleBoard(), &display.boards.error);
    EXPECT_STREQ(display.visibleBoard()->getBoardName(), "SYS: Error");
}

/**
 * Test 6: U8G2DisplayModule getTickBudgetMs returns 33 ms (matching ~30 FPS frame rate).
 */
TEST(U8G2DisplayModuleTest, GetTickBudgetMs_Returns33) {
    U8G2DisplayModule display;
    EXPECT_EQ(display.getTickBudgetMs(), 33u);
}

