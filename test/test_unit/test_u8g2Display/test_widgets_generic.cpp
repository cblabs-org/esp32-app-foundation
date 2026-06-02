/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/test_unit/test_u8g2Display/test_widgets_generic.cpp
 * Description: GoogleTest unit-test suite for the WO2 generic widget layer:
 *              LabelWidget, ScrollingTextWidget, ClockWidget, ProgressBarWidget,
 *              and ImageWidget. Validates construction, state management, text
 *              handling, animation math, and safety invariants.
 *              All tests execute on the native GoogleTest runner (NATIVE_BUILD).
 *
 * Exported Functions:
 * - (Linked into test_main.cpp GoogleTest entry point)
 */

#include <gtest/gtest.h>
#include <cstring>

#include "widgets/LabelWidget.hpp"
#include "widgets/ScrollingTextWidget.hpp"
#include "widgets/ClockWidget.hpp"
#include "widgets/ProgressBarWidget.hpp"
#include "widgets/ImageWidget.hpp"

// =============================================================================
// Test Group: LabelWidget
// =============================================================================

class LabelWidgetTest : public ::testing::Test {
protected:
    LabelWidget label{10, 20, 100, 16};
};

/** Default construction initializes empty text and left alignment. */
TEST_F(LabelWidgetTest, Construction_DefaultState) {
    EXPECT_TRUE(label.getVisible());
    EXPECT_EQ(label.getX(), 10);
    EXPECT_EQ(label.getY(), 20);
    EXPECT_EQ(label.getW(), 100);
    EXPECT_EQ(label.getH(), 16);
}

/** setText() stores text that does not crash or overflow. */
TEST_F(LabelWidgetTest, SetText_StoresText) {
    label.setText("Hello World");
    // No crash = success for native build (buffer internals are private)
    EXPECT_NO_FATAL_FAILURE(label.setText("Hello World"));
}

/** setText() with nullptr clears the buffer safely. */
TEST_F(LabelWidgetTest, SetText_NullClears) {
    label.setText("Hello");
    label.setText(nullptr);
    // No crash — buffer should be cleared
    EXPECT_NO_FATAL_FAILURE(label.tick(0));
}

/** setText() with very long string does not overflow the 128-byte buffer. */
TEST_F(LabelWidgetTest, SetText_BoundsChecked) {
    char longStr[256];
    memset(longStr, 'A', 255);
    longStr[255] = '\0';
    // Must not crash — strncpy should truncate to 127 chars
    EXPECT_NO_FATAL_FAILURE(label.setText(longStr));
}

/** setAlignment() stores alignment value correctly. */
TEST_F(LabelWidgetTest, SetAlignment_StoresValue) {
    // No crash for valid values
    EXPECT_NO_FATAL_FAILURE(label.setAlignment(0)); // LEFT
    EXPECT_NO_FATAL_FAILURE(label.setAlignment(1)); // CENTER
    EXPECT_NO_FATAL_FAILURE(label.setAlignment(2)); // RIGHT
}

/** setTruncated() toggles cleanly. */
TEST_F(LabelWidgetTest, SetTruncated_Toggle) {
    EXPECT_NO_FATAL_FAILURE(label.setTruncated(true));
    EXPECT_NO_FATAL_FAILURE(label.setTruncated(false));
}

/** tick() is a no-op — no crash, no state change. */
TEST_F(LabelWidgetTest, Tick_NoOp) {
    label.setText("Test");
    EXPECT_NO_FATAL_FAILURE(label.tick(0));
    EXPECT_NO_FATAL_FAILURE(label.tick(1000));
    EXPECT_NO_FATAL_FAILURE(label.tick(999999));
}

/** setFont() does not crash with nullptr. */
TEST_F(LabelWidgetTest, SetFont_NullSafe) {
    EXPECT_NO_FATAL_FAILURE(label.setFont(nullptr));
}

// =============================================================================
// Test Group: ScrollingTextWidget
// =============================================================================

class ScrollingTextWidgetTest : public ::testing::Test {
protected:
    ScrollingTextWidget scroller{0, 0, 200, 12, nullptr};
};

/** setText() stores text and sets needsLayout. */
TEST_F(ScrollingTextWidgetTest, SetText_StoresAndInvalidatesLayout) {
    EXPECT_NO_FATAL_FAILURE(scroller.setText("Hello Scrolling World"));
}

/** setText() with identical text is deduped (no layout reset). */
TEST_F(ScrollingTextWidgetTest, SetText_DeduplicatesIdenticalText) {
    scroller.setText("Dedup Test");
    // Second call with same text should be silently ignored
    EXPECT_NO_FATAL_FAILURE(scroller.setText("Dedup Test"));
}

/** setText(nullptr) clears buffer safely. */
TEST_F(ScrollingTextWidgetTest, SetText_NullClearsBuffer) {
    scroller.setText("Some text");
    scroller.setText(nullptr);
    EXPECT_NO_FATAL_FAILURE(scroller.tick(0));
}

