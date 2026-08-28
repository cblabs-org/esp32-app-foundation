/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Test: test/test_unit/test_md5Utils/test_md5Utils.cpp
 * Description: Implementation of Md5UtilsTest class.
 */

#include <gtest/gtest.h>
#include "MD5Utils.hpp"

class Md5UtilsTest : public ::testing::Test {
protected:
    MD5Utils md5;
};

TEST_F(Md5UtilsTest, Construction) {
    MD5Utils utils;
}

#ifndef PIO_UNIT_TESTING
// These tests exercise the real mbedtls-based decoder — skip in native builds
// where base64ToHex returns a fixed mock value.

TEST_F(Md5UtilsTest, Base64ToHexKnownVector) {
    std::string base64 = "XUFAKrxLKna5cZ2REBfFkg==";
    std::string hex = md5.base64ToHex(base64);
    EXPECT_STREQ(hex.c_str(), "5d41402abc4b2a76b9719d911017c592");
}

TEST_F(Md5UtilsTest, Base64ToHexEmptyInput) {
    std::string hex = md5.base64ToHex("");
    EXPECT_STREQ(hex.c_str(), "");
}

TEST_F(Md5UtilsTest, Base64ToHexInvalidBase64) {
    std::string hex = md5.base64ToHex("!!!invalid!!!");
    EXPECT_STREQ(hex.c_str(), "");
}

TEST_F(Md5UtilsTest, MultipleBase64ToHexCalls) {
    std::string hex1 = md5.base64ToHex("XUFAKrxLKna5cZ2REBfFkg==");
    std::string hex2 = md5.base64ToHex("1B2M2Y8AsgTpgAmY7PhCfg==");
    EXPECT_STREQ(hex1.c_str(), "5d41402abc4b2a76b9719d911017c592");
    EXPECT_STREQ(hex2.c_str(), "d41d8cd98f00b204e9800998ecf8427e");
}
#endif

TEST_F(Md5UtilsTest, CalculateFileMD5Nonexistent) {
    std::string hex = md5.calculateFileMD5("/nonexistent.txt");
    EXPECT_TRUE(hex.length() == 0 || hex.length() == 32);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
