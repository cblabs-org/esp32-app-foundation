#include <gtest/gtest.h>
#include "DeviceCrypto.hpp"
#include <string>
#include <cstring>

class MockDeviceCrypto : public DeviceCrypto {
public:
    std::unique_ptr<char[]> encrypt(const char* plaintext, size_t inputLen, size_t* outLen) override {
        if (!plaintext) {
            if (outLen) *outLen = 0;
            return nullptr;
        }
        if (inputLen == 0) {
            if (outLen) *outLen = 0;
            char* empty = new char[1];
            empty[0] = '\0';
            return std::unique_ptr<char[]>(empty);
        }
        std::string enc = "ENC:" + std::string(plaintext, inputLen);
        if (outLen) *outLen = enc.length();
        char* buf = new char[enc.length() + 1];
        strcpy(buf, enc.c_str());
        return std::unique_ptr<char[]>(buf);
    }

    std::unique_ptr<char[]> decrypt(const char* ciphertext, size_t inputLen, size_t* outLen) override {
        if (!ciphertext || inputLen == 0) {
            if (outLen) *outLen = 0;
            return nullptr;
        }
        std::string ct(ciphertext, inputLen);
        if (ct.find("ENC:") == 0) {
            std::string pt = ct.substr(4);
            if (outLen) *outLen = pt.length();
            char* buf = new char[pt.length() + 1];
            strcpy(buf, pt.c_str());
            return std::unique_ptr<char[]>(buf);
        }
        if (outLen) *outLen = 0;
        return nullptr;
    }
};

class DeviceCryptoTest : public ::testing::Test {
protected:
    MockDeviceCrypto crypto;
};

TEST_F(DeviceCryptoTest, Construction) {
    MockDeviceCrypto c;
}

TEST_F(DeviceCryptoTest, EncryptReturnsNonNull) {
    size_t outLen = 0;
    auto res = crypto.encrypt("hello", 5, &outLen);
    EXPECT_NE(res, nullptr);
    EXPECT_GT(outLen, 0);
}

TEST_F(DeviceCryptoTest, DecryptRoundTrip) {
    const char* original = "roundtrip-test";
    size_t origLen = strlen(original);
    
    size_t encLen = 0;
    auto enc = crypto.encrypt(original, origLen, &encLen);
    ASSERT_NE(enc, nullptr);
    
    size_t decLen = 0;
    auto dec = crypto.decrypt(enc.get(), encLen, &decLen);
    ASSERT_NE(dec, nullptr);
    
    EXPECT_EQ(decLen, origLen);
    EXPECT_EQ(std::string(dec.get(), decLen), std::string(original, origLen));
}

TEST_F(DeviceCryptoTest, EncryptEmptyString) {
    size_t outLen = 0;
    auto res = crypto.encrypt("", 0, &outLen);
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(outLen, 0);
}

TEST_F(DeviceCryptoTest, EncryptNullptr) {
    size_t outLen = 0;
    auto res = crypto.encrypt(nullptr, 0, &outLen);
    EXPECT_EQ(res, nullptr);
    EXPECT_EQ(outLen, 0);
}

TEST_F(DeviceCryptoTest, DecryptInvalidInput) {
    size_t outLen = 0;
    auto res = crypto.decrypt("not-valid", 9, &outLen);
    EXPECT_EQ(res, nullptr);
    EXPECT_EQ(outLen, 0);
}

TEST_F(DeviceCryptoTest, EncryptOutputLength) {
    size_t outLen = 0;
    crypto.encrypt("test", 4, &outLen);
    EXPECT_EQ(outLen, 8); // "ENC:test" is 8 chars
}

TEST_F(DeviceCryptoTest, MultipleEncryptCalls) {
    size_t outLen1 = 0;
    auto res1 = crypto.encrypt("one", 3, &outLen1);
    
    size_t outLen2 = 0;
    auto res2 = crypto.encrypt("two", 3, &outLen2);
    
    EXPECT_NE(res1, nullptr);
    EXPECT_NE(res2, nullptr);
    EXPECT_STRNE(res1.get(), res2.get());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
