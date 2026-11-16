/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Test: test/test_unit/test_deviceCrypto/test_deviceCrypto.cpp
 * Description: Implementation of MockDeviceCrypto, DeviceCryptoTest classes.
 */

#include <gtest/gtest.h>
#include "DeviceCrypto.hpp"
#include <mbedtls/base64.h>
#include <string>
#include <vector>
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

class RealDeviceCryptoTest : public ::testing::Test {
protected:
    DeviceCrypto crypto;

    void SetUp() override {
        crypto.begin();
    }

    static std::vector<uint8_t> decodeBase64(const char* b64, size_t len) {
        size_t decLen = 0;
        int ret = mbedtls_base64_decode(nullptr, 0, &decLen,
                                        reinterpret_cast<const unsigned char*>(b64), len);
        if (ret != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL || decLen == 0) {
            return {};
        }
        std::vector<uint8_t> out(decLen);
        size_t actualLen = 0;
        ret = mbedtls_base64_decode(out.data(), out.size(), &actualLen,
                                    reinterpret_cast<const unsigned char*>(b64), len);
        if (ret != 0) {
            return {};
        }
        out.resize(actualLen);
        return out;
    }

    static std::string encodeBase64(const std::vector<uint8_t>& raw) {
        size_t b64Len = 0;
        mbedtls_base64_encode(nullptr, 0, &b64Len, raw.data(), raw.size());
        if (b64Len == 0) {
            return {};
        }
        std::vector<unsigned char> out(b64Len + 1);
        size_t actualLen = 0;
        int ret = mbedtls_base64_encode(out.data(), out.size(), &actualLen,
                                        raw.data(), raw.size());
        if (ret != 0) {
            return {};
        }
        return std::string(reinterpret_cast<const char*>(out.data()), actualLen);
    }
};

TEST_F(RealDeviceCryptoTest, UninitializedGuardAndEmptyInput) {
    DeviceCrypto uninit;
    size_t outLen = 123;
    EXPECT_EQ(uninit.encrypt("secret", 6, &outLen), nullptr);
    EXPECT_EQ(outLen, 0u);

    outLen = 123;
    EXPECT_EQ(uninit.decrypt("AAAAAAAAAAAAAAAAAAAAAA==", 24, &outLen), nullptr);
    EXPECT_EQ(outLen, 0u);

    outLen = 123;
    EXPECT_EQ(crypto.encrypt("", 0, &outLen), nullptr);
    EXPECT_EQ(outLen, 0u);

    outLen = 123;
    EXPECT_EQ(crypto.decrypt("", 0, &outLen), nullptr);
    EXPECT_EQ(outLen, 0u);
}

TEST_F(RealDeviceCryptoTest, RoundTripBoundaryLengthsAndFreshIV) {
    const size_t lengths[] = {1, 10, 15, 16, 17, 32};
    for (size_t len : lengths) {
        std::string ascii(len, '\0');
        std::string binary(len, '\0');
        for (size_t i = 0; i < len; ++i) {
            ascii[i] = static_cast<char>('A' + (i % 26));
            binary[i] = static_cast<char>((i == 0) ? 0x00 : ((i * 37 + 0x80) & 0xFF));
        }

        const std::string* variants[] = {&ascii, &binary};
        for (const std::string* payload : variants) {
            size_t encLen1 = 0;
            auto enc1 = crypto.encrypt(payload->data(), payload->size(), &encLen1);
            ASSERT_NE(enc1, nullptr);
            ASSERT_GT(encLen1, 0u);

            size_t encLen2 = 0;
            auto enc2 = crypto.encrypt(payload->data(), payload->size(), &encLen2);
            ASSERT_NE(enc2, nullptr);
            ASSERT_GT(encLen2, 0u);

            EXPECT_NE(std::string(enc1.get(), encLen1), std::string(enc2.get(), encLen2));

            size_t decLen1 = 0;
            auto dec1 = crypto.decrypt(enc1.get(), encLen1, &decLen1);
            ASSERT_NE(dec1, nullptr);
            EXPECT_EQ(decLen1, payload->size());
            EXPECT_EQ(std::memcmp(dec1.get(), payload->data(), payload->size()), 0);

            size_t decLen2 = 0;
            auto dec2 = crypto.decrypt(enc2.get(), encLen2, &decLen2);
            ASSERT_NE(dec2, nullptr);
            EXPECT_EQ(decLen2, payload->size());
            EXPECT_EQ(std::memcmp(dec2.get(), payload->data(), payload->size()), 0);
        }
    }
}

