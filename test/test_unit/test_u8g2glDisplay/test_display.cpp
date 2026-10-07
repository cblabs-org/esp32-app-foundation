/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_u8g2glDisplay/test_display.cpp
 * Description: U8G2GLDisplay::begin() registers the runtime's time source and
 *              runs the generated ui_init() once. It must be idempotent,
 *              because ui_init() binds every label to its subject and a second
 *              call would register each binding again.
 *
 *              begin() sets process-wide state, so these tests cannot assume
 *              they run first: they handle both "already begun" and "not yet".
 *
 * Exported Functions:
 * - TEST(U8G2GLDisplayTest, ...): begin() and isReady() tests
 */

#include <gtest/gtest.h>

#include "ui.hpp"  // Generated: first of the U8G2GL headers, so its capacities are seen first
#include "U8G2GLDisplay.hpp"
#include <u8g2gl/u8g2gl_core.h>

namespace {

/** @brief A time source that is plainly not millis(), so a re-registration is visible. */
uint32_t fakeTick() { return 12345u; }

}  // namespace

/** begin() makes the runtime ready and registers millis() as the time source. */
TEST(U8G2GLDisplayTest, BeginRegistersTheTimeSourceAndBecomesReady) {
    const bool alreadyBegun = U8G2GLDisplay::isReady();
    u8g2gl_set_tick_cb(fakeTick);

    U8G2GLDisplay::begin();

    EXPECT_TRUE(U8G2GLDisplay::isReady());
    // First call: millis() replaces the fake (the native stub returns 0). Later calls change nothing.
    EXPECT_EQ(u8g2gl_tick_now(), alreadyBegun ? 12345u : 0u);
}

/** A second begin() does nothing: it neither re-registers the time source nor re-runs ui_init(). */
TEST(U8G2GLDisplayTest, SecondBeginIsANoOp) {
    U8G2GLDisplay::begin();
    ASSERT_TRUE(U8G2GLDisplay::isReady());

    u8g2gl_set_tick_cb(fakeTick);
    U8G2GLDisplay::begin();
    U8G2GLDisplay::begin();

    EXPECT_EQ(u8g2gl_tick_now(), 12345u);   // Still the fake: begin() did not register millis() again
    EXPECT_TRUE(U8G2GLDisplay::isReady());
}
