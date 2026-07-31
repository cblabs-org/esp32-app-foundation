/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Test: test/test_unit/test_hardware_board/test_hardware_board.cpp
 * Description: Native GoogleTest suite verifying hardware facade, null board, and contract compilation.
 */

#include <gtest/gtest.h>
#include "board.h"

class HardwareBoardTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(HardwareBoardTest, NullBoardFacadeLoadsSuccessfully) {
    // Verify PIN_UNASSIGNED constant from board_pins.h
    EXPECT_EQ(PIN_UNASSIGNED, -1);

    // Verify PIN_IS_VALID macro
    EXPECT_FALSE(PIN_IS_VALID(PIN_UNASSIGNED));
    EXPECT_TRUE(PIN_IS_VALID(0));

    // Verify null board assigns display pins to PIN_UNASSIGNED
    EXPECT_EQ(PIN_DISPLAY_CS, PIN_UNASSIGNED);
    EXPECT_EQ(PIN_DISPLAY_DC, PIN_UNASSIGNED);
    EXPECT_EQ(PIN_DISPLAY_SCK, PIN_UNASSIGNED);
    EXPECT_EQ(PIN_DISPLAY_MISO, PIN_UNASSIGNED);
    EXPECT_EQ(PIN_DISPLAY_MOSI, PIN_UNASSIGNED);

    // Verify BOARD_NAME definition
#ifdef BOARD_NAME
    EXPECT_STREQ(BOARD_NAME, "Host Native (Null Board)");
#else
    FAIL() << "BOARD_NAME macro is not defined";
#endif
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
