/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Module: test/test_unit/test_helpers/MockTabRegistry.hpp
 * Description: Shared mock ITabRegistry for unit tests. Captures addTab()
 *              calls into a vector of TabEntry for assertion.
 *
 * NOTE: sample-app/test/test_unit/test_weather/test_main.cpp contains a
 *       local copy of this mock (separate PlatformIO project, cannot use
 *       shared helpers). Keep both in sync when modifying.
 */

#pragma once

#include "interfaces/ITabRegistry.hpp"
#include <string>
#include <vector>

/**
 * @brief Mock ITabRegistry that captures addTab() calls for test verification.
 *
 * Usage:
 * @code
 *   MockTabRegistry registry;
 *   module.registerTabs(registry);
 *   ASSERT_EQ(registry.entries.size(), 3u);
 *   EXPECT_EQ(registry.entries[0].name, "Security");
 * @endcode
 */
class MockTabRegistry : public ITabRegistry {
public:
    struct Entry {
        std::string name;
        std::string parent;
        int         order;
        std::string icon;
        std::string script;
    };

    std::vector<Entry> entries;

    using ITabRegistry::addTab;  // Expose TabEntry convenience overload

    /** @brief Auto-generated doc */
    void addTab(const char* name, const char* parent, int order,
                const char* icon, const char* script) override {
        entries.push_back({
            name   ? name   : "",
            parent ? parent : "",
            order,
            icon   ? icon   : "",
            script ? script : ""
        });
    }
};
