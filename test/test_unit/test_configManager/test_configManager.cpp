/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/test_unit/test_configManager/test_configManager.cpp
 * Description: GoogleTest unit-test suite for the ConfigManager centralised
 *              configuration gateway. Validates module registration, beginAll()
 *              dispatch, saveModuleConfig, exportAllConfig, importAllConfig,
 *              schema version migration, and error handling.
 *
 *              All tests execute on the native GoogleTest runner (NATIVE_BUILD).
 *              Config files are written as "test/tmp/mod_<ns>.json" in the working directory.
 *
 * Exported Functions:
 * - main(): GoogleTest entry point
 */

#include <gtest/gtest.h>
#include "ConfigManager.hpp"
#include <fstream>
#include <cstdio>
#include <string>

// =============================================================================
// Mock IConfigurable Implementations
// =============================================================================

/** @brief Basic mock configurable with namespace "testmod" and schema v1. */
class MockModule : public IConfigurable {
public:
    bool applyConfigCalled = false;
    mutable bool serializeConfigCalled = false;
    bool resetConfigCalled = false;
    int lastFromVersion = -1;

    // Internal state that config should round-trip
    int setting1 = 42;
    bool setting2 = false;

    const char* getConfigNamespace() const override { return "testmod"; }
    int getSchemaVersion() const override { return 1; }

    void applyConfig(const JsonObject& section) override {
        applyConfigCalled = true;
        setting1 = section["setting1"] | 42;
        setting2 = section["setting2"] | false;
    }

    void serializeConfig(JsonObject& section) const override {
        serializeConfigCalled = true;
        section["setting1"] = setting1;
        section["setting2"] = setting2;
    }

    void resetConfig() override {
        resetConfigCalled = true;
        setting1 = 42;
        setting2 = false;
    }
};

/** @brief Mock module with a different namespace for multi-module tests. */
class MockModuleB : public IConfigurable {
public:
    bool applyConfigCalled = false;
    bool resetConfigCalled = false;
    std::string name = "default";

    const char* getConfigNamespace() const override { return "modB"; }
    int getSchemaVersion() const override { return 1; }

    void applyConfig(const JsonObject& section) override {
        applyConfigCalled = true;
        const char* n = section["name"] | "default";
        name = n;
    }

    void serializeConfig(JsonObject& section) const override {
        section["name"] = name.c_str();
    }

    void resetConfig() override {
        resetConfigCalled = true;
        name = "default";
    }
};

/** @brief Mock with schema version 2 that can migrate from v1. */
class MockModuleV2 : public IConfigurable {
public:
    bool migrateCalled = false;
    int lastFromVersion = -1;
    int value = 0;

    const char* getConfigNamespace() const override { return "v2mod"; }
    int getSchemaVersion() const override { return 2; }

    void applyConfig(const JsonObject& section) override {
        value = section["value"] | 0;
    }

    void serializeConfig(JsonObject& section) const override {
        section["value"] = value;
    }

    bool migrateConfig(JsonObject& section, int fromVersion) override {
        migrateCalled = true;
        lastFromVersion = fromVersion;
        // Migration: rename old_value → value
        if (section.containsKey("old_value")) {
            section["value"] = section["old_value"];
            section.remove("old_value");
        }
        return true;
    }

    void resetConfig() override {
        value = 0;
    }
};

// =============================================================================
// Test Fixture
// =============================================================================

class ConfigManagerTest : public ::testing::Test {
protected:
    ConfigManager configManager;

    void SetUp() override {
        // Clean up any leftover config files from previous test runs
        remove("test/tmp/mod_testmod.json");
        remove("test/tmp/mod_modB.json");
        remove("test/tmp/mod_v2mod.json");
    }

    void TearDown() override {
        remove("test/tmp/mod_testmod.json");
        remove("test/tmp/mod_modB.json");
        remove("test/tmp/mod_v2mod.json");
    }
};

// =============================================================================
// Test Group 1: Module Registration
// =============================================================================

TEST_F(ConfigManagerTest, RegisterModule_ValidPointer_ReturnsTrue) {
    MockModule mod;
    EXPECT_TRUE(configManager.registerModule(&mod));
}

TEST_F(ConfigManagerTest, RegisterModule_Nullptr_ReturnsFalse) {
    EXPECT_FALSE(configManager.registerModule(nullptr));
}

TEST_F(ConfigManagerTest, RegisterModule_ManyModules_AllSucceed) {
    // Registry is now a dynamic vector — no hard cap. Verify multiple
    // modules register successfully.
    MockModule modA;
    MockModuleB modB;
    MockModuleV2 modC;
    EXPECT_TRUE(configManager.registerModule(&modA));
    EXPECT_TRUE(configManager.registerModule(&modB));
    EXPECT_TRUE(configManager.registerModule(&modC));
}

TEST_F(ConfigManagerTest, RegisterModule_AfterBeginAll_ReturnsFalse) {
    // After beginAll() locks the registry, further registrations must be
    // rejected — matching Application's two-tier locked-vector model.
    MockModule mod;
    configManager.registerModule(&mod);
    configManager.beginAll();

    MockModule late;
    EXPECT_FALSE(configManager.registerModule(&late));
}

