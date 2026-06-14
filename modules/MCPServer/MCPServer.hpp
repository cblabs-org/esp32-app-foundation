/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/mcpServer/MCPServer.hpp
 * Description: MCP (Model Context Protocol) JSON-RPC 2.0 server skeleton that
 *              reads line-delimited JSON from Serial and dispatches to registered
 *              tool handlers via the MCPToolRegistry. Built-in foundation tools
 *              are registered first via mcpBuiltinTools::registerAll(), then
 *              contributor tools are collected from IMCPContributor modules.
 *              On hardware, tick() reads characters from Serial; in NATIVE_BUILD,
 *              a mock Serial class is provided so the module compiles for the
 *              native test runner.
 *
 *              Implements IConfigurable (namespace "mcp") for runtime toggle
 *              persistence. The _mcpEnabled flag gates all Serial/HTTP processing;
 *              _mcpReadOnly blocks tools tagged with mutatesState = true.
 *
 * Exported Functions/Classes:
 * - MCPServer: MCP JSON-RPC server over Serial transport
 *
 *   Lifecycle:
 *   - begin():             Register built-in tools, collect contributor tools, and start serving
 *   - tick():              Poll Serial for incoming JSON-RPC lines (hardware only)
 *   - processRequest():    Process a single JSON-RPC request via Print stream
 *   - isEnabled():         Query whether the MCP server is currently enabled
 *
 *   IWebContributor (tab: "MCP", parent: "System", order: 30):
 *   - registerRoutes():    Register REST API routes
 *   - getTabName():        Return "MCP" for portal tab
 *   - getParentTab():      Return "System" for navigation hierarchy
 *   - getTabOrder():       Return 30 for pill ordering
 *
 *   IConfigurable (namespace: "mcp"):
 *   - getConfigNamespace(): Return "mcp"
 *   - applyConfig():       Apply persisted MCP settings from JSON
 *   - serializeConfig():   Serialize current MCP settings to JSON
 *   - resetConfig():       Restore secure defaults
 */
#pragma once

#include <vector>
#include <string>

#ifndef NATIVE_BUILD
    #include <Arduino.h>
#else
    #include <cstdio>
    #include <cstdint>
    #include <cstring>

    // -----------------------------------------------------------------------
    // Minimal Print base class mock for native builds
    // Mirrors the Arduino Print interface used by the JSON-RPC response path.
    // -----------------------------------------------------------------------
    class Print {
    public:
        virtual ~Print() = default;
        virtual size_t write(uint8_t c) = 0;
        virtual size_t write(const uint8_t* buf, size_t size) {
            size_t n = 0;
            for (size_t i = 0; i < size; ++i) n += write(buf[i]);
            return n;
        }
        size_t print(const char* s) {
            return write(reinterpret_cast<const uint8_t*>(s), strlen(s));
        }
        size_t println(const char* s) {
            size_t n = print(s);
            n += write('\n');
            return n;
        }
        size_t println() { return write('\n'); }
    };

    // -----------------------------------------------------------------------
    // Minimal String mock for native builds
    // Comprehensive stub satisfying mcpServer, wifiManager, and other modules.
    // -----------------------------------------------------------------------
    #ifndef STRING_STUB_DEFINED
    #define STRING_STUB_DEFINED
    class String {
    public:
        String() = default;
        String(const char* s) : _buf(s ? s : "") {}
        String(const std::string& s) : _buf(s) {}
        const char* c_str() const { return _buf.c_str(); }
        int length() const { return static_cast<int>(_buf.size()); }
        bool isEmpty() const { return _buf.empty(); }
        char operator[](int i) const { return _buf[static_cast<size_t>(i)]; }
        String& operator=(const char* s) { _buf = s ? s : ""; return *this; }
        String& operator+=(char c) { _buf += c; return *this; }
        String operator+(const char* s) const { return String(_buf + s); }
        String operator+(const String& s) const { return String(_buf + s._buf); }
        bool operator==(const char* s) const { return _buf == s; }
    private:
        std::string _buf;
    };
    #endif

    // -----------------------------------------------------------------------
    // Minimal Serial mock — string-buffer class for native compilation
    // Registration hook called by other modules to contribute their tools.
    // Mirrors the pattern used by WebServer for PsychicHttpServer.
    // -----------------------------------------------------------------------
    class MockSerial : public Print {
    public:
        size_t write(uint8_t c) override {
            _output += static_cast<char>(c);
            return 1;
        }
        int available() { return 0; }  // No-op in native — tests call _processLine directly
        int read() { return -1; }
        void begin(unsigned long) {}

        /// @brief Retrieve and clear the accumulated output buffer.
        std::string flush() {
            std::string out = std::move(_output);
            _output.clear();
            return out;
        }
        const std::string& getOutput() const { return _output; }

    private:
        std::string _output; ///< Accumulated serial output for test inspection
    };

    extern MockSerial Serial; // Defined in mcpServer.cpp
