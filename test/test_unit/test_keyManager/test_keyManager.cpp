/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Test: test/test_unit/test_keyManager/test_keyManager.cpp
 * Description: Implementation of MockDeviceCrypto, KeyManagerTest classes.
 */

#include <gtest/gtest.h>
#include <unistd.h>
#include "KeyManager.hpp"
#include "DeviceCrypto.hpp"
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
        unlink(KEY_MANAGER_PATH);
        keyManager.begin(&mockCrypto);
    }

    void TearDown() override {
        unlink(KEY_MANAGER_PATH);
    }
};

// =============================================================================
// Original Tests (updated for nested JSON format)
// =============================================================================

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

// =============================================================================
// Ownership Metadata Tests
// =============================================================================

TEST_F(KeyManagerTest, AddKeyWithMetadata_RoundTrip) {
    // Store a key with ownership metadata
    EXPECT_TRUE(keyManager.addKey("wifi_password", "my_secret_pass", "WiFi", true));

    // Value should still decrypt correctly
    std::string retrieved = keyManager.getKeyById("wifi_password");
    EXPECT_EQ(retrieved, "my_secret_pass");
}

TEST_F(KeyManagerTest, AddKeyWithMetadata_IsExclusive) {
    EXPECT_TRUE(keyManager.addKey("wifi_password", "pass123", "WiFi", true));

    EXPECT_TRUE(keyManager.isExclusive("wifi_password"));
}

TEST_F(KeyManagerTest, AddKeyWithoutMetadata_NotExclusive) {
    EXPECT_TRUE(keyManager.addKey("api_token", "secret123"));

    EXPECT_FALSE(keyManager.isExclusive("api_token"));
}

TEST_F(KeyManagerTest, IsExclusive_NonExistentKey) {
    EXPECT_FALSE(keyManager.isExclusive("does_not_exist"));
}

TEST_F(KeyManagerTest, IsExclusive_NullKey) {
    EXPECT_FALSE(keyManager.isExclusive(nullptr));
}

TEST_F(KeyManagerTest, IsExclusive_EmptyKey) {
    EXPECT_FALSE(keyManager.isExclusive(""));
}

TEST_F(KeyManagerTest, AddKeyWithManagedByButNotExclusive) {
    // managedBy set but exclusive=false — should not be exclusive
    EXPECT_TRUE(keyManager.addKey("shared_key", "val", "SomeModule", false));

    EXPECT_FALSE(keyManager.isExclusive("shared_key"));
}

TEST_F(KeyManagerTest, ListKeysWithMetadata_MixedKeys) {
    // Add a mix of managed and unmanaged keys
    EXPECT_TRUE(keyManager.addKey("wifi_password", "pass1", "WiFi", true));
    EXPECT_TRUE(keyManager.addKey("portal_hash", "hash1", "Security", true));
    EXPECT_TRUE(keyManager.addKey("user_api_key", "key1"));

    auto keys = keyManager.listKeysWithMetadata();
    EXPECT_EQ(keys.size(), 3u);

    // Find each key and verify metadata
    bool foundWifi = false, foundHash = false, foundUser = false;
    for (const auto& k : keys) {
        if (k.id == "wifi_password") {
            foundWifi = true;
            EXPECT_TRUE(k.hasValue);
            EXPECT_EQ(k.managedBy, "WiFi");
            EXPECT_TRUE(k.exclusive);
        } else if (k.id == "portal_hash") {
            foundHash = true;
            EXPECT_TRUE(k.hasValue);
            EXPECT_EQ(k.managedBy, "Security");
            EXPECT_TRUE(k.exclusive);
        } else if (k.id == "user_api_key") {
            foundUser = true;
            EXPECT_TRUE(k.hasValue);
            EXPECT_EQ(k.managedBy, "");
            EXPECT_FALSE(k.exclusive);
        }
    }
    EXPECT_TRUE(foundWifi);
    EXPECT_TRUE(foundHash);
    EXPECT_TRUE(foundUser);
}

TEST_F(KeyManagerTest, ListKeysWithMetadata_EmptyRegistry) {
    auto keys = keyManager.listKeysWithMetadata();
    EXPECT_TRUE(keys.empty());
}

TEST_F(KeyManagerTest, ListKeysWithMetadata_AttributedNonExclusive) {
    // managedBy set but not exclusive — should have managedBy but not exclusive
    EXPECT_TRUE(keyManager.addKey("shared_key", "val", "Analytics", false));

    auto keys = keyManager.listKeysWithMetadata();
    ASSERT_EQ(keys.size(), 1u);
    EXPECT_EQ(keys[0].id, "shared_key");
    EXPECT_EQ(keys[0].managedBy, "Analytics");
    EXPECT_FALSE(keys[0].exclusive);
}

TEST_F(KeyManagerTest, OverwriteKey_PreservesNewMetadata) {
    // First write without metadata
    EXPECT_TRUE(keyManager.addKey("evolving_key", "v1"));
    EXPECT_FALSE(keyManager.isExclusive("evolving_key"));

    // Overwrite with metadata
    EXPECT_TRUE(keyManager.addKey("evolving_key", "v2", "Module", true));
    EXPECT_TRUE(keyManager.isExclusive("evolving_key"));
    EXPECT_EQ(keyManager.getKeyById("evolving_key"), "v2");
}

TEST_F(KeyManagerTest, MetadataPersistsAcrossInstances) {
    EXPECT_TRUE(keyManager.addKey("persistent_managed", "secret", "WiFi", true));

    KeyManager newInstance;
    newInstance.begin(&mockCrypto);

    EXPECT_TRUE(newInstance.isExclusive("persistent_managed"));
    EXPECT_EQ(newInstance.getKeyById("persistent_managed"), "secret");

    auto keys = newInstance.listKeysWithMetadata();
    ASSERT_EQ(keys.size(), 1u);
    EXPECT_EQ(keys[0].managedBy, "WiFi");
    EXPECT_TRUE(keys[0].exclusive);
}

// =============================================================================
// SystemLocked & getOwner Tests
// =============================================================================

TEST_F(KeyManagerTest, SystemLockedKey_StorageAndQuery) {
    EXPECT_TRUE(keyManager.addKey("wifi_password", "pass123", "WiFi", true, true));

    EXPECT_TRUE(keyManager.isExclusive("wifi_password"));
    EXPECT_TRUE(keyManager.isSystemLocked("wifi_password"));
    EXPECT_EQ(keyManager.getOwner("wifi_password"), "WiFi");

    auto keys = keyManager.listKeysWithMetadata();
    ASSERT_EQ(keys.size(), 1u);
    EXPECT_TRUE(keys[0].systemLocked);
    EXPECT_TRUE(keys[0].exclusive);
    EXPECT_EQ(keys[0].managedBy, "WiFi");
}

TEST_F(KeyManagerTest, NonSystemLockedKey_IsSystemLockedReturnsFalse) {
    EXPECT_TRUE(keyManager.addKey("app_key", "val123", "App", true, false));

    EXPECT_TRUE(keyManager.isExclusive("app_key"));
    EXPECT_FALSE(keyManager.isSystemLocked("app_key"));
    EXPECT_EQ(keyManager.getOwner("app_key"), "App");
}

TEST_F(KeyManagerTest, GetOwner_UnmanagedOrNonExistent) {
    EXPECT_TRUE(keyManager.addKey("unmanaged", "val"));
    EXPECT_EQ(keyManager.getOwner("unmanaged"), "");
    EXPECT_EQ(keyManager.getOwner("does_not_exist"), "");
    EXPECT_FALSE(keyManager.isSystemLocked("does_not_exist"));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