// =============================================================================
// Test Group 2: beginAll — Load dispatch
// =============================================================================

TEST_F(ConfigManagerTest, BeginAll_NoModules_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(configManager.beginAll());
}

TEST_F(ConfigManagerTest, BeginAll_NoFile_CallsResetConfig) {
    MockModule mod;
    configManager.registerModule(&mod);
    configManager.beginAll();

    // No file on disk → module should get resetConfig() called
    EXPECT_TRUE(mod.resetConfigCalled);
}

TEST_F(ConfigManagerTest, BeginAll_WithFile_CallsApplyConfig) {
    // Pre-create a config file
    std::ofstream file("test/tmp/mod_testmod.json");
    file << R"({"setting1":99,"setting2":true,"_v":1})";
    file.close();

    MockModule mod;
    configManager.registerModule(&mod);
    configManager.beginAll();

    EXPECT_TRUE(mod.applyConfigCalled);
    EXPECT_EQ(mod.setting1, 99);
    EXPECT_TRUE(mod.setting2);
}

TEST_F(ConfigManagerTest, BeginAll_CorruptedFile_CallsResetConfig) {
    std::ofstream file("test/tmp/mod_testmod.json");
    file << "{ invalid json : ";
    file.close();

    MockModule mod;
    configManager.registerModule(&mod);
    configManager.beginAll();

    EXPECT_TRUE(mod.resetConfigCalled);
}

TEST_F(ConfigManagerTest, BeginAll_MultipleModules_AllProcessed) {
    MockModule modA;
    MockModuleB modB;
    configManager.registerModule(&modA);
    configManager.registerModule(&modB);
    configManager.beginAll();

    // Both should have resetConfig called (no files exist)
    EXPECT_TRUE(modA.resetConfigCalled);
    EXPECT_TRUE(modB.resetConfigCalled);
}

// =============================================================================
// Test Group 3: saveModuleConfig
// =============================================================================

TEST_F(ConfigManagerTest, SaveModuleConfig_ValidModule_CreatesFile) {
    MockModule mod;
    mod.setting1 = 77;
    mod.setting2 = true;

    configManager.registerModule(&mod);
    EXPECT_TRUE(configManager.saveModuleConfig(&mod));

    // Verify file was created
    std::ifstream file("test/tmp/mod_testmod.json");
    EXPECT_TRUE(file.is_open());
}

TEST_F(ConfigManagerTest, SaveModuleConfig_Nullptr_ReturnsFalse) {
    EXPECT_FALSE(configManager.saveModuleConfig(nullptr));
}

TEST_F(ConfigManagerTest, SaveModuleConfig_Roundtrip_ValuesPreserved) {
    MockModule mod;
    mod.setting1 = 77;
    mod.setting2 = true;

    configManager.registerModule(&mod);
    EXPECT_TRUE(configManager.saveModuleConfig(&mod));

    // Read back via beginAll
    MockModule mod2;
    ConfigManager cm2;
    cm2.registerModule(&mod2);
    cm2.beginAll();

    EXPECT_TRUE(mod2.applyConfigCalled);
    EXPECT_EQ(mod2.setting1, 77);
    EXPECT_TRUE(mod2.setting2);
}

TEST_F(ConfigManagerTest, SaveModuleConfig_IncludesSchemaVersion) {
    MockModule mod;
    configManager.registerModule(&mod);
    configManager.saveModuleConfig(&mod);

    // Read the file directly
    std::ifstream file("test/tmp/mod_testmod.json");
    std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    JsonDocument doc;
    deserializeJson(doc, jsonStr);
    EXPECT_EQ(doc["_v"].as<int>(), 1);
}

// =============================================================================
// Test Group 4: Schema Migration
// =============================================================================

TEST_F(ConfigManagerTest, BeginAll_OldVersion_CallsMigrateConfig) {
    // Create a v1 config file for a module that declares v2
    std::ofstream file("test/tmp/mod_v2mod.json");
    file << R"({"old_value":55,"_v":1})";
    file.close();

    MockModuleV2 mod;
    configManager.registerModule(&mod);
    configManager.beginAll();

    EXPECT_TRUE(mod.migrateCalled);
    EXPECT_EQ(mod.lastFromVersion, 1);
    EXPECT_EQ(mod.value, 55); // Should have migrated old_value → value
}

TEST_F(ConfigManagerTest, BeginAll_CurrentVersion_DoesNotMigrate) {
    // Create a v2 config file (current version)
    std::ofstream file("test/tmp/mod_v2mod.json");
    file << R"({"value":42,"_v":2})";
    file.close();

    MockModuleV2 mod;
    configManager.registerModule(&mod);
    configManager.beginAll();

    EXPECT_FALSE(mod.migrateCalled);
    EXPECT_EQ(mod.value, 42);
}

// =============================================================================
// Test Group 5: Bulk Export/Import
// =============================================================================

