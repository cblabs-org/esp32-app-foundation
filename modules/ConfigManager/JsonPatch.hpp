/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/ConfigManager/JsonPatch.hpp
 * Description: RFC 6902 JSON Patch subset — element-level config addressing.
 *              Provides JSON Pointer resolution (RFC 6901) and patch op dispatch
 *              for the ConfigManager gateway. All operations use fixed char
 *              buffers — no String class, no heap allocation beyond ArduinoJson's
 *              own pool.
 *
 *              Supported ops: replace, add, test, remove (object keys only).
 *              Unsupported: move, copy.
 *
 * Exported Classes:
 * - JsonPatch: Static utility class for JSON Patch operations
 *   - applyOne():        Apply a single RFC 6902 op to a mutable JSON document
 *   - validatePointer(): Validate a JSON Pointer string (RFC 6901)
 */

#pragma once
#include <ArduinoJson.h>

/**
 * @brief RFC 6902 JSON Patch subset — element-level config addressing.
 *
 * Provides JSON Pointer resolution (RFC 6901) and patch op dispatch
 * for the ConfigManager gateway. All operations use fixed char buffers —
 * no String class, no heap allocation beyond ArduinoJson's own pool.
 *
 * Supported ops: replace, add, test, remove (object keys only).
 * Unsupported: move, copy.
 */
class JsonPatch {
public:
    /// Maximum depth of JSON Pointer segments (e.g. /a/b/c/d/e/f = 6)
    static constexpr int MAX_DEPTH = 6;
    /// Maximum length of a single pointer token
    static constexpr int MAX_TOKEN_LEN = 32;
    /// Maximum ops per patch request
    static constexpr int MAX_PATCH_OPS = 32;
    /// Maximum total JSON Pointer string length (derived from MAX_DEPTH * (MAX_TOKEN_LEN + 1) + 1)
    static constexpr int MAX_POINTER_LEN = MAX_DEPTH * (MAX_TOKEN_LEN + 1) + 2;

    /// Error codes for patch operations
    enum class Error : uint8_t {
        OK = 0,
        INVALID_POINTER,       // malformed JSON Pointer
        PATH_NOT_FOUND,        // target path does not exist (for replace/test/remove)
        INDEX_OUT_OF_RANGE,    // array index >= size
        ARRAY_REMOVE_BLOCKED,  // remove on array index not supported in v1
        ARRAY_INSERT_BLOCKED,  // indexed array insert not supported in v1
        TEST_FAILED,           // test op value mismatch
        UNKNOWN_OP,            // unrecognised op string
        TOO_MANY_OPS,          // ops array exceeds MAX_PATCH_OPS
        MUTEX_TIMEOUT,         // could not acquire config mutex
        PERSIST_FAILED,        // ops applied in RAM but flash write failed
        MISSING_VALUE,         // op requires 'value' field but it was absent
    };

    /// Result of a patch operation
    struct PatchResult {
        Error  error    = Error::OK;
        int    opIndex  = -1;       // which op failed (-1 = N/A)
        char   path[MAX_POINTER_LEN] = {};  // path of failed op
        bool   applied  = false;    // true only if ALL ops succeeded and were persisted
        int    opsApplied = 0;      // count of ops successfully applied
    };

    /**
     * @brief Apply a single RFC 6902 op to a mutable JSON document.
     *
     * Dispatches to the appropriate handler based on the "op" field value.
     * Supported ops: "replace", "add", "test", "remove".
     *
     * @param doc        Mutable root JSON object (the document being patched).
     * @param op         Read-only JSON object representing a single RFC 6902 op.
     * @param failedPath Output buffer for the path of the failed op (on error).
     * @param pathCap    Capacity of the failedPath buffer.
     * @return Error::OK on success, specific error code on failure.
     */
    static Error applyOne(JsonObject& doc, JsonObjectConst op, char* failedPath, size_t pathCap);

    /**
     * @brief Validate a JSON Pointer string (RFC 6901).
     *
     * Checks: must start with '/', must not be empty, no trailing '/',
     * depth <= MAX_DEPTH, each token <= MAX_TOKEN_LEN characters.
     *
     * @param ptr Null-terminated JSON Pointer string.
     * @return Error::OK if valid, Error::INVALID_POINTER if not.
     */
    static Error validatePointer(const char* ptr);

    /**
     * @brief Resolve a JSON Pointer to its value in a document.
     *
     * Uses the same pointer resolver and unescaping as applyOne().
     *
     * @param root  The root variant to walk from.
     * @param ptr   Null-terminated JSON Pointer string.
     * @param out   Output: the resolved value.
     * @return true if the path resolved successfully.
     */
    static bool resolveValue(JsonVariantConst root, const char* ptr, JsonVariantConst& out);

private:
    /**
     * @brief Resolve a JSON Pointer to its parent variant and final token.
     *
     * E.g. "/channels/5/label" → parent = doc["channels"][5], lastToken = "label"
     *
     * @param root         The root variant to walk from.
     * @param ptr          Null-terminated JSON Pointer string.
     * @param parent       Output: the resolved parent variant.
     * @param lastToken    Output buffer for the final pointer token.
     * @param lastTokenCap Capacity of the lastToken buffer.
     * @return true if resolution succeeded, false otherwise.
     */
    static bool resolveParent(JsonVariant root, const char* ptr,
                              JsonVariant& parent, char* lastToken, size_t lastTokenCap);

    /**
     * @brief RFC 6901 §3 token unescaping: ~1 → /, ~0 → ~ (order matters).
     *
     * Modifies the token string in-place.
     *
     * @param token Null-terminated token string to unescape.
     */
    static void unescapeToken(char* token);

    /// Strict RFC 6901 array index check: digits only, no leading zeros
    static bool isStrictIndex(const char* token);
};