#endif

#include "MCPToolRegistry.hpp"
#include "MCPBuiltinTools.hpp"
#include "../../include/interfaces/IWebContributor.hpp"
#include "../../include/interfaces/IConfigurable.hpp"

// Forward-declare to avoid pulling in the full headers
class IMCPContributor;
class ConfigManager;
class TimeManager;
class WifiManager;
class AuthManager;

/**
 * @brief MCP JSON-RPC 2.0 server with Serial line-delimited transport.
 *
 * The server collects tools from all registered IMCPContributor modules
 * during begin() and processes incoming JSON-RPC requests in tick().
 * Supports: initialize, ping, tools/list, tools/call.
 *
 * Also implements IConfigurable (namespace "mcp") so runtime toggle
 * state (enabled/readOnly) persists across reboots via ConfigManager.
 */
class MCPServer : public IWebContributor, public IConfigurable {
public:
    /**
     * @brief Initialise the MCP server: register built-in and contributor tools.
     * @param contributors  List of IMCPContributor pointers to collect tools from.
     * @param configManager Optional pointer to the ConfigManager for config tools.
     * @param timeManager   Optional pointer to the TimeManager for built-in time tools.
     * @param wifiManager   Optional pointer to the WifiManager — single source of truth
     *                      for the device hostname shown on the status endpoint.
     * @param authManager   Optional pointer to the AuthManager for bearer token
     *                      authentication on HTTP MCP routes. When non-null and auth
     *                      is enabled, all HTTP /api/mcp/* requests require a valid
     *                      Authorization: Bearer header. Serial transport is exempt.
     */
    void begin(const std::vector<IMCPContributor*>& contributors, ConfigManager* configManager = nullptr, TimeManager* timeManager = nullptr, WifiManager* wifiManager = nullptr, AuthManager* authManager = nullptr);

    /**
     * @brief Process a single JSON-RPC request and write the response to the provided stream.
     * @param requestJson Null-terminated JSON string containing the request.
     * @param responseStream Print stream where the JSON response should be written.
     */
    void processRequest(const char* requestJson, Print& responseStream);

    /**
     * @brief Poll Serial for incoming data and dispatch complete lines.
     *
     * On hardware: reads available characters, accumulates a line buffer,
     * and dispatches on newline. In NATIVE_BUILD: no-op (tests call
     * _processLine directly via the test harness).
     */
    void tick();

    /**
     * @brief Access the tool registry (primarily for testing).
     * @return Const reference to the internal MCPToolRegistry.
     */
    const MCPToolRegistry& getRegistry() const { return _registry; }

    /**
     * @brief Late-bind display info for the capture_display tool.
     * @param info Populated display descriptor with framebuffer pointer.
     */
    void setDisplayInfo(const DisplayInfo& info);

    /**
     * @brief Query whether the MCP server is currently enabled.
     * @return true if the server is processing requests; false if disabled.
     */
    bool isEnabled() const { return _mcpEnabled; }

    // -----------------------------------------------------------------------
    // IWebContributor Implementation
    // -----------------------------------------------------------------------
    /**
     * @brief Registers the REST endpoints required for the Web UI.
     * @param server The PsychicHttpServer instance to register routes with.
     */
    void registerRoutes(PsychicHttpServer& server) override;

