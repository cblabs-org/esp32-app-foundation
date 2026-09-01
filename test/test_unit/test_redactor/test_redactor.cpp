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
        Redactor::instance().clearSecrets();
    }
};

TEST_F(RedactorTest, RedactWithoutSecrets) {
    char buffer[128];
    strncpy(buffer, "Hello world", sizeof(buffer));
    Redactor::instance().redact(buffer, sizeof(buffer));
    EXPECT_STREQ(buffer, "Hello world");
}

TEST_F(RedactorTest, RegisterSecretAndRedact) {
    Redactor::instance().registerSecret("mySecretPassword");
    char buffer[128];
    strncpy(buffer, "User pass is mySecretPassword!", sizeof(buffer));
    Redactor::instance().redact(buffer, sizeof(buffer));
    EXPECT_STREQ(buffer, "User pass is ****************!");
}

TEST_F(RedactorTest, MultipleSecretsRegistered) {
    Redactor::instance().registerSecret("alpha");
    Redactor::instance().registerSecret("beta");
    Redactor::instance().registerSecret(std::string("gamma"));
    Redactor::instance().registerSecret(std::string("delta"));

    char buffer[256];
    strncpy(buffer, "Secrets: alpha, beta, gamma, delta.", sizeof(buffer));
    Redactor::instance().redact(buffer, sizeof(buffer));
    EXPECT_STREQ(buffer, "Secrets: *****, ****, *****, *****.");
}

TEST_F(RedactorTest, EmptySecretIgnored) {
    EXPECT_FALSE(Redactor::instance().registerSecret(""));
    EXPECT_FALSE(Redactor::instance().registerSecret(std::string("")));
    EXPECT_FALSE(Redactor::instance().registerSecret(static_cast<const char*>(nullptr)));
}

TEST_F(RedactorTest, BufferLongerThanSecret) {
    Redactor::instance().registerSecret("token123");
    char buffer[1024];
    std::string prefix(400, 'A');
    std::string suffix(400, 'B');
    std::string full = prefix + "token123" + suffix;
    strncpy(buffer, full.c_str(), sizeof(buffer));

    Redactor::instance().redact(buffer, sizeof(buffer));
    std::string expected = prefix + "********" + suffix;
    EXPECT_STREQ(buffer, expected.c_str());
}

TEST_F(RedactorTest, DeduplicationDoesNotDuplicate) {
    EXPECT_TRUE(Redactor::instance().registerSecret("duplicateSecret"));
    EXPECT_TRUE(Redactor::instance().registerSecret("duplicateSecret")); // returns true if already registered
    EXPECT_TRUE(Redactor::instance().registerSecret(std::string("duplicateSecret")));

    EXPECT_EQ(Redactor::instance().secretCount(), 1);

    char buffer[128];
    strncpy(buffer, "Value: duplicateSecret", sizeof(buffer));
    Redactor::instance().redact(buffer, sizeof(buffer));
    EXPECT_STREQ(buffer, "Value: ***************");
}

TEST_F(RedactorTest, NullOrZeroLengthBufferSafe) {
    EXPECT_NO_FATAL_FAILURE(Redactor::instance().redact(nullptr, 0));
    EXPECT_NO_FATAL_FAILURE(Redactor::instance().redact(nullptr, 100));
    char buf[10] = "test";
    EXPECT_NO_FATAL_FAILURE(Redactor::instance().redact(buf, 0));
}

TEST_F(RedactorTest, MaxSecretsCapEnforced) {
    for (size_t i = 0; i < Redactor::MAX_SECRETS; ++i) {
        std::string s = "secret" + std::to_string(i);
        EXPECT_TRUE(Redactor::instance().registerSecret(s));
    }
    EXPECT_FALSE(Redactor::instance().registerSecret("oneTooMany"));
}

TEST_F(RedactorTest, SecretCountReturnsCorrectValue) {
    Redactor::instance().registerSecret("sec1");
    Redactor::instance().registerSecret("sec2");
    Redactor::instance().registerSecret("sec3");
    EXPECT_EQ(Redactor::instance().secretCount(), 3);
}

TEST_F(RedactorTest, ShortSecretRejected) {
    EXPECT_FALSE(Redactor::instance().registerSecret("ab"));
    EXPECT_EQ(Redactor::instance().secretCount(), 0);
}

TEST_F(RedactorTest, SingletonIdentity) {
    EXPECT_EQ(&Redactor::instance(), &Redactor::instance());
}

TEST_F(RedactorTest, ClearSecretsResetsCount) {
    Redactor::instance().registerSecret("secret1");
    Redactor::instance().registerSecret("secret2");
    EXPECT_EQ(Redactor::instance().secretCount(), 2);
    
    Redactor::instance().clearSecrets();
    EXPECT_EQ(Redactor::instance().secretCount(), 0);

    char buffer[128];
    strncpy(buffer, "User pass is secret1!", sizeof(buffer));
    Redactor::instance().redact(buffer, sizeof(buffer));
    EXPECT_STREQ(buffer, "User pass is secret1!");
}

TEST_F(RedactorTest, SanitizeInterfaceDelegates) {
    Redactor::instance().registerSecret("interfaceSecret");
    ISanitizer* sanitizer = &Redactor::instance();
    
    char buffer[128];
    strncpy(buffer, "Data: interfaceSecret...", sizeof(buffer));
    sanitizer->sanitize(buffer, sizeof(buffer));
    EXPECT_STREQ(buffer, "Data: ***************...");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
