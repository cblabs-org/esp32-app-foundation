#include <gtest/gtest.h>
#include "Logger.hpp"
#include <string>

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
    }
};

TEST_F(LoggerTest, LogSecureWithoutSecrets) {
    testing::internal::CaptureStdout();
    Logger::logSecure("TEST", "Hello %s", "world");
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find("Hello world"), std::string::npos);
}

TEST_F(LoggerTest, RegisterSecret) {
    EXPECT_NO_FATAL_FAILURE(Logger::registerSecret("mypassword"));
}

TEST_F(LoggerTest, LogSecureWithSecretRedaction) {
    Logger::registerSecret("secret123");
    testing::internal::CaptureStdout();
    Logger::logSecure("TEST", "This is secret123!");
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output.find("secret123"), std::string::npos);
    EXPECT_NE(output.find("********"), std::string::npos);
}

TEST_F(LoggerTest, MultipleSecretsRegistered) {
    Logger::registerSecret("foo");
    Logger::registerSecret("bar");
    Logger::registerSecret("baz");
    
    testing::internal::CaptureStdout();
    Logger::logSecure("TEST", "foo bar baz");
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_EQ(output.find("foo"), std::string::npos);
    EXPECT_EQ(output.find("bar"), std::string::npos);
    EXPECT_EQ(output.find("baz"), std::string::npos);
}

TEST_F(LoggerTest, EmptySecret) {
    EXPECT_NO_FATAL_FAILURE(Logger::registerSecret(""));
}

TEST_F(LoggerTest, LogSecureWithFormatSpecifiers) {
    testing::internal::CaptureStdout();
    Logger::logSecure("TAG", "Value: %d, Name: %s", 42, "test");
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find("Value: 42, Name: test"), std::string::npos);
}

TEST_F(LoggerTest, LogSecureWithEmptyFormat) {
    testing::internal::CaptureStdout();
    Logger::logSecure("TAG", "");
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find("TAG"), std::string::npos);
}

TEST_F(LoggerTest, LogSecureWithLongMessage) {
    std::string longMsg(600, 'A');
    testing::internal::CaptureStdout();
    Logger::logSecure("TAG", "%s", longMsg.c_str());
    std::string output = testing::internal::GetCapturedStdout();
    // Verify it doesn't crash and outputs something
    EXPECT_NE(output.find("TAG"), std::string::npos);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
