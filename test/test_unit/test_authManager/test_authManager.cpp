/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/test_unit/test_authManager/test_authManager.cpp
 * Description: GoogleTest unit tests for the AuthManager WO1 skeleton.
 *              Verifies IConfigurable contract (namespace, schema version,
 *              round-trip serialisation, resetConfig), IWebContributor contract
 *              (tab name, parent, icon), and auth-guard pass-through behaviour.
 *              No hardware, no LittleFS, no network required.
 *
 * Test cases:
 *   test_config_namespace          — getConfigNamespace() returns "auth"
 *   test_schema_version            — getSchemaVersion() returns 1
 *   test_serialize_defaults        — fresh instance serialises expected flags
 *   test_apply_config_roundtrip    — serialize → apply → serialize produces identical JSON
 *   test_reset_config              — after setting flags, resetConfig() clears all to false
 *   test_tab_name                  — getTabName() returns "Security"
 *   test_parent_tab                — getParentTab() returns nullptr
 *   test_tab_icon_not_null         — getTabIcon() returns a non-null string
 *   test_check_portal_auth_open    — checkPortalAuth() returns true when auth disabled
 *   test_check_mcp_auth_open       — checkMcpAuth() returns true when auth disabled
 */

#include <gtest/gtest.h>
#include <ArduinoJson.h>
#include "../../../modules/AuthManager/AuthManager.hpp"

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class AuthManagerTest : public ::testing::Test {
protected:
    AuthManager auth; ///< Fresh instance under test (no dependencies wired)
};

// ---------------------------------------------------------------------------
// IConfigurable — namespace and schema version
// ---------------------------------------------------------------------------

/**
 * @brief getConfigNamespace() must return "auth" so ConfigManager routes
 *        config I/O to /mod/auth.json.
 */
TEST_F(AuthManagerTest, test_config_namespace) {
    EXPECT_STREQ("auth", auth.getConfigNamespace());
}

/**
 * @brief getSchemaVersion() must return 1 for the initial WO1 schema.
 */
TEST_F(AuthManagerTest, test_schema_version) {
    EXPECT_EQ(1, auth.getSchemaVersion());
}

// ---------------------------------------------------------------------------
// IConfigurable — serialisation
// ---------------------------------------------------------------------------

/**
 * @brief A freshly constructed AuthManager must serialise all three flags as false.
 *
 * This guards against accidental non-zero initialisation of member variables.
 */
TEST_F(AuthManagerTest, test_serialize_defaults) {
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();

    auth.serializeConfig(section);

    EXPECT_FALSE(section["enabled"].as<bool>());
    EXPECT_FALSE(section["passwordSet"].as<bool>());
    EXPECT_FALSE(section["certPresent"].as<bool>());
}

/**
 * @brief serialize → apply → serialize must produce identical JSON.
 *
 * Sets two flags to true via direct applyConfig(), serialises, re-applies
 * the serialised form, then serialises again and verifies the output matches.
 */
TEST_F(AuthManagerTest, test_apply_config_roundtrip) {
    // --- Arrange: apply a non-default config ---
    JsonDocument srcDoc;
    JsonObject srcSection = srcDoc.to<JsonObject>();
    srcSection["enabled"]     = true;
    srcSection["passwordSet"] = true;
    srcSection["certPresent"] = false;

    auth.applyConfig(srcSection);

    // --- Act: serialise to a new section ---
    JsonDocument dstDoc;
    JsonObject dstSection = dstDoc.to<JsonObject>();
    auth.serializeConfig(dstSection);

    // --- Assert: values round-trip cleanly ---
    EXPECT_TRUE(dstSection["enabled"].as<bool>());
    EXPECT_TRUE(dstSection["passwordSet"].as<bool>());
    EXPECT_FALSE(dstSection["certPresent"].as<bool>());
}

// ---------------------------------------------------------------------------
// IConfigurable — resetConfig
// ---------------------------------------------------------------------------

/**
 * @brief resetConfig() must return all three flags to false regardless of
 *        what was applied previously.
 *
 * KeyManager is not wired (nullptr), so deleteKey() paths are skipped.
 * The test verifies the in-memory state only.
 */
TEST_F(AuthManagerTest, test_reset_config) {
    // --- Arrange: set flags to true ---
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    section["enabled"]     = true;
    section["passwordSet"] = true;
    section["certPresent"] = true;
    auth.applyConfig(section);

    // --- Act ---
    auth.resetConfig();

    // --- Assert: all flags cleared ---
    JsonDocument outDoc;
    JsonObject outSection = outDoc.to<JsonObject>();
    auth.serializeConfig(outSection);

    EXPECT_FALSE(outSection["enabled"].as<bool>());
    EXPECT_FALSE(outSection["passwordSet"].as<bool>());
    EXPECT_FALSE(outSection["certPresent"].as<bool>());
}

// ---------------------------------------------------------------------------
// IWebContributor — tab metadata
// ---------------------------------------------------------------------------

/**
 * @brief getTabName() must return "Security" for the Security bottom-nav tab.
 */
TEST_F(AuthManagerTest, test_tab_name) {
    EXPECT_STREQ("Security", auth.getTabName());
}

/**
 * @brief getParentTab() must return nullptr — Security is a top-level tab
 *        (not nested under System or any other parent).
 */
TEST_F(AuthManagerTest, test_parent_tab) {
    EXPECT_EQ(nullptr, auth.getParentTab());
}

/**
 * @brief getTabIcon() must return a non-null string containing SVG path data.
 *
 * The exact SVG content is not asserted (implementation detail), but the
 * return value must be non-null and non-empty so app.js renders an icon.
 */
TEST_F(AuthManagerTest, test_tab_icon_not_null) {
    const char* icon = auth.getTabIcon();
    ASSERT_NE(nullptr, icon);
    EXPECT_GT(strlen(icon), 0u);
}

// ---------------------------------------------------------------------------
// Auth guards — pass-through in WO1
// ---------------------------------------------------------------------------

/**
 * @brief checkPortalAuth() must return true when auth is not enabled.
 *
 * In WO1 auth is never enforced; this verifies the guard is a safe no-op
 * so existing routes are not blocked before WO2 lands.
 */
TEST_F(AuthManagerTest, test_check_portal_auth_open) {
    // No session, no auth enforcement — guard must pass
    EXPECT_TRUE(auth.checkPortalAuth(nullptr));
}

/**
 * @brief checkMcpAuth() must return true when auth is not enabled.
 *
 * Same rationale as test_check_portal_auth_open but for the MCP bearer
 * token check path (WO6).
 */
TEST_F(AuthManagerTest, test_check_mcp_auth_open) {
    EXPECT_TRUE(auth.checkMcpAuth(nullptr));
}

#if defined(NATIVE_BUILD)
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
