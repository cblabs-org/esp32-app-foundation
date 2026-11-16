/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
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
    EXPECT_EQ(Redactor::instance().registerSecret("mySecretPassword"), Redactor::RegisterResult::Registered);
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
    EXPECT_EQ(Redactor::instance().registerSecret(""), Redactor::RegisterResult::Invalid);
    EXPECT_EQ(Redactor::instance().registerSecret(std::string("")), Redactor::RegisterResult::Invalid);
    EXPECT_EQ(Redactor::instance().registerSecret(static_cast<const char*>(nullptr)), Redactor::RegisterResult::Invalid);
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
    EXPECT_EQ(Redactor::instance().registerSecret("duplicateSecret"), Redactor::RegisterResult::Registered);
    EXPECT_EQ(Redactor::instance().registerSecret("duplicateSecret"), Redactor::RegisterResult::AlreadyPresent);
    EXPECT_EQ(Redactor::instance().registerSecret(std::string("duplicateSecret")), Redactor::RegisterResult::AlreadyPresent);

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
        std::string s = "secret" + std::to_string(i + 1000); // Ensures length is >= 4
        EXPECT_EQ(Redactor::instance().registerSecret(s), Redactor::RegisterResult::Registered);
    }
    EXPECT_EQ(Redactor::instance().registerSecret("oneTooMany"), Redactor::RegisterResult::Full);
}

TEST_F(RedactorTest, SecretCountReturnsCorrectValue) {
    Redactor::instance().registerSecret("sec1");
    Redactor::instance().registerSecret("sec2");
    Redactor::instance().registerSecret("sec3");
    EXPECT_EQ(Redactor::instance().secretCount(), 3);
}

TEST_F(RedactorTest, ShortSecretRejected) {
    EXPECT_EQ(Redactor::instance().registerSecret("ab"), Redactor::RegisterResult::TooShort);
    EXPECT_EQ(Redactor::instance().secretCount(), 0);
}

TEST_F(RedactorTest, TooLongSecretRejected) {
    std::string longSecret(200, 'X');
    EXPECT_EQ(Redactor::instance().registerSecret(longSecret), Redactor::RegisterResult::TooLong);
    EXPECT_EQ(Redactor::instance().secretCount(), 0);
}

TEST_F(RedactorTest, DescendingScanOrder) {
    // Registering a short secret, then a longer secret that contains the short one
    EXPECT_EQ(Redactor::instance().registerSecret("pass"), Redactor::RegisterResult::Registered);
    EXPECT_EQ(Redactor::instance().registerSecret("password"), Redactor::RegisterResult::Registered);
    
    char buffer[128];
    strncpy(buffer, "I have a password", sizeof(buffer));
    // Should redact the whole 'password' to 8 stars, not '****word'
    Redactor::instance().redact(buffer, sizeof(buffer));
    EXPECT_STREQ(buffer, "I have a ********");
}

TEST_F(RedactorTest, UnterminatedBufferSafe) {
    Redactor::instance().registerSecret("secret");
    
    // Create an unterminated buffer
    char buffer[8] = {'s', 'e', 'c', 'r', 'e', 't', '1', '2'};
    
    // We expect it to redact only within the given bound (which acts as bufLen)
    Redactor::instance().redact(buffer, 8);
    EXPECT_EQ(memcmp(buffer, "******12", 8), 0);
}

