/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_u8g2Display/test_screensaver.cpp
 * Description: GoogleTest unit-test suite for the ScreensaverController's
 *              enable/disable API, independent dimTimeout and blankTimeout
 *              settings, and interaction with the state machine.
 *
 * Exported Functions:
 * - (GoogleTest fixtures — discovered automatically by gtest_main)
 */

#include <gtest/gtest.h>
#include "modules/U8G2Display/ScreensaverController.hpp"

// Pull in U8g2lib.h so we have the real U8G2 base class (the same one that
// ScreensaverController.cpp compiles against on native).  The test never
// drives real hardware — it only needs a valid U8G2& reference.
#include <U8g2lib.h>

// Instantiate the base U8G2 class for passing to tick().
// On native, constructors are no-ops; we never call begin().
static U8G2 stubDisplay;

// =============================================================================
// Test Group: ScreensaverController — Enable/Disable
// =============================================================================

class ScreensaverEnabledTest : public ::testing::Test {
protected:
    ScreensaverController sc; ///< Fresh controller for each test
};

/** Default state: screensaver is enabled. */
TEST_F(ScreensaverEnabledTest, DefaultIsEnabled) {
    EXPECT_TRUE(sc.isEnabled());
}

/** setEnabled(false) disables the screensaver. */
TEST_F(ScreensaverEnabledTest, SetEnabledFalse_Disables) {
    sc.setEnabled(false);
    EXPECT_FALSE(sc.isEnabled());
}

/** setEnabled(true) after disable re-enables the screensaver. */
TEST_F(ScreensaverEnabledTest, SetEnabledTrue_ReEnables) {
    sc.setEnabled(false);
    sc.setEnabled(true);
    EXPECT_TRUE(sc.isEnabled());
}

/** When disabled, tick() must NOT transition to dimmed state. */
TEST_F(ScreensaverEnabledTest, DisabledPreventsTransitionToDim) {
    sc.setDimTimeout(100);
    sc.setBlankTimeout(200);
    sc.setEnabled(false);

    // Simulate time well past the dim timeout
    sc.tick(500, stubDisplay, 255);

    EXPECT_FALSE(sc.isDimmed());
    EXPECT_FALSE(sc.isBlanked());
    EXPECT_FALSE(sc.isActive());
}

/** When disabled, tick() must NOT transition to blanked state. */
TEST_F(ScreensaverEnabledTest, DisabledPreventsTransitionToBlank) {
    sc.setDimTimeout(100);
    sc.setBlankTimeout(200);
    sc.setEnabled(false);

    // Simulate time well past both timeouts
    sc.tick(1000, stubDisplay, 255);

    EXPECT_FALSE(sc.isDimmed());
    EXPECT_FALSE(sc.isBlanked());
}

/** Re-enabling after disabling allows normal timeout behaviour. */
TEST_F(ScreensaverEnabledTest, ReEnableAllowsTimeoutBehaviour) {
    sc.setDimTimeout(100);
    sc.setBlankTimeout(200);
    sc.setEnabled(false);
    sc.setEnabled(true);

    // Simulate time past dim timeout
    sc.tick(150, stubDisplay, 255);

    EXPECT_TRUE(sc.isDimmed());
}

// =============================================================================
// Test Group: ScreensaverController — Disable while dimmed/blanked triggers wake
// =============================================================================

class ScreensaverDisableWhileActiveTest : public ::testing::Test {
protected:
    ScreensaverController sc; ///< Fresh controller for each test
};

/** Disabling while dimmed triggers a wake request (serviced on next tick). */
TEST_F(ScreensaverDisableWhileActiveTest, DisableWhileDimmed_RequestsWake) {
    sc.setDimTimeout(100);
    sc.setBlankTimeout(500);

    // Enter dim state
    sc.tick(200, stubDisplay, 255);
    ASSERT_TRUE(sc.isDimmed());

    // Disable — should request wake
    sc.setEnabled(false);

    // Next tick services the wake
    sc.tick(300, stubDisplay, 255);
    EXPECT_FALSE(sc.isDimmed());
    EXPECT_FALSE(sc.isBlanked());
    EXPECT_FALSE(sc.isActive());
}

/** Disabling while blanked triggers a wake request (serviced on next tick). */
TEST_F(ScreensaverDisableWhileActiveTest, DisableWhileBlanked_RequestsWake) {
    sc.setDimTimeout(100);
    sc.setBlankTimeout(200);
    sc.setPolicy(ScreensaverPolicy::FullSleep);

    // Enter dim then blank state
    sc.tick(150, stubDisplay, 255);
    ASSERT_TRUE(sc.isDimmed());
    sc.tick(300, stubDisplay, 255);
    ASSERT_TRUE(sc.isBlanked());

    // Disable — should request wake
    sc.setEnabled(false);

    // Next tick services the wake
    sc.tick(400, stubDisplay, 255);
    EXPECT_FALSE(sc.isDimmed());
    EXPECT_FALSE(sc.isBlanked());
}

// =============================================================================
// Test Group: ScreensaverController — Independent dimTimeout and blankTimeout
// =============================================================================

class ScreensaverTimeoutsTest : public ::testing::Test {
protected:
    ScreensaverController sc; ///< Fresh controller for each test
};

/** getDimTimeout returns the set value. */
TEST_F(ScreensaverTimeoutsTest, GetDimTimeout_ReturnsSetValue) {
    sc.setDimTimeout(12345);
    EXPECT_EQ(12345UL, sc.getDimTimeout());
}

/** getBlankTimeout returns the set value. */
TEST_F(ScreensaverTimeoutsTest, GetBlankTimeout_ReturnsSetValue) {
    sc.setBlankTimeout(67890);
    EXPECT_EQ(67890UL, sc.getBlankTimeout());
}

/** Independent timeouts: dim occurs at dimTimeout, blank at blankTimeout. */
TEST_F(ScreensaverTimeoutsTest, IndependentTimeouts_DimBeforeBlank) {
    sc.setDimTimeout(100);
    sc.setBlankTimeout(300);
    sc.setPolicy(ScreensaverPolicy::FullSleep);

    // Before dim timeout — still active
    sc.tick(50, stubDisplay, 255);
    EXPECT_FALSE(sc.isDimmed());
    EXPECT_FALSE(sc.isBlanked());

    // Past dim timeout, before blank — dimmed only
    sc.tick(150, stubDisplay, 255);
    EXPECT_TRUE(sc.isDimmed());
    EXPECT_FALSE(sc.isBlanked());

    // Past blank timeout — blanked
    sc.tick(350, stubDisplay, 255);
    EXPECT_TRUE(sc.isBlanked());
}

/** With DimOnly policy, blank never happens regardless of blankTimeout. */
TEST_F(ScreensaverTimeoutsTest, DimOnlyPolicy_NeverBlanks) {
    sc.setDimTimeout(100);
    sc.setBlankTimeout(200);
    sc.setPolicy(ScreensaverPolicy::DimOnly);

    sc.tick(150, stubDisplay, 255);
    EXPECT_TRUE(sc.isDimmed());

    sc.tick(500, stubDisplay, 255);
    EXPECT_TRUE(sc.isDimmed());
    EXPECT_FALSE(sc.isBlanked());
}

/** dimTimeout can be set independently from blankTimeout. */
TEST_F(ScreensaverTimeoutsTest, DimTimeout_IndependentFromBlank) {
    sc.setDimTimeout(50);
    sc.setBlankTimeout(10000);

    sc.tick(60, stubDisplay, 255);
    EXPECT_TRUE(sc.isDimmed());

    // Blank should NOT have triggered
    EXPECT_FALSE(sc.isBlanked());
}
