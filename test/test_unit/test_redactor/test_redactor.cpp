/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test: test/test_unit/test_redactor/test_redactor.cpp
 * Description: GoogleTest suite for Redactor class secret masking and registration.
 */

#include <gtest/gtest.h>
#include "Redactor.h"
#include <string>
#include <cstring>

class RedactorTest : public ::testing::Test {
protected:
    void SetUp() override {
    }
};

TEST_F(RedactorTest, RedactWithoutSecrets) {
    char buffer[128];
    strncpy(buffer, "Hello world", sizeof(buffer));
    Redactor::redact(buffer, sizeof(buffer));
    EXPECT_STREQ(buffer, "Hello world");
}

TEST_F(RedactorTest, RegisterSecretAndRedact) {
    Redactor::registerSecret("mySecretPassword");
    char buffer[128];
    strncpy(buffer, "User pass is mySecretPassword!", sizeof(buffer));
    Redactor::redact(buffer, sizeof(buffer));
    EXPECT_STREQ(buffer, "User pass is ****************!");
}

TEST_F(RedactorTest, MultipleSecretsRegistered) {
    Redactor::registerSecret("alpha");
    Redactor::registerSecret("beta");
    Redactor::registerSecret(String("gamma"));
    Redactor::registerSecret(std::string("delta"));

    char buffer[256];
    strncpy(buffer, "Secrets: alpha, beta, gamma, delta.", sizeof(buffer));
    Redactor::redact(buffer, sizeof(buffer));
    EXPECT_STREQ(buffer, "Secrets: *****, ****, *****, *****.");
}

TEST_F(RedactorTest, EmptySecretIgnored) {
    EXPECT_NO_FATAL_FAILURE(Redactor::registerSecret(""));
    EXPECT_NO_FATAL_FAILURE(Redactor::registerSecret(String("")));
    EXPECT_NO_FATAL_FAILURE(Redactor::registerSecret(std::string("")));
    EXPECT_NO_FATAL_FAILURE(Redactor::registerSecret(static_cast<const char*>(nullptr)));
}

TEST_F(RedactorTest, BufferLongerThanSecret) {
    Redactor::registerSecret("token123");
    char buffer[1024];
    std::string prefix(400, 'A');
    std::string suffix(400, 'B');
    std::string full = prefix + "token123" + suffix;
    strncpy(buffer, full.c_str(), sizeof(buffer));

    Redactor::redact(buffer, sizeof(buffer));
    std::string expected = prefix + "********" + suffix;
    EXPECT_STREQ(buffer, expected.c_str());
}

TEST_F(RedactorTest, DeduplicationDoesNotDuplicate) {
    Redactor::registerSecret("duplicateSecret");
    Redactor::registerSecret("duplicateSecret");
    Redactor::registerSecret(String("duplicateSecret"));
    Redactor::registerSecret(std::string("duplicateSecret"));

    char buffer[128];
    strncpy(buffer, "Value: duplicateSecret", sizeof(buffer));
    Redactor::redact(buffer, sizeof(buffer));
    EXPECT_STREQ(buffer, "Value: ***************");
}

TEST_F(RedactorTest, NullOrZeroLengthBufferSafe) {
    EXPECT_NO_FATAL_FAILURE(Redactor::redact(nullptr, 0));
    EXPECT_NO_FATAL_FAILURE(Redactor::redact(nullptr, 100));
    char buf[10] = "test";
    EXPECT_NO_FATAL_FAILURE(Redactor::redact(buf, 0));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