TEST_F(RedactorTest, TailPrefixCheck) {
    Redactor::instance().registerSecret("mySecretToken");
    
    // Simulate truncation mid-secret
    char buffer[128];
    strncpy(buffer, "Auth: mySecre", sizeof(buffer));
    
    // With isTruncated=false (default), it shouldn't match partial
    Redactor::instance().redact(buffer, sizeof(buffer), false);
    EXPECT_STREQ(buffer, "Auth: mySecre");
    
    // With isTruncated=true, it should mask the partial secret at the tail
    Redactor::instance().redact(buffer, sizeof(buffer), true);
    EXPECT_STREQ(buffer, "Auth: *******");
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


TEST_F(RedactorTest, MaxLengthBoundaryIsInclusive) {
    // MAX_SECRET_LEN must be usable, not one short of it.
    EXPECT_EQ(Redactor::instance().registerSecret(std::string(Redactor::MAX_SECRET_LEN, 'q')),
              Redactor::RegisterResult::Registered);
    EXPECT_EQ(Redactor::instance().registerSecret(std::string(Redactor::MAX_SECRET_LEN + 1, 'r')),
              Redactor::RegisterResult::TooLong);
}

TEST_F(RedactorTest, UnterminatedBufferStaysInBounds) {
    Redactor::instance().registerSecret("LEAKME1234");
    char buf[16];
    memset(buf, 'X', sizeof(buf));          // deliberately no NUL anywhere
    memcpy(buf, "LEAKME1234", 10);
    EXPECT_NO_FATAL_FAILURE(Redactor::instance().redact(buf, sizeof(buf)));
    EXPECT_EQ(memcmp(buf, "**********", 10), 0);
    EXPECT_EQ(memcmp(buf + 10, "XXXXXX", 6), 0);   // nothing past the content was touched
}

TEST_F(RedactorTest, TailPrefixMaskedOnlyWhenTruncationIsDeclared) {
    Redactor::instance().registerSecret("TOPSECRETVALUE");
    char cut[16];
    snprintf(cut, sizeof(cut), "xxxxx%s", "TOPSECRETVALUE");   // cut mid-secret
    Redactor::instance().redact(cut, sizeof(cut), true);
    EXPECT_EQ(strstr(cut, "TOPSEC"), nullptr) << "truncated prefix survived";

    char cut2[16];
    snprintf(cut2, sizeof(cut2), "xxxxx%s", "TOPSECRETVALUE");
    Redactor::instance().redact(cut2, sizeof(cut2), false);
    EXPECT_NE(strstr(cut2, "TOPSEC"), nullptr) << "flag must gate the tail check";
}

TEST_F(RedactorTest, ScanOrderIndependentOfRegistrationOrder) {
    // Storage is append-only, so descending-length ordering has to be resolved
    // per redact() call rather than by sorting shared slots.
    Redactor::instance().registerSecret("abcd");
    Redactor::instance().registerSecret("abcdefghij");
    char a[64]; strncpy(a, "v=abcdefghij", sizeof(a));
    Redactor::instance().redact(a, sizeof(a));
    EXPECT_STREQ(a, "v=**********");

    Redactor::instance().clearSecrets();
    Redactor::instance().registerSecret("abcdefghij");
    Redactor::instance().registerSecret("abcd");
    char b[64]; strncpy(b, "v=abcdefghij", sizeof(b));
    Redactor::instance().redact(b, sizeof(b));
    EXPECT_STREQ(b, "v=**********");
}

TEST_F(RedactorTest, IsFullReflectsCapacity) {
    EXPECT_FALSE(Redactor::instance().isFull());
    for (size_t i = 0; i < Redactor::MAX_SECRETS; ++i) {
        Redactor::instance().registerSecret("secret" + std::to_string(i));
    }
    EXPECT_TRUE(Redactor::instance().isFull());
}

TEST_F(RedactorTest, DescribeCoversEveryResult) {
    using R = Redactor::RegisterResult;
    for (auto r : {R::Registered, R::AlreadyPresent, R::TooShort,
                   R::TooLong, R::Full, R::Invalid}) {
        ASSERT_NE(Redactor::describe(r), nullptr);
        EXPECT_GT(strlen(Redactor::describe(r)), 0u);
    }
}

TEST_F(RedactorTest, ClearSecretsWipesPlaintext) {
    Redactor::instance().registerSecret("wipeMeCompletely");
    Redactor::instance().clearSecrets();
    EXPECT_EQ(Redactor::instance().secretCount(), 0u);
    char buf[64]; strncpy(buf, "wipeMeCompletely", sizeof(buf));
    Redactor::instance().redact(buf, sizeof(buf));
    EXPECT_STREQ(buf, "wipeMeCompletely");   // no longer registered
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
