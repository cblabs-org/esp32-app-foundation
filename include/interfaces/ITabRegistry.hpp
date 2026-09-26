/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/interfaces/ITabRegistry.hpp
 * Description: Abstract registry interface for registering navigation tabs in
 *              the web portal. Passed to IWebContributor implementors via
 *              registerTabs() to declare one or more UI tabs from a single
 *              module, eliminating the need for inner stub objects.
 *
 * Exported Structs:
 * - TabEntry: Value type describing a single navigation tab
 *
 * Exported Classes:
 * - ITabRegistry: Abstract interface for tab registration
 *   - addTab(): Register a navigation tab with optional parent, order, icon, and script
 */

#pragma once

/**
 * @brief Value type describing a single navigation tab entry.
 *
 * All const char* fields must point to storage with static or application
 * lifetime — the registry stores pointers, not copies.
 */
struct TabEntry {
    const char* name;    ///< Display name for the tab (required, static lifetime)
    const char* parent;  ///< Parent tab name for sub-navigation (nullptr = top-level)
    int         order;   ///< Sort order within the same hierarchy level (lower = first)
    const char* icon;    ///< SVG inner content for bottom-nav icon (nullptr = generic; bare geometry only)
    const char* script;  ///< Route path for tab JavaScript file (nullptr = none)
};

/**
 * @brief Abstract registry interface for registering navigation tabs.
 *
 * Passed to IWebContributor::registerTabs() during module registration.
 * Concrete implementation is provided by WebServerManager.
 *
 * All const char* parameters must point to storage with static or application
 * lifetime — the registry stores pointers, not copies.
 */
class ITabRegistry {
public:
    /** @brief Destructor for ITabRegistry. */
    virtual ~ITabRegistry() = default;

    /**
     * @brief Register a navigation tab with optional hierarchy and metadata.
     *
     * When providing an icon for a top-level tab (bottom navigation), return
     * BARE SVG inner geometry only (e.g. <path>, <rect>, <circle>, <line>).
     * The client (app.js) wraps the returned string in:
     *   <svg viewBox="0 0 24 24" fill="none" stroke="currentColor"
     *        stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
     *     {returned string}
     *   </svg>
     *
     * Return BARE geometry only — do NOT include stroke, stroke-width, fill,
     * or stroke-linecap attributes on inner elements. Any such attribute on
     * the inner element overrides the wrapper and breaks the design system's
     * 2px Feather stroke weight.
     *
     * Correct:   "<path d=\"M14 14.76V3.5a2.5 2.5 0 0 0-5 0v11.26a4.5 4.5 0 1 0 5 0z\"/>"
     * Incorrect: "<path d=\"...\" stroke-width=\"1.5\"/>"   // silently overrides wrapper
     *
     * @param name    Display name for the tab (required, static lifetime).
     * @param parent  Parent tab name for sub-navigation (nullptr = top-level).
     * @param order   Sort order within the same hierarchy level (lower = first, default 100).
     * @param icon    SVG inner content for bottom-nav icon (nullptr = generic icon).
     *                Must be bare geometry without inline stroke or fill attributes.
     * @param script  Route path for tab JavaScript file (nullptr = no script).
     */
    virtual void addTab(const char* name,
                        const char* parent = nullptr,
                        int         order  = 100,
                        const char* icon   = nullptr,
                        const char* script = nullptr) = 0;

    /**
     * @brief Convenience overload — register a tab from a TabEntry struct.
     *
     * Delegates to the 5-parameter addTab(). Non-virtual so concrete
     * registries only need to implement the parameter form.
     */
    void addTab(const TabEntry& entry) {
        addTab(entry.name, entry.parent, entry.order, entry.icon, entry.script);
    }
};
