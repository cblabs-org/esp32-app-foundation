/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Test: test/test_unit/test_logger/test_logger.cpp
 * Description: Implementation of LoggerTest class.
 */

#include <gtest/gtest.h>
#include "Logger.hpp"
#include <string>

#include "Redactor.h"
class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Redactor::instance().clearSecrets();
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
    Logger::registerSecret("fooo");
    Logger::registerSecret("barr");
    Logger::registerSecret("bazz");
    
    testing::internal::CaptureStdout();
    Logger::logSecure("TEST", "fooo barr bazz");
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_EQ(output.find("fooo"), std::string::npos);
    EXPECT_EQ(output.find("barr"), std::string::npos);
    EXPECT_EQ(output.find("bazz"), std::string::npos);
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
