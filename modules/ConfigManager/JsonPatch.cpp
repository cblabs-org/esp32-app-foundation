/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/ConfigManager/JsonPatch.cpp
 * Description: Implementation of the RFC 6902 JSON Patch subset engine.
 *              Provides JSON Pointer resolution (RFC 6901), token unescaping,
 *              and patch op dispatch for replace, add, test, remove.
 *
 *              All operations use fixed char buffers — no String class,
 *              no heap allocation beyond ArduinoJson's own pool.
 *
 * Exported Functions/Classes:
 * - JsonPatch: see JsonPatch.hpp
 */

#include "JsonPatch.hpp"
#include <string.h>
#include "platform/LogShim.h"

static constexpr const char* TAG = "JsonPatch";

// =============================================================================
// Pointer Validation (RFC 6901)
// =============================================================================

/**
 * @brief Validate a JSON Pointer string per RFC 6901 rules.
 *
 * Rejects: empty string, missing leading '/', trailing '/',
 * depth exceeding MAX_DEPTH, individual tokens exceeding MAX_TOKEN_LEN.
 *
 * @param ptr Null-terminated JSON Pointer string.
 * @return Error::OK if valid, Error::INVALID_POINTER otherwise.
 */
JsonPatch::Error JsonPatch::validatePointer(const char* ptr) {
    if (ptr == nullptr || ptr[0] == '\0') {
        ESP_LOGW(TAG, "validatePointer — empty or null pointer");
        return Error::INVALID_POINTER;
    }
    if (ptr[0] != '/') {
        ESP_LOGW(TAG, "validatePointer — must start with '/'");
        return Error::INVALID_POINTER;
    }

    size_t len = strlen(ptr);

    // Total length must fit in our working buffer
    if (len >= static_cast<size_t>(MAX_POINTER_LEN)) {
        ESP_LOGW(TAG, "validatePointer — pointer length %d exceeds MAX_POINTER_LEN %d",
                 (int)len, MAX_POINTER_LEN);
        return Error::INVALID_POINTER;
    }

    if (len > 1 && ptr[len - 1] == '/') {
        ESP_LOGW(TAG, "validatePointer — trailing '/' not allowed");
        return Error::INVALID_POINTER;
    }

    // Count depth and check token lengths
    int depth = 0;
    int tokenLen = 0;

    for (size_t i = 1; i < len; ++i) {  // skip leading '/'
        if (ptr[i] == '/') {
            ++depth;
            if (tokenLen > MAX_TOKEN_LEN) {
                ESP_LOGW(TAG, "validatePointer — token exceeds %d chars", MAX_TOKEN_LEN);
                return Error::INVALID_POINTER;
            }
            tokenLen = 0;
        } else {
            ++tokenLen;
        }
    }
    // Account for the final segment
    ++depth;
    if (tokenLen > MAX_TOKEN_LEN) {
        ESP_LOGW(TAG, "validatePointer — token exceeds %d chars", MAX_TOKEN_LEN);
        return Error::INVALID_POINTER;
    }

    if (depth > MAX_DEPTH) {
        ESP_LOGW(TAG, "validatePointer — depth %d exceeds limit %d", depth, MAX_DEPTH);
        return Error::INVALID_POINTER;
    }

    return Error::OK;
}

// =============================================================================
// Token Unescaping (RFC 6901 §3)
// =============================================================================

/**
 * @brief Unescape a JSON Pointer token in-place per RFC 6901 §3.
 *
 * Processing order matters: ~1 → '/' first, then ~0 → '~'.
 * This prevents double-unescaping (e.g. ~01 → ~1 → / is incorrect).
 *
 * @param token Null-terminated token string to unescape in-place.
 */
