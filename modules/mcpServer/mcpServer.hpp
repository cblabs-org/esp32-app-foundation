/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/mcpServer/mcpServer.hpp
 * Description: MCP (Model Context Protocol) JSON-RPC 2.0 server skeleton that
 *              reads line-delimited JSON from Serial and dispatches to registered
 *              tool handlers via the McpToolRegistry. On hardware, tick() reads
 *              characters from Serial; in NATIVE_BUILD, a mock Serial class is
 *              provided so the module compiles for the native test runner.
 *
 * Exported Functions/Classes:
 * - McpServer: MCP JSON-RPC server over Serial transport
 *   - begin():  Collect tools from all IMcpContributor modules and start serving
 *   - tick():   Poll Serial for incoming JSON-RPC lines (hardware only)
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
    // Only the subset used by _inputBuffer (concatenation, c_str(), clear).
    // -----------------------------------------------------------------------
    class String {
    public:
        String() = default;
        String(const char* s) : _buf(s ? s : "") {}
        const char* c_str() const { return _buf.c_str(); }
        String& operator=(const char* s) { _buf = s ? s : ""; return *this; }
        String& operator+=(char c) { _buf += c; return *this; }
        bool operator==(const char* s) const { return _buf == s; }
    private:
        std::string _buf;
    };

    // -----------------------------------------------------------------------
    // Minimal Serial mock — string-buffer class for native compilation
    // Mirrors the pattern used by webServer.hpp for AsyncWebServer.
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

#include "mcpToolRegistry.hpp"

// Forward-declare to avoid pulling in the full interface header
class IMcpContributor;

/**
 * @brief MCP JSON-RPC 2.0 server with Serial line-delimited transport.
 *
 * The server collects tools from all registered IMcpContributor modules
 * during begin() and processes incoming JSON-RPC requests in tick().
 * Supports: initialize, ping, tools/list, tools/call.
 */
class McpServer {
public:
    /**
     * @brief Initialise the MCP server: register all contributor tools.
     * @param contributors List of IMcpContributor pointers to collect tools from.
     */
    void begin(const std::vector<IMcpContributor*>& contributors);

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
     * @return Const reference to the internal McpToolRegistry.
     */
    const McpToolRegistry& getRegistry() const { return _registry; }

#ifdef NATIVE_BUILD
    /**
     * @brief Expose _processLine for native unit tests.
     * @param line The raw JSON-RPC request string.
     */
    void processLineForTest(const char* line);
#endif

private:
    McpToolRegistry _registry;                    ///< Registered tool collection
    String          _inputBuffer;                 ///< Accumulator for incoming Serial bytes
    bool            _initialized = false;         ///< Guard: begin() must be called first

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
