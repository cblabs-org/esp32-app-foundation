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
 *              iGfxWidget, iBoardLayout, TextAlign, and AppFonts. Validates
 *              construction, visibility, geometry, and enum value contracts.
 *              All tests execute on the native GoogleTest runner (NATIVE_BUILD).
 *
 * Exported Functions:
 * - (Linked into test_main.cpp GoogleTest entry point)
 */

#include <gtest/gtest.h>
#include "widgets/iGfxWidget.hpp"
#include "widgets/drawingPrimitives.hpp"
#include "boards/interfaces/iBoardLayout.hpp"
#include "fonts/fonts.hpp"

// =============================================================================
// Concrete test stubs — minimal implementations of abstract interfaces
// =============================================================================

/**
 * @brief Minimal concrete widget for testing iGfxWidget's non-virtual API.
 *        tick() and render() are trivial no-ops.
 */
class TestWidget : public iGfxWidget {
public:
    using iGfxWidget::iGfxWidget; // Inherit constructors

    void tick(uint32_t /*currentMillis*/) override {}
    void render(U8G2& /*display*/) override {}
};

/**
 * @brief Minimal concrete layout for testing iBoardLayout's non-virtual API.
 *        tick() and render() are trivial no-ops.
 */
class TestLayout : public iBoardLayout {
public:
    using iBoardLayout::iBoardLayout; // Inherit constructors

    void tick(uint32_t /*currentMillis*/) override {}
    void render(U8G2& /*display*/) override {}
};

// =============================================================================
// Test Group: iGfxWidget
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
// Test Group: iBoardLayout
// =============================================================================

class IBoardLayoutTest : public ::testing::Test {
protected:
    TestLayout layout{nullptr}; ///< Layout with nullptr Application (valid in tests)
};

/** A concrete layout subclass can be instantiated with nullptr Application. */
TEST_F(IBoardLayoutTest, Construction_WithNullApp_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(TestLayout testLayout(nullptr));
}

/** renderAnimationUpdate default no-op must be callable without crashing. */
TEST_F(IBoardLayoutTest, RenderAnimationUpdate_DefaultNoOp_DoesNotCrash) {
    U8G2* nullDisplay = nullptr;
    // The default implementation is an empty body — should not dereference display
    EXPECT_NO_FATAL_FAILURE(layout.renderAnimationUpdate(*nullDisplay, 0));
}

// =============================================================================
// Test Group: TextAlign Enum
// =============================================================================

/** TextAlign::LEFT must equal 0. */
TEST(TextAlignTest, LEFT_EqualsZero) {
    EXPECT_EQ(static_cast<int>(TextAlign::LEFT), 0);
}

/** TextAlign::CENTER must equal 1. */
TEST(TextAlignTest, CENTER_EqualsOne) {
    EXPECT_EQ(static_cast<int>(TextAlign::CENTER), 1);
}

/** TextAlign::RIGHT must equal 2. */
TEST(TextAlignTest, RIGHT_EqualsTwo) {
    EXPECT_EQ(static_cast<int>(TextAlign::RIGHT), 2);
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