TEST_F(ConfigManagerTest, ExportAllConfig_ProducesNamespaceSections) {
    MockModule modA;
    modA.setting1 = 88;
    MockModuleB modB;
    modB.name = "exported";

    configManager.registerModule(&modA);
    configManager.registerModule(&modB);

    JsonDocument outDoc;
    configManager.exportAllConfig(outDoc);

    EXPECT_TRUE(outDoc.containsKey("testmod"));
    EXPECT_TRUE(outDoc.containsKey("modB"));
    EXPECT_EQ(outDoc["testmod"]["setting1"].as<int>(), 88);
    EXPECT_STREQ(outDoc["modB"]["name"].as<const char*>(), "exported");
}

TEST_F(ConfigManagerTest, ExportAllConfig_IncludesVersionPerSection) {
    MockModule mod;
    configManager.registerModule(&mod);

    JsonDocument outDoc;
    configManager.exportAllConfig(outDoc);

    EXPECT_EQ(outDoc["testmod"]["_v"].as<int>(), 1);
}

TEST_F(ConfigManagerTest, ImportAllConfig_AppliesAndPersists) {
    MockModule mod;
    configManager.registerModule(&mod);

    JsonDocument inDoc;
    JsonObject section = inDoc["testmod"].to<JsonObject>();
    section["setting1"] = 123;
    section["setting2"] = true;

    EXPECT_TRUE(configManager.importAllConfig(inDoc));
    EXPECT_EQ(mod.setting1, 123);
    EXPECT_TRUE(mod.setting2);

    // Verify persisted to disk
    std::ifstream file("test/tmp/mod_testmod.json");
    EXPECT_TRUE(file.is_open());
}

TEST_F(ConfigManagerTest, ImportAllConfig_MissingNamespace_Skipped) {
    MockModule mod;
    configManager.registerModule(&mod);

    JsonDocument inDoc;
    // Deliberately don't include "testmod" section
    inDoc["other"] = "data";

    EXPECT_TRUE(configManager.importAllConfig(inDoc));
    // Module should not have been touched
    EXPECT_FALSE(mod.applyConfigCalled);
}


// =============================================================================
// Test Group 6: _meta envelope — export/import WO1 behaviour
// =============================================================================

TEST_F(ConfigManagerTest, ExportAllConfig_ContainsMetaKey) {
    MockModule modA;
    MockModuleB modB;
    configManager.registerModule(&modA);
    configManager.registerModule(&modB);

    JsonDocument outDoc;
    configManager.exportAllConfig(outDoc);

    // _meta must be present and must be a JSON object
    ASSERT_TRUE(outDoc.containsKey("_meta"));
    JsonObject meta = outDoc["_meta"].as<JsonObject>();
    EXPECT_FALSE(meta.isNull());

    // module_count must equal the number of registered modules (2)
    EXPECT_EQ(meta["module_count"].as<int>(), 2);
}

TEST_F(ConfigManagerTest, ImportAllConfig_SkipsMetaKey) {
    MockModule mod;
    configManager.registerModule(&mod);

    // Build a document that contains both a _meta envelope AND a valid namespace
    JsonDocument inDoc;
    JsonObject meta = inDoc["_meta"].to<JsonObject>();
    meta["backup_version"]   = 1;
    meta["firmware_version"] = "1.0.0-test";
    meta["module_count"]     = 1;

    JsonObject section = inDoc["testmod"].to<JsonObject>();
    section["setting1"] = 77;
    section["setting2"] = true;

    // Must not crash, must return true
    EXPECT_TRUE(configManager.importAllConfig(inDoc));

    // The valid namespace must have been applied
    EXPECT_TRUE(mod.applyConfigCalled);
    EXPECT_EQ(mod.setting1, 77);
    EXPECT_TRUE(mod.setting2);
}

TEST_F(ConfigManagerTest, ExportAllConfig_OtaNamespacePresent) {
    // Use a simple MockOtaConfigurable stub with namespace "ota"
    class MockOtaConfigurable : public IConfigurable {
    public:
        const char* getConfigNamespace() const override { return "ota"; }
        int getSchemaVersion() const override { return 1; }

        void applyConfig(const JsonObject& section) override {}

        void serializeConfig(JsonObject& section) const override {
            section["channel"]   = "stable";
            section["autoCheck"] = true;
        }

        void resetConfig() override {}
    };

    MockOtaConfigurable otaMod;
    configManager.registerModule(&otaMod);

    JsonDocument outDoc;
    configManager.exportAllConfig(outDoc);

    ASSERT_TRUE(outDoc.containsKey("ota"));
    EXPECT_STREQ(outDoc["ota"]["channel"].as<const char*>(), "stable");
    EXPECT_TRUE(outDoc["ota"]["autoCheck"].as<bool>());
}

// =============================================================================
// Test Group 7: IWebContributor
// =============================================================================

TEST_F(ConfigManagerTest, GetTabName_ReturnsBackup) {
    EXPECT_STREQ("Backup", configManager.getTabName());
}

TEST_F(ConfigManagerTest, GetParentTab_ReturnsSystem) {
    EXPECT_STREQ("System", configManager.getParentTab());
}

TEST_F(ConfigManagerTest, GetTabOrder_Returns40) {
    EXPECT_EQ(40, configManager.getTabOrder());
}

// =============================================================================
// GoogleTest main entry point
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