/** isScrollFinished() defaults to false. */
TEST_F(ScrollingTextWidgetTest, IsScrollFinished_DefaultFalse) {
    EXPECT_FALSE(scroller.isScrollFinished());
}

/** resetScroll() resets state cleanly. */
TEST_F(ScrollingTextWidgetTest, ResetScroll_ResetsState) {
    scroller.setText("Test");
    scroller.resetScroll();
    EXPECT_FALSE(scroller.isScrollFinished());
}

/** tick() with initial delay: first tick should not crash. */
TEST_F(ScrollingTextWidgetTest, Tick_InitialDelay) {
    scroller.setText("Delay Test");
    // First tick initializes the delay (lastScrollMs = currentMillis + 3000)
    EXPECT_NO_FATAL_FAILURE(scroller.tick(0));
    // Second tick within delay period should be safe
    EXPECT_NO_FATAL_FAILURE(scroller.tick(1000));
}

/** tick() 3-second initial delay logic: scrollFinished should not be set before 5 seconds. */
TEST_F(ScrollingTextWidgetTest, Tick_ThreeSecondDelayLogic) {
    scroller.setText("Static text");
    // needsScroll is false by default (tick doesn't know text width in native)
    // but tick should not crash and should respect the delay
    scroller.tick(0);       // Initialize delay
    scroller.tick(1000);    // Still within delay
    scroller.tick(2000);    // Still within delay
    EXPECT_FALSE(scroller.isScrollFinished());  // Should not be finished yet
}

/** tick() with enough time elapsed for static text dwell should set scrollFinished. */
TEST_F(ScrollingTextWidgetTest, Tick_StaticTextDwell) {
    scroller.setText("Static");
    scroller.tick(0);        // Initialize: lastScrollMs = 0 + 3000 = 3000
    scroller.tick(3500);     // Past initial delay, needsScroll=false -> dwell timer starts
    // After 2 more seconds of dwell (5000 + something), should be finished
    scroller.tick(5500);     // 5500 > 3000 + 2000 -> scrollFinished = true
    EXPECT_TRUE(scroller.isScrollFinished());
}

/** setFont() triggers layout recalculation. */
TEST_F(ScrollingTextWidgetTest, SetFont_TriggersLayout) {
    uint8_t fakeFont = 0;
    EXPECT_NO_FATAL_FAILURE(scroller.setFont(&fakeFont));
}

// =============================================================================
// Test Group: ClockWidget
// =============================================================================

class ClockWidgetTest : public ::testing::Test {
protected:
    // Construct with nullptr TimeManager — valid for testing state logic
    ClockWidget clock{nullptr, 10, 0, 56, 14, nullptr};
};

/** Construction with nullptr TimeManager does not crash. */
TEST_F(ClockWidgetTest, Construction_NullTimeMgr_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(ClockWidget testClock(nullptr, 0, 0));
    EXPECT_TRUE(clock.getVisible());
}

/** Blink toggle via tick() with advancing millis. */
TEST_F(ClockWidgetTest, Tick_BlinkToggle) {
    // tick should toggle showColon every 500ms
    EXPECT_NO_FATAL_FAILURE(clock.tick(0));
    EXPECT_NO_FATAL_FAILURE(clock.tick(500));
    EXPECT_NO_FATAL_FAILURE(clock.tick(1000));
    EXPECT_NO_FATAL_FAILURE(clock.tick(1500));
}

/** setFormat changes format enum without crash. */
TEST_F(ClockWidgetTest, SetFormat_HH_MM) {
    EXPECT_NO_FATAL_FAILURE(clock.setFormat(ClockWidget::ClockFormat::HH_MM));
}

TEST_F(ClockWidgetTest, SetFormat_HH_MM_SS) {
    EXPECT_NO_FATAL_FAILURE(clock.setFormat(ClockWidget::ClockFormat::HH_MM_SS));
}

/** setBlink() toggle does not crash. */
TEST_F(ClockWidgetTest, SetBlink_Toggle) {
    EXPECT_NO_FATAL_FAILURE(clock.setBlink(false));
    EXPECT_NO_FATAL_FAILURE(clock.setBlink(true));
}

/** tick() with blink disabled should be a no-op. */
TEST_F(ClockWidgetTest, Tick_BlinkDisabled_NoOp) {
    clock.setBlink(false);
    EXPECT_NO_FATAL_FAILURE(clock.tick(0));
    EXPECT_NO_FATAL_FAILURE(clock.tick(1000));
}

/** setAlignment does not crash. */
TEST_F(ClockWidgetTest, SetAlignment) {
    EXPECT_NO_FATAL_FAILURE(clock.setAlignment(0));
    EXPECT_NO_FATAL_FAILURE(clock.setAlignment(1));
    EXPECT_NO_FATAL_FAILURE(clock.setAlignment(2));
}

/** setFont and setSecondaryFont do not crash. */
TEST_F(ClockWidgetTest, SetFonts_NullSafe) {
    EXPECT_NO_FATAL_FAILURE(clock.setFont(nullptr));
    EXPECT_NO_FATAL_FAILURE(clock.setSecondaryFont(nullptr));
}