void JsonPatch::unescapeToken(char* token) {
    if (token == nullptr) return;

    // Pass 1: ~1 → '/'
    char* src = token;
    char* dst = token;
    while (*src) {
        if (src[0] == '~' && src[1] == '1') {
            *dst++ = '/';
            src += 2;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';

    // Pass 2: ~0 → '~'
    src = token;
    dst = token;
    while (*src) {
        if (src[0] == '~' && src[1] == '0') {
            *dst++ = '~';
            src += 2;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

// =============================================================================
// Parent Resolution
// =============================================================================

/**
 * @brief Resolve a JSON Pointer to its parent variant and final token.
 *
 * Splits the pointer on '/' delimiters, walks the document to the
 * second-to-last token, and returns the parent variant plus the final
 * token string. For array elements, detects all-digit tokens and
 * indexes into the array.
 *
 * @param root         The root variant to walk from.
 * @param ptr          Null-terminated JSON Pointer string (must be validated first).
 * @param parent       Output: the resolved parent variant.
 * @param lastToken    Output buffer for the final pointer token.
 * @param lastTokenCap Capacity of the lastToken buffer.
 * @return true if resolution succeeded, false if a segment was not found.
 */
bool JsonPatch::resolveParent(JsonVariant root, const char* ptr,
                              JsonVariant& parent, char* lastToken, size_t lastTokenCap) {
    // Work with a mutable copy for tokenisation
    char buf[MAX_POINTER_LEN];
    size_t ptrLen = strlen(ptr);
    if (ptrLen >= sizeof(buf)) return false;
    memcpy(buf, ptr, ptrLen + 1);

    // Split into tokens (skip leading '/')
    char* tokens[MAX_DEPTH];
    int tokenCount = 0;

    char* p = buf + 1;  // skip leading '/'
    while (*p && tokenCount < MAX_DEPTH) {
        tokens[tokenCount] = p;
        // Find next '/' or end
        char* slash = strchr(p, '/');
        if (slash) {
            *slash = '\0';
            p = slash + 1;
        } else {
            p = p + strlen(p);
        }
        // Unescape each token in-place
        unescapeToken(tokens[tokenCount]);
        ++tokenCount;
    }

    if (tokenCount == 0) return false;

    // Copy the final token to the output buffer
    strncpy(lastToken, tokens[tokenCount - 1], lastTokenCap - 1);
    lastToken[lastTokenCap - 1] = '\0';

    // Walk to the parent (all tokens except the last)
    JsonVariant current = root;
    for (int i = 0; i < tokenCount - 1; ++i) {
        const char* tok = tokens[i];

        // Check if this token is a numeric array index
        if (isStrictIndex(tok) && current.is<JsonArrayConst>()) {
            long idx = atoi(tok);
            // Numeric index into array
            if (idx < 0 || static_cast<size_t>(idx) >= current.as<JsonArray>().size()) {
                return false;
            }
            current = current[static_cast<size_t>(idx)];
        } else {
            // Object key lookup
            current = current[tok];
        }

        if (current.isNull()) {
            return false;
        }
    }

    parent = current;
    return true;
}

// =============================================================================
// Op Dispatch
// =============================================================================

/**
 * @brief Apply a single RFC 6902 op to a mutable JSON document.
 *
 * Reads the "op" field from the op object and dispatches to the
 * appropriate handler:
 *   - "replace": target must exist; replace value in-place
 *   - "add":     for objects: set (create or overwrite); for arrays:
 *                "-" appends, numeric index inserts (reject if > size)
 *   - "test":    resolve to value, compare with "value" field
 *   - "remove":  array targets → ARRAY_REMOVE_BLOCKED; object keys removed
 *
 * Unknown ops are rejected with UNKNOWN_OP.
 *
 * @param doc        Mutable root JSON object (the document being patched).
 * @param op         Read-only JSON object representing a single RFC 6902 op.
 * @param failedPath Output buffer for the path of the failed op (on error).
 * @param pathCap    Capacity of the failedPath buffer.
 * @return Error::OK on success, specific error code on failure.
 */
JsonPatch::Error JsonPatch::applyOne(JsonObject& doc, JsonObjectConst op,
                                     char* failedPath, size_t pathCap) {
    const char* opStr  = op["op"]   | (const char*)nullptr;
    const char* path   = op["path"] | (const char*)nullptr;

    if (opStr == nullptr || path == nullptr) {
        ESP_LOGW(TAG, "applyOne — missing 'op' or 'path' field");
        if (failedPath && pathCap > 0) {
            strncpy(failedPath, path ? path : "(null)", pathCap - 1);
            failedPath[pathCap - 1] = '\0';
        }
        return Error::INVALID_POINTER;
    }

    // Validate the pointer
    Error valErr = validatePointer(path);
    if (valErr != Error::OK) {
        if (failedPath && pathCap > 0) {
            strncpy(failedPath, path, pathCap - 1);
            failedPath[pathCap - 1] = '\0';
        }
        return valErr;
    }

    // RFC 6902 requires 'value' for replace, add, test
    if (strcmp(opStr, "replace") == 0 || strcmp(opStr, "add") == 0 || strcmp(opStr, "test") == 0) {
        if (!op["value"].is<JsonVariantConst>()) {
            ESP_LOGW(TAG, "applyOne — missing 'value' field for op '%s'", opStr);
            return Error::MISSING_VALUE;
        }
    }

    // Copy path into failedPath early (overwritten on success is fine)
    if (failedPath && pathCap > 0) {
        strncpy(failedPath, path, pathCap - 1);
        failedPath[pathCap - 1] = '\0';
    }

    // Resolve parent + last token
    JsonVariant parent;
    char lastToken[MAX_TOKEN_LEN + 1] = {};

    // -------------------------------------------------------------------------
    // REPLACE
    // -------------------------------------------------------------------------
    if (strcmp(opStr, "replace") == 0) {
        if (!resolveParent(doc, path, parent, lastToken, sizeof(lastToken))) {
            ESP_LOGW(TAG, "replace — path resolution failed: %s", path);
            return Error::PATH_NOT_FOUND;
        }

        // Target must exist
        if (parent.is<JsonObject>()) {
            JsonObject parentObj = parent.as<JsonObject>();
            // NOTE: ArduinoJson's isNull() returns true for both absent keys and keys
            // holding a literal JSON null value. This means a key explicitly set to null
            // cannot be replaced — it is treated as non-existent. This is deliberate for
            // config use (null config values are semantically equivalent to absent).
            if (!parentObj[lastToken].is<JsonVariant>() || parentObj[lastToken].isNull()) {
                ESP_LOGW(TAG, "replace — target does not exist: %s", path);
                return Error::PATH_NOT_FOUND;
            }
            parentObj[lastToken] = op["value"];
        } else if (parent.is<JsonArray>()) {
            if (!isStrictIndex(lastToken)) {
                ESP_LOGW(TAG, "replace — non-numeric array index: %s", lastToken);
                return Error::PATH_NOT_FOUND;
            }
            size_t idx = static_cast<size_t>(atoi(lastToken));
            JsonArray arr = parent.as<JsonArray>();
            if (idx >= arr.size()) {
                ESP_LOGW(TAG, "replace — index %zu out of range (size=%d)", idx, (int)arr.size());
                return Error::INDEX_OUT_OF_RANGE;
            }
            arr[idx] = op["value"];
        } else {
            ESP_LOGW(TAG, "replace — parent is neither object nor array: %s", path);
            return Error::PATH_NOT_FOUND;
        }

        return Error::OK;
    }

    // -------------------------------------------------------------------------
    // ADD
    // -------------------------------------------------------------------------
    if (strcmp(opStr, "add") == 0) {
        if (!resolveParent(doc, path, parent, lastToken, sizeof(lastToken))) {
            ESP_LOGW(TAG, "add — path resolution failed: %s", path);
            return Error::PATH_NOT_FOUND;
        }

        if (parent.is<JsonObject>()) {
            // For objects: set value (create or overwrite)
            JsonObject parentObj = parent.as<JsonObject>();
            parentObj[lastToken] = op["value"];
        } else if (parent.is<JsonArray>()) {
            JsonArray arr = parent.as<JsonArray>();
            if (strcmp(lastToken, "-") == 0) {
                // Append to array
                arr.add(op["value"]);
            } else {
                // Indexed array insert blocked in v1 — same shift hazard as remove.
                // Config arrays are fixed-extent; only append via "-" is supported.
                ESP_LOGW(TAG, "add — indexed array insert not supported: %s", path);
                return Error::ARRAY_INSERT_BLOCKED;
            }
        } else {
            ESP_LOGW(TAG, "add — parent is neither object nor array: %s", path);
            return Error::PATH_NOT_FOUND;
        }

        return Error::OK;
    }

    // -------------------------------------------------------------------------
    // TEST
    // -------------------------------------------------------------------------
    if (strcmp(opStr, "test") == 0) {
        if (!resolveParent(doc, path, parent, lastToken, sizeof(lastToken))) {
            ESP_LOGW(TAG, "test — path resolution failed: %s", path);
            return Error::PATH_NOT_FOUND;
        }

        JsonVariant target;
        if (parent.is<JsonObject>()) {
            JsonObject parentObj = parent.as<JsonObject>();
            // NOTE: ArduinoJson's isNull() returns true for both absent keys and keys
            // holding a literal JSON null value. This means a key explicitly set to null
            // cannot be tested — it is treated as non-existent. This is deliberate for
            // config use (null config values are semantically equivalent to absent).
            if (!parentObj[lastToken].is<JsonVariant>() || parentObj[lastToken].isNull()) {
                ESP_LOGW(TAG, "test — target does not exist: %s", path);
                return Error::PATH_NOT_FOUND;
            }
            target = parentObj[lastToken];
        } else if (parent.is<JsonArray>()) {
            if (!isStrictIndex(lastToken)) {
                ESP_LOGW(TAG, "test — non-numeric array index: %s", lastToken);
                return Error::PATH_NOT_FOUND;
            }
            size_t idx = static_cast<size_t>(atoi(lastToken));
            JsonArray arr = parent.as<JsonArray>();
            if (idx >= arr.size()) {
                ESP_LOGW(TAG, "test — index %zu out of range (size=%d)", idx, (int)arr.size());
                return Error::INDEX_OUT_OF_RANGE;
            }
            target = arr[idx];
        } else {
            ESP_LOGW(TAG, "test — parent is neither object nor array: %s", path);
            return Error::PATH_NOT_FOUND;
        }

        // Compare target value against op["value"]
        // NOTE: ArduinoJson's comparison coerces across numeric types, so
        // test with integer 5 matches stored 5.0. This is more forgiving than
        // RFC 6902's strict equality requirement, but appropriate for config
        // CAS operations where numeric precision is rarely the concern.
        JsonVariantConst expected = op["value"];
        if (target != expected) {
            ESP_LOGW(TAG, "test — value mismatch at %s", path);
            return Error::TEST_FAILED;
        }

        return Error::OK;
    }

    // -------------------------------------------------------------------------
    // REMOVE
    // -------------------------------------------------------------------------
    if (strcmp(opStr, "remove") == 0) {
        if (!resolveParent(doc, path, parent, lastToken, sizeof(lastToken))) {
            ESP_LOGW(TAG, "remove — path resolution failed: %s", path);
            return Error::PATH_NOT_FOUND;
        }

        if (parent.is<JsonArray>()) {
            // Array remove is explicitly blocked in v1
            ESP_LOGW(TAG, "remove — array element removal not supported: %s", path);
            return Error::ARRAY_REMOVE_BLOCKED;
        }

        if (parent.is<JsonObject>()) {
            JsonObject parentObj = parent.as<JsonObject>();
            // NOTE: ArduinoJson's isNull() returns true for both absent keys and keys
            // holding a literal JSON null value. This means a key explicitly set to null
            // cannot be removed — it is treated as non-existent. This is deliberate for
            // config use (null config values are semantically equivalent to absent).
            if (!parentObj[lastToken].is<JsonVariant>() || parentObj[lastToken].isNull()) {
                ESP_LOGW(TAG, "remove — key does not exist: %s", path);
                return Error::PATH_NOT_FOUND;
            }
            parentObj.remove(lastToken);
            return Error::OK;
        }

        ESP_LOGW(TAG, "remove — parent is neither object nor array: %s", path);
        return Error::PATH_NOT_FOUND;
    }

    // -------------------------------------------------------------------------
    // UNKNOWN OP
    // -------------------------------------------------------------------------
    ESP_LOGW(TAG, "applyOne — unknown op: '%s'", opStr);
    return Error::UNKNOWN_OP;
}

// =============================================================================
// Strict RFC 6901 Array Index Check
// =============================================================================

/**
 * @brief Check if a token is a valid RFC 6901 array index.
 *
 * Valid indices are strings of digits with no leading zeros (except "0" itself).
 * This replaces the previous strtol-based checks for stricter RFC compliance.
 *
 * @param token Null-terminated token string.
 * @return true if the token is a valid array index.
 */
bool JsonPatch::isStrictIndex(const char* token) {
    if (token == nullptr || token[0] == '\0') return false;
    // Leading zero only valid for "0" itself
    if (token[0] == '0' && token[1] != '\0') return false;
    for (const char* p = token; *p; ++p) {
        if (*p < '0' || *p > '9') return false;
    }
    return true;
}

// =============================================================================
// Value Resolution (read-only pointer walk)
// =============================================================================

/**
 * @brief Resolve a JSON Pointer to its value in a document.
 *
 * Uses the same pointer unescaping logic as applyOne()/resolveParent() but
 * operates on a const document. Intended for extracting stored values from
 * a post-apply snapshot.
 *
 * @param root  The root variant to walk from.
 * @param ptr   Null-terminated JSON Pointer string.
 * @param out   Output: the resolved value.
 * @return true if the path resolved successfully.
 */
bool JsonPatch::resolveValue(JsonVariantConst root, const char* ptr, JsonVariantConst& out) {
    if (ptr == nullptr || ptr[0] != '/') return false;

    char buf[MAX_POINTER_LEN];
    size_t ptrLen = strlen(ptr);
    if (ptrLen >= sizeof(buf)) return false;
    memcpy(buf, ptr, ptrLen + 1);

    JsonVariantConst current = root;
    char* p = buf + 1;  // skip leading '/'
    while (*p) {
        char* token = p;
        char* slash = strchr(p, '/');
        if (slash) {
            *slash = '\0';
            p = slash + 1;
        } else {
            p = p + strlen(p);
        }
        unescapeToken(token);

        if (current.is<JsonArrayConst>()) {
            // Strict RFC 6901 numeric index
            if (!isStrictIndex(token)) return false;
            size_t idx = static_cast<size_t>(atoi(token));
            if (idx >= current.as<JsonArrayConst>().size()) return false;
            current = current[idx];
        } else if (current.is<JsonObjectConst>()) {
            current = current[token];
        } else {
            return false;
        }
        if (current.isNull()) return false;
    }
    out = current;
    return true;
}