TEST_F(RealDeviceCryptoTest, RejectsInvalidPKCS7PaddingMismatch_CWE354) {
    const char* plaintext = "12345678901"; // 11 bytes -> 5 bytes of 0x05 PKCS#7 padding
    size_t encLen = 0;
    auto enc = crypto.encrypt(plaintext, 11, &encLen);
    ASSERT_NE(enc, nullptr);

    std::vector<uint8_t> raw = decodeBase64(enc.get(), encLen);
    ASSERT_EQ(raw.size(), 32u); // 16-byte IV + 16-byte ciphertext block

    // Mutate IV[11] so CBC decryption yields [..., 0x99, 0x05, 0x05, 0x05, 0x05]
    raw[11] ^= static_cast<uint8_t>(0x05 ^ 0x99);
    std::string tampered = encodeBase64(raw);
    ASSERT_FALSE(tampered.empty());

    size_t decLen = 999;
    auto dec = crypto.decrypt(tampered.c_str(), tampered.size(), &decLen);
    EXPECT_EQ(dec, nullptr);
    EXPECT_EQ(decLen, 0u);
}

TEST_F(RealDeviceCryptoTest, RejectsOutOfRangePKCS7Padding) {
    const char* plaintext = "12345678901";
    size_t encLen = 0;
    auto enc = crypto.encrypt(plaintext, 11, &encLen);
    ASSERT_NE(enc, nullptr);

    std::vector<uint8_t> raw = decodeBase64(enc.get(), encLen);
    ASSERT_EQ(raw.size(), 32u);

    const uint8_t badPadValues[] = {0x00, 0x11};
    for (uint8_t badPad : badPadValues) {
        std::vector<uint8_t> tamperedRaw = raw;
        tamperedRaw[15] ^= static_cast<uint8_t>(0x05 ^ badPad);
        std::string tampered = encodeBase64(tamperedRaw);
        ASSERT_FALSE(tampered.empty());

        size_t decLen = 999;
        auto dec = crypto.decrypt(tampered.c_str(), tampered.size(), &decLen);
        EXPECT_EQ(dec, nullptr);
        EXPECT_EQ(decLen, 0u);
    }
}

TEST_F(RealDeviceCryptoTest, RejectsWrongKey) {
    const char* plaintext = "secret-data"; // 11 bytes -> 1 ciphertext block
    size_t encLen = 0;
    auto enc = crypto.encrypt(plaintext, 11, &encLen);
    ASSERT_NE(enc, nullptr);

    std::vector<uint8_t> raw = decodeBase64(enc.get(), encLen);
    ASSERT_EQ(raw.size(), 32u);
    raw[15] ^= 0x80; // Flip MSB of last decrypted padding byte so both cannot be in 0x01..0x10
    std::string encFlipMsb = encodeBase64(raw);
    ASSERT_FALSE(encFlipMsb.empty());

    DeviceCrypto otherCrypto;
    otherCrypto.begin();

    size_t outLen1 = 999;
    auto dec1 = otherCrypto.decrypt(enc.get(), encLen, &outLen1);
    size_t outLen2 = 999;
    auto dec2 = otherCrypto.decrypt(encFlipMsb.c_str(), encFlipMsb.size(), &outLen2);

    EXPECT_TRUE(dec1 == nullptr || dec2 == nullptr);
    if (dec1 == nullptr) {
        EXPECT_EQ(outLen1, 0u);
    } else {
        EXPECT_NE(std::string(dec1.get(), outLen1), std::string(plaintext, 11));
    }
    if (dec2 == nullptr) {
        EXPECT_EQ(outLen2, 0u);
    } else {
        EXPECT_NE(std::string(dec2.get(), outLen2), std::string(plaintext, 11));
    }
}

TEST_F(RealDeviceCryptoTest, RejectsTruncatedAndUnalignedPayloads) {
    std::vector<uint8_t> ivOnly(16, 0x42);
    std::string ivOnlyB64 = encodeBase64(ivOnly);
    size_t decLen = 999;
    EXPECT_EQ(crypto.decrypt(ivOnlyB64.c_str(), ivOnlyB64.size(), &decLen), nullptr);
    EXPECT_EQ(decLen, 0u);

    std::vector<uint8_t> nonAligned(20, 0x42);
    std::string nonAlignedB64 = encodeBase64(nonAligned);
    decLen = 999;
    EXPECT_EQ(crypto.decrypt(nonAlignedB64.c_str(), nonAlignedB64.size(), &decLen), nullptr);
    EXPECT_EQ(decLen, 0u);

    const char* invalidInputs[] = {"abc", "not-valid", "AAAA!@#$", "AB==", "AA=A"};
    for (const char* bad : invalidInputs) {
        decLen = 999;
        EXPECT_EQ(crypto.decrypt(bad, std::strlen(bad), &decLen), nullptr);
        EXPECT_EQ(decLen, 0u);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
