/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/test_unit/test_u8g2Display/test_widgets_primitives.cpp
 * Description: GoogleTest unit-test suite for the widget primitives layer:
 *              IGfxWidget, TextAlign, and AppFonts. Validates
 *              construction, visibility, geometry, and enum value contracts.
 *              All tests execute on the native GoogleTest runner (NATIVE_BUILD).
 *
 * Exported Functions:
 * - (Linked into test_main.cpp GoogleTest entry point)
 */

#include <gtest/gtest.h>
#include "widgets/IGfxWidget.hpp"
#include "widgets/DrawingPrimitives.hpp"
#include "fonts/Fonts.hpp"

// =============================================================================
// Concrete test stubs — minimal implementations of abstract interfaces
// =============================================================================

/**
 * @brief Minimal concrete widget for testing IGfxWidget's non-virtual API.
 *        tick() and render() are trivial no-ops.
 */
class TestWidget : public IGfxWidget {
public:
    using IGfxWidget::IGfxWidget; // Inherit constructors

    void tick(uint32_t /*currentMillis*/) override {}
    void render(U8G2& /*display*/) override {}
};

// =============================================================================
// Test Group: IGfxWidget
// =============================================================================

class IGfxWidgetTest : public ::testing::Test {
protected:
    TestWidget widget{10, 20, 100, 50}; ///< Widget with explicit geometry
};

/** Default visibility must be true. */
TEST_F(IGfxWidgetTest, DefaultVisibility_IsTrue) {
    EXPECT_TRUE(widget.getVisible());
}

/** setVisible(false) must disable visibility. */
TEST_F(IGfxWidgetTest, SetVisibleFalse_DisablesVisibility) {
    widget.setVisible(false);
    EXPECT_FALSE(widget.getVisible());
}

/** setVisible(true) after false must re-enable visibility. */
TEST_F(IGfxWidgetTest, SetVisibleTrue_ReenablesVisibility) {
    widget.setVisible(false);
    widget.setVisible(true);
    EXPECT_TRUE(widget.getVisible());
}

/** Constructor must store x, y, w, h correctly. */
TEST_F(IGfxWidgetTest, Constructor_StoresGeometry) {
    EXPECT_EQ(widget.getX(), 10);
    EXPECT_EQ(widget.getY(), 20);
    EXPECT_EQ(widget.getW(), 100);
    EXPECT_EQ(widget.getH(), 50);
}

/** Default w and h must be -1 when not specified. */
TEST(IGfxWidgetDefaultsTest, DefaultWidthHeight_AreNegativeOne) {
    TestWidget w(5, 15);
    EXPECT_EQ(w.getW(), -1);
    EXPECT_EQ(w.getH(), -1);
}

/** setCoords must update all geometry values. */
TEST_F(IGfxWidgetTest, SetCoords_UpdatesGeometry) {
    widget.setCoords(30, 40, 200, 80);
    EXPECT_EQ(widget.getX(), 30);
    EXPECT_EQ(widget.getY(), 40);
    EXPECT_EQ(widget.getW(), 200);
    EXPECT_EQ(widget.getH(), 80);
}

/** setCoords with default w/h must reset them to -1. */
TEST_F(IGfxWidgetTest, SetCoords_DefaultWidthHeight) {
    widget.setCoords(0, 0);
    EXPECT_EQ(widget.getW(), -1);
    EXPECT_EQ(widget.getH(), -1);
}

/** renderAnimationUpdate default no-op must be callable without crashing. */
TEST_F(IGfxWidgetTest, RenderAnimationUpdate_DefaultNoOp_DoesNotCrash) {
    U8G2* nullDisplay = nullptr;
    // The default implementation is an empty body — should not dereference display
    EXPECT_NO_FATAL_FAILURE(widget.renderAnimationUpdate(*nullDisplay, 0));
}

// =============================================================================
// Test Group: IBoardLayout (REMOVED — IBoardLayout deleted per ADR-39/WO7)
// =============================================================================

// =============================================================================
// Test Group: TextAlign Enum
// =============================================================================

/** TextAlign::Left must equal 0. */
TEST(TextAlignTest, LEFT_EqualsZero) {
    EXPECT_EQ(static_cast<int>(TextAlign::Left), 0);
}

/** TextAlign::Center must equal 1. */
TEST(TextAlignTest, CENTER_EqualsOne) {
    EXPECT_EQ(static_cast<int>(TextAlign::Center), 1);
}

/** TextAlign::Right must equal 2. */
TEST(TextAlignTest, RIGHT_EqualsTwo) {
    EXPECT_EQ(static_cast<int>(TextAlign::Right), 2);
}

// =============================================================================
// Test Group: Font Management (NATIVE_BUILD)
// =============================================================================

/** In NATIVE_BUILD, all font accessors must return nullptr. */
TEST(AppFontsNativeTest, DefaultSmall_ReturnsNullptr) {
    EXPECT_EQ(AppFonts::defaultSmall(), nullptr);
}

TEST(AppFontsNativeTest, DefaultMedium_ReturnsNullptr) {
    EXPECT_EQ(AppFonts::defaultMedium(), nullptr);
}

TEST(AppFontsNativeTest, DefaultLarge_ReturnsNullptr) {
    EXPECT_EQ(AppFonts::defaultLarge(), nullptr);
}

TEST(AppFontsNativeTest, DefaultClock_ReturnsNullptr) {
    EXPECT_EQ(AppFonts::defaultClock(), nullptr);
}

TEST(AppFontsNativeTest, DefaultIcon_ReturnsNullptr) {
    EXPECT_EQ(AppFonts::defaultIcon(), nullptr);
}
