#include <gtest/gtest.h>
#include "md5Utils.hpp"

class Md5UtilsTest : public ::testing::Test {
protected:
    Md5Utils md5;
};

TEST_F(Md5UtilsTest, Construction) {
    Md5Utils utils;
}

TEST_F(Md5UtilsTest, Base64ToHexKnownVector) {
    String base64 = "XUFAKrxLKna5cZ2REBfFkg==";
    String hex = md5.base64ToHex(base64);
    EXPECT_STREQ(hex.c_str(), "5d41402abc4b2a76b9719d911017c592");
}

TEST_F(Md5UtilsTest, Base64ToHexEmptyInput) {
    String hex = md5.base64ToHex("");
    EXPECT_STREQ(hex.c_str(), "");
}

TEST_F(Md5UtilsTest, Base64ToHexInvalidBase64) {
    String hex = md5.base64ToHex("!!!invalid!!!");
    EXPECT_STREQ(hex.c_str(), "");
}

TEST_F(Md5UtilsTest, CalculateFileMD5Nonexistent) {
    String hex = md5.calculateFileMD5("/nonexistent.txt");
    EXPECT_TRUE(hex.length() == 0 || hex.length() == 32);
}

TEST_F(Md5UtilsTest, MultipleBase64ToHexCalls) {
    String hex1 = md5.base64ToHex("XUFAKrxLKna5cZ2REBfFkg==");
    String hex2 = md5.base64ToHex("1B2M2Y8AsgTpgAmY7PhCfg==");
    EXPECT_STREQ(hex1.c_str(), "5d41402abc4b2a76b9719d911017c592");
    EXPECT_STREQ(hex2.c_str(), "d41d8cd98f00b204e9800998ecf8427e");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
