#include <gtest/gtest.h>
#include "keyManager.hpp"
#include "deviceCrypto.hpp"
#include <fstream>
#include <stdio.h>
#include <string>

/**
 * @brief Mock implementation of DeviceCrypto for testing round-trip.
 */
class MockDeviceCrypto : public DeviceCrypto {
public:
    std::unique_ptr<char[]> encrypt(const char* plaintext, size_t inputLen, size_t* outLen) override {
        if (!plaintext || inputLen == 0) {
            if (outLen) *outLen = 0;
            return nullptr;
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

class KeyManagerTest : public ::testing::Test {
protected:
    KeyManager keyManager;
    MockDeviceCrypto mockCrypto;

    void SetUp() override {
        remove(KEY_MANAGER_PATH);
        keyManager.begin(&mockCrypto);
    }

    void TearDown() override {
        remove(KEY_MANAGER_PATH);
    }
};

TEST_F(KeyManagerTest, AddAndRetrieveKey) {
    EXPECT_TRUE(keyManager.addKey("api_token", "super_secret_token"));
    
    std::string retrieved = keyManager.getKeyById("api_token");
    EXPECT_EQ(retrieved, "super_secret_token");
}

TEST_F(KeyManagerTest, RetrieveEmptyKey) {
    EXPECT_EQ(keyManager.getKeyById("non_existent"), "");
}

TEST_F(KeyManagerTest, DeleteKey) {
    EXPECT_TRUE(keyManager.addKey("to_delete", "secret_value"));
    EXPECT_EQ(keyManager.getKeyById("to_delete"), "secret_value");

    EXPECT_TRUE(keyManager.deleteKey("to_delete"));
    EXPECT_EQ(keyManager.getKeyById("to_delete"), "");
    
    EXPECT_FALSE(keyManager.deleteKey("to_delete")); // Should return false as it's already deleted
}

TEST_F(KeyManagerTest, PersistsAcrossInstances) {
    EXPECT_TRUE(keyManager.addKey("persistent_key", "persistent_secret"));
    
    KeyManager newInstance;
    newInstance.begin(&mockCrypto);
    
    EXPECT_EQ(newInstance.getKeyById("persistent_key"), "persistent_secret");
}

TEST_F(KeyManagerTest, AddKeyNullptrId) {
    EXPECT_FALSE(keyManager.addKey(nullptr, "secret"));
}

TEST_F(KeyManagerTest, AddKeyEmptyId) {
    EXPECT_FALSE(keyManager.addKey("", "secret"));
}

TEST_F(KeyManagerTest, AddKeyNullptrSecret) {
    EXPECT_FALSE(keyManager.addKey("mykey", nullptr));
}

TEST_F(KeyManagerTest, GetKeyByIdNullptr) {
    EXPECT_EQ(keyManager.getKeyById(nullptr), "");
}

TEST_F(KeyManagerTest, GetKeyByIdNonExistent) {
    EXPECT_EQ(keyManager.getKeyById("non_existent_key_2"), "");
}

TEST_F(KeyManagerTest, DeleteKeyNullptr) {
    EXPECT_FALSE(keyManager.deleteKey(nullptr));
}

TEST_F(KeyManagerTest, DeleteKeyNonExistent) {
    EXPECT_FALSE(keyManager.deleteKey("no-such-key"));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