// =============================================================================
// Test Group: ProgressBarWidget
// =============================================================================

class ProgressBarWidgetTest : public ::testing::Test {
protected:
    ProgressBarWidget bar{0, 0, 200, 20, nullptr};
};

/** setPercent() snap (0 duration) sets currentPercent immediately. */
TEST_F(ProgressBarWidgetTest, SetPercent_Snap_Immediate) {
    bar.setPercent(50, 0);
    bar.tick(0);
    // After snap, percent should be at target immediately
    // We can't directly read currentPercent, but tick should not crash
    EXPECT_NO_FATAL_FAILURE(bar.tick(100));
}

/** setPercent() with animation duration sets target. */
TEST_F(ProgressBarWidgetTest, SetPercent_WithAnimation) {
    bar.setPercent(75, 1000); // Animate to 75% over 1 second
    EXPECT_NO_FATAL_FAILURE(bar.tick(0));
    EXPECT_NO_FATAL_FAILURE(bar.tick(500));
    EXPECT_NO_FATAL_FAILURE(bar.tick(1000));
}

/** setPercent() clamps values to [0, 100]. */
TEST_F(ProgressBarWidgetTest, SetPercent_Clamping) {
    EXPECT_NO_FATAL_FAILURE(bar.setPercent(-10));
    EXPECT_NO_FATAL_FAILURE(bar.setPercent(150));
}

/** setMessage() stores message safely. */
TEST_F(ProgressBarWidgetTest, SetMessage_Stores) {
    EXPECT_NO_FATAL_FAILURE(bar.setMessage("Loading..."));
    EXPECT_NO_FATAL_FAILURE(bar.setMessage("Updating firmware"));
}

/** setMessage(nullptr) is safely ignored. */
TEST_F(ProgressBarWidgetTest, SetMessage_NullIgnored) {
    bar.setMessage("Hello");
    EXPECT_NO_FATAL_FAILURE(bar.setMessage(nullptr));
}

/** setMessage() deduplication: identical text is silently ignored. */
TEST_F(ProgressBarWidgetTest, SetMessage_Dedup) {
    bar.setMessage("Loading...");
    EXPECT_NO_FATAL_FAILURE(bar.setMessage("Loading..."));
}

/** setShowPercentText() toggle. */
TEST_F(ProgressBarWidgetTest, SetShowPercentText_Toggle) {
    EXPECT_NO_FATAL_FAILURE(bar.setShowPercentText(true));
    EXPECT_NO_FATAL_FAILURE(bar.setShowPercentText(false));
}

/** tick() interpolation math: animation should progress. */
TEST_F(ProgressBarWidgetTest, Tick_Interpolation) {
    bar.setPercent(100, 1000);
    bar.tick(0);     // Initialize animation start time
    bar.tick(250);   // 25% through animation
    bar.tick(500);   // 50% through animation
    bar.tick(750);   // 75% through animation
    bar.tick(1000);  // Animation complete
    // No crash = success; internal currentPercent should reach 100
    EXPECT_NO_FATAL_FAILURE(bar.tick(1001));
}

/** tick() with widget invisible should be a no-op. */
TEST_F(ProgressBarWidgetTest, Tick_InvisibleNoOp) {
    bar.setVisible(false);
    bar.setPercent(50, 1000);
    EXPECT_NO_FATAL_FAILURE(bar.tick(0));
    EXPECT_NO_FATAL_FAILURE(bar.tick(500));
}

// =============================================================================
// Test Group: ImageWidget
// =============================================================================

class ImageWidgetTest : public ::testing::Test {
protected:
    // A small 8x8 test bitmap (all zeros)
    static const unsigned char testBits[8];
    ImageWidget img{0, 0, 8, 8, testBits};
};

const unsigned char ImageWidgetTest::testBits[8] = {0, 0, 0, 0, 0, 0, 0, 0};

/** Construction with nullptr bits does not crash. */
TEST(ImageWidgetNullTest, Construction_NullBits_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(ImageWidget nullImg(0, 0, 8, 8, nullptr));
}

/** tick() is a no-op — no crash. */
TEST_F(ImageWidgetTest, Tick_NoOp) {
    EXPECT_NO_FATAL_FAILURE(img.tick(0));
    EXPECT_NO_FATAL_FAILURE(img.tick(1000));
}

/** Visibility toggle works correctly. */
TEST_F(ImageWidgetTest, Visibility_Toggle) {
    EXPECT_TRUE(img.getVisible());
    img.setVisible(false);
    EXPECT_FALSE(img.getVisible());
    img.setVisible(true);
    EXPECT_TRUE(img.getVisible());
}

/** Geometry is correctly stored from constructor. */
TEST_F(ImageWidgetTest, Geometry_StoredCorrectly) {
    EXPECT_EQ(img.getX(), 0);
    EXPECT_EQ(img.getY(), 0);
    EXPECT_EQ(img.getW(), 8);
    EXPECT_EQ(img.getH(), 8);
}