    /**
     * @brief The MCP server exposes a "MCP" UI tab under System.
     * @return "MCP"
     */
    const char* getTabName() const override { return "MCP"; }

    /**
     * @brief Get the parent tab for navigation hierarchy.
     * @return "System" — MCP appears as a sub-section of the System tab.
     */
    const char* getParentTab() const override { return "System"; }

    /**
     * @brief Get the tab display order within the System sub-navigation.
     * @return 30 — MCP pill appears after Keys (20).
     */
    int getTabOrder() const override { return 30; }

    // -----------------------------------------------------------------------
    // IConfigurable Implementation
    // -----------------------------------------------------------------------
    /**
     * @brief Return the config namespace for MCP settings persistence.
     * @return "mcp"
     */
    const char* getConfigNamespace() const override;

    /**
     * @brief Return the current config schema version.
     * @return 1 (initial schema)
     */
    int getSchemaVersion() const override;

    /**
     * @brief Apply MCP settings from a parsed JSON config section.
     * @param section JSON object containing "enabled" and "readOnly" keys.
     */
    void applyConfig(const JsonObject& section) override;

    /**
     * @brief Serialize current MCP settings into a JSON config section.
     * @param section Mutable JSON object to write "enabled" and "readOnly" into.
     */
    void serializeConfig(JsonObject& section) const override;

    /**
     * @brief Reset MCP settings to secure factory defaults.
     *
     * Defaults: enabled = true, readOnly = true (secure-by-default).
     */
    void resetConfig() override;

#ifdef NATIVE_BUILD
    /**
     * @brief Expose _processLine for native unit tests.
     * @param line The raw JSON-RPC request string.
     */
    void processLineForTest(const char* line);
#endif

private:
    MCPToolRegistry _registry;                    ///< Registered tool collection
    MCPToolRegistry* _registryPtr = nullptr;      ///< Pointer for list_tools lambda capture
    String          _inputBuffer;                 ///< Accumulator for incoming Serial bytes
    bool            _initialized = false;         ///< Guard: begin() must be called first
    bool            _mcpEnabled  = true;          ///< Master on/off switch — false disables all processing
    bool            _mcpReadOnly = false;         ///< Guard: if true, mutating tools return error
    ConfigManager*  _configManager = nullptr;     ///< Cached for runtime config persistence
    WifiManager*    _wifiManager = nullptr;       ///< Cached reference for status endpoint hostname (single source of truth)
    AuthManager*    _authManager = nullptr;       ///< Cached for bearer token auth on HTTP routes (WO6); nullptr = no auth

    /**
     * @brief Parse a complete JSON-RPC line and route to the appropriate handler.
     * @param line Null-terminated JSON string (one line, no trailing newline).
     */
    void _processLine(const char* line);

    /**
     * @brief Handle the MCP "initialize" handshake request.
     * @param req  The parsed JSON-RPC request document.
     * @param out  Print stream for the response (Serial on hardware).
     */
    void _handleInitialize(const JsonDocument& req, Print& out);

    /**
     * @brief Handle "tools/list" — enumerate all registered tools.
     * @param req  The parsed JSON-RPC request document.
     * @param out  Print stream for the response.
     */
    void _handleToolsList(const JsonDocument& req, Print& out);

    /**
     * @brief Handle "tools/call" — invoke a specific tool by name.
     * @param req  The parsed JSON-RPC request document.
     * @param out  Print stream for the response.
     */
    void _handleToolsCall(const JsonDocument& req, Print& out);

    /**
     * @brief Handle "ping" — simple keep-alive response.
     * @param req  The parsed JSON-RPC request document.
     * @param out  Print stream for the response.
     */
    void _handlePing(const JsonDocument& req, Print& out);

    /**
     * @brief Send a standard JSON-RPC error response.
     * @param code    JSON-RPC error code (e.g. -32700, -32601).
     * @param message Human-readable error description.
     * @param id      The request id (may be null for parse errors).
     * @param out     Print stream for the response.
     */
    void _sendError(int code, const char* message, JsonVariantConst id, Print& out);
};
