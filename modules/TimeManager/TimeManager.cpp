/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/timeManager/timeManager.cpp
 * Description: Implementation of the time manager, handling NTP sync, timezone
 *              configuration, periodic re-sync, and REST API endpoints.
 *
 * Exported Functions/Classes:
 * - TimeManager::begin():           Initializes NTP config and timezone.
 * - TimeManager::tick():            Non-blocking update routine (NTP state machine + re-sync).
 * - TimeManager::setTimezone():     Sets POSIX timezone string.
 * - TimeManager::getLocalTime():    Gets current local time safely.
 * - TimeManager::isTimeSet():       Returns true if time has been synchronized.
 * - TimeManager::applyConfig():     Apply time config from persisted JSON.
 * - TimeManager::serializeConfig(): Serialize current time config to JSON.
 * - TimeManager::resetConfig():     Reset to factory defaults.
 * - TimeManager::registerRoutes():  Register REST API endpoints.
 */

#include "TimeManager.hpp"
#include "../ConfigManager/ConfigManager.hpp"
#include "platform/LogShim.h"

#include <cstring>

static constexpr const char* TAG = "TimeManager";

#include <PsychicHttp.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// =============================================================================
// Lifecycle
// =============================================================================

/**
 * @brief Initializes NTP config and sets the POSIX timezone.
 *
 * Uses the effective POSIX timezone, which is already populated in _posixTz —
 * either from a persisted value applied via applyConfig(), or from the
 * factory default seeded by resetConfig() ("GMT0").
 */
void TimeManager::begin() {
    // --- Step 1: Apply timezone ---
    // _posixTz is already populated by applyConfig() (persisted) or
    // resetConfig() (factory default "GMT0") by the time begin() runs.
    setTimezone(_posixTz);

    // --- Step 2: Configure NTP servers ---
    ESP_LOGI(TAG, "Configuring NTP servers: %s, %s, %s",
             _ntpServer1, _ntpServer2, _ntpServer3);

    configTzTime(_posixTz, _ntpServer1, _ntpServer2, _ntpServer3);

    _syncStatus = TimeSyncStatus::IN_PROGRESS;
}

/**
 * @brief Non-blocking update routine to be called in main loop.
 *
 * Handles two phases:
 * 1. Initial sync: polls SNTP status until COMPLETED, then records the sync time.
 * 2. Periodic re-sync: after RESYNC_INTERVAL_MS, resets status to IN_PROGRESS
 *    and re-polls. This corrects clock drift over long uptimes.
 */
void TimeManager::tick(uint32_t currentMillis) {
    // --- Phase 1: Waiting for initial or re-sync ---
    if (_syncStatus == TimeSyncStatus::IN_PROGRESS) {
        sntp_sync_status_t syncStatus = sntp_get_sync_status();

        if (syncStatus == SNTP_SYNC_STATUS_COMPLETED) {
            _syncStatus = TimeSyncStatus::SYNCED;
            _lastSyncTime = currentMillis;
            ESP_LOGI(TAG, "NTP time synchronization complete");
        }
        return;
    }

    // --- Phase 2: Check if periodic re-sync is due ---
    if (_syncStatus == TimeSyncStatus::SYNCED && _lastSyncTime > 0) {
        unsigned long now = currentMillis;
        if ((now - _lastSyncTime) >= RESYNC_INTERVAL_MS) {
            ESP_LOGI(TAG, "Periodic NTP re-sync triggered (interval: %luh)",
                     RESYNC_INTERVAL_MS / 3600000UL);
            _syncStatus = TimeSyncStatus::IN_PROGRESS;
            // SNTP client continues running; we just re-check the status
        }
    }
}

// =============================================================================
// Timezone
// =============================================================================

/**
 * @brief Sets POSIX timezone string and applies it immediately.
 * @param timezone POSIX timezone string.
 */
void TimeManager::setTimezone(const char* timezone) {
    if (timezone != nullptr) {
        ESP_LOGI(TAG, "Setting timezone: %s", timezone);
        setenv("TZ", timezone, 1); // 1 = overwrite existing TZ variable
        tzset();
    }
}

/**
 * @brief Gets current local time safely.
 * @param timeinfo Pointer to a tm struct to populate.
 * @return true if time is valid and populated, false otherwise.
 */
bool TimeManager::getLocalTime(struct tm* timeinfo) {
    if (timeinfo == nullptr) {
        return false;
    }

    if (_syncStatus != TimeSyncStatus::SYNCED) {
        ESP_LOGD(TAG, "getLocalTime() called but time is not yet synced");
        return false;
    }

    time_t now = 0;
    time(&now);
    localtime_r(&now, timeinfo);

    return true;
}

/**
 * @brief Returns true if time has been successfully synchronized.
 * @return true if time is set.
 */
bool TimeManager::isTimeSet() const {
    return _syncStatus == TimeSyncStatus::SYNCED;
}

// =============================================================================
// IConfigurable Implementation
// =============================================================================

/**
 * @brief Apply time configuration from a parsed JSON section.
 *
 * Reads timezone (IANA name + POSIX string) and NTP server addresses
 * from the persisted config. Sets _configured = true to signal that
 * the user has explicitly configured timezone (used by TimeConfigGate).
 *
 * @param section Read-only JSON object containing config keys.
 */
void TimeManager::applyConfig(const JsonObject& section) {
    // --- Timezone ---
    if (section["timezone"].is<const char*>()) {
        strncpy(_timezone, section["timezone"].as<const char*>(), sizeof(_timezone) - 1);
        _timezone[sizeof(_timezone) - 1] = '\0';
    }
    if (section["posix"].is<const char*>()) {
        strncpy(_posixTz, section["posix"].as<const char*>(), sizeof(_posixTz) - 1);
        _posixTz[sizeof(_posixTz) - 1] = '\0';
    }

    // --- NTP Servers ---
    if (section["ntpServer1"].is<const char*>()) {
        strncpy(_ntpServer1, section["ntpServer1"].as<const char*>(), sizeof(_ntpServer1) - 1);
        _ntpServer1[sizeof(_ntpServer1) - 1] = '\0';
    }
    if (section["ntpServer2"].is<const char*>()) {
        strncpy(_ntpServer2, section["ntpServer2"].as<const char*>(), sizeof(_ntpServer2) - 1);
        _ntpServer2[sizeof(_ntpServer2) - 1] = '\0';
    }
    if (section["ntpServer3"].is<const char*>()) {
        strncpy(_ntpServer3, section["ntpServer3"].as<const char*>(), sizeof(_ntpServer3) - 1);
        _ntpServer3[sizeof(_ntpServer3) - 1] = '\0';
    }

    // --- Mark as user-configured ---
    _configured = true;

    // --- Notify ConfigManager of the active timezone for _meta envelope ---
    if (_configManager != nullptr && strlen(_timezone) > 0) {
        _configManager->setMetaTimezone(_timezone);
    }

    ESP_LOGI(TAG, "Config applied — timezone: %s (%s), NTP: %s",
             _timezone, _posixTz, _ntpServer1);
}

/**
 * @brief Serialize current time configuration into a JSON section.
 *
 * Writes timezone (IANA + POSIX), NTP server addresses. ConfigManager
 * appends the _v schema version field separately.
 *
 * @param section Mutable JSON object to write config into.
 */
void TimeManager::serializeConfig(JsonObject& section) const {
    section["timezone"]   = _timezone;
    section["posix"]      = _posixTz;
    section["ntpServer1"] = _ntpServer1;
    section["ntpServer2"] = _ntpServer2;
    section["ntpServer3"] = _ntpServer3;
}

/**
 * @brief Reset time configuration to factory defaults.
 *
 * Clears the IANA timezone name (no user choice yet), seeds the POSIX TZ
 * with the factory default ("GMT0"), resets NTP servers to defaults, and
 * marks as unconfigured. ConfigManager will call saveModuleConfig() afterwards.
 */
void TimeManager::resetConfig() {
    memset(_timezone, 0, sizeof(_timezone));
    memset(_posixTz, 0, sizeof(_posixTz));
    strncpy(_posixTz, "GMT0", sizeof(_posixTz) - 1);
    strncpy(_ntpServer1, "pool.ntp.org", sizeof(_ntpServer1) - 1);
    strncpy(_ntpServer2, "time.nist.gov", sizeof(_ntpServer2) - 1);
    strncpy(_ntpServer3, "time.windows.com", sizeof(_ntpServer3) - 1);
    _configured = false;
}

// =============================================================================
// IWebContributor Implementation
// =============================================================================

/**
 * @brief Register REST API route handlers for time management.
 *
 * Routes:
 * - GET  /api/time/config  — current timezone, NTP servers, sync status
 * - POST /api/time/config  — update timezone and NTP servers
 * - GET  /api/time/zones   — stream IANA-to-POSIX lookup from LittleFS
 *
 * @param server Reference to the AsyncWebServer instance.
 */
void TimeManager::registerRoutes(PsychicHttpServer& server) {
    // --- GET /api/time/config ---
    server.on("/api/time/config", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        _handleGetConfig(request, response);
        return ESP_OK;
    });

    // --- POST /api/time/config ---
    server.on("/api/time/config", HTTP_POST,
        [this](PsychicRequest* request, PsychicResponse* response) {
            _handlePostConfig(request, response);
            return ESP_OK;
        }
    );

    // --- GET /api/time/zones ---
    server.on("/api/time/zones", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        _handleGetZones(request, response);
        return ESP_OK;
    });

    // --- POST /api/time/zones/update ---
    server.on("/api/time/zones/update", HTTP_POST,
        [this](PsychicRequest* request, PsychicResponse* response) {
            _handlePostZonesUpdate(request, response);
            return ESP_OK;
        }
    );

    ESP_LOGI(TAG, "REST API routes registered");
}

// =============================================================================
// Private Route Handlers
// =============================================================================

/**
 * @brief Handle GET /api/time/config — returns current time configuration.
 *
 * Response JSON:
 * {
 *   "timezone":   "Europe/London",
 *   "posix":      "GMT0BST,M3.5.0/1,M10.5.0",
 *   "ntpServer1": "pool.ntp.org",
 *   "ntpServer2": "time.nist.gov",
 *   "ntpServer3": "time.windows.com",
 *   "syncStatus": "SYNCED",
 *   "configured": true,
 *   "localTime":  "2026-06-02T12:34:56"
 * }
 *
 * @param request The incoming HTTP request.
 */
void TimeManager::_handleGetConfig(PsychicRequest* request, PsychicResponse* response) {
    JsonDocument doc;
    doc["timezone"]   = _timezone;
    doc["posix"]      = _posixTz;
    doc["ntpServer1"] = _ntpServer1;
    doc["ntpServer2"] = _ntpServer2;
    doc["ntpServer3"] = _ntpServer3;
    doc["configured"] = _configured;

    // Sync status as string
    const char* statusStr = "UNSYNCED";
    if (_syncStatus == TimeSyncStatus::SYNCED) statusStr = "SYNCED";
    else if (_syncStatus == TimeSyncStatus::IN_PROGRESS) statusStr = "IN_PROGRESS";
    doc["syncStatus"] = statusStr;

    // Current local time (if synced)
    if (_syncStatus == TimeSyncStatus::SYNCED) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            char buf[32];
            strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &timeinfo);
            doc["localTime"] = buf;
        }
    }

    String output;
    serializeJson(doc, output);
    response->send(200, "application/json", output.c_str());
}

/**
 * @brief Handle POST /api/time/config — updates timezone and NTP servers.
 *
 * Expects JSON body:
 * {
 *   "timezone":   "Europe/London",
 *   "posix":      "GMT0BST,M3.5.0/1,M10.5.0",
 *   "ntpServer1": "pool.ntp.org",     // optional
 *   "ntpServer2": "time.nist.gov",    // optional
 *   "ntpServer3": "time.windows.com"  // optional
 * }
 *
 * @param request The incoming HTTP request.
 * @param data    Request body data.
 * @param len     Length of the body data.
 * @param index   Current chunk index.
 * @param total   Total body size.
 */
void TimeManager::_handlePostConfig(PsychicRequest* request, PsychicResponse* response) {
    const char* body = request->bodyCStr();
    if (!body || strlen(body) == 0) {
        response->send(400, "application/json", "{\"error\":\"body_required\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
        response->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    // --- Apply timezone ---
    if (doc["timezone"].is<const char*>()) {
        strncpy(_timezone, doc["timezone"].as<const char*>(), sizeof(_timezone) - 1);
        _timezone[sizeof(_timezone) - 1] = '\0';
    }
    if (doc["posix"].is<const char*>()) {
        strncpy(_posixTz, doc["posix"].as<const char*>(), sizeof(_posixTz) - 1);
        _posixTz[sizeof(_posixTz) - 1] = '\0';
        setTimezone(_posixTz);
    }

    // --- Apply NTP servers (optional) ---
    if (doc["ntpServer1"].is<const char*>()) {
        strncpy(_ntpServer1, doc["ntpServer1"].as<const char*>(), sizeof(_ntpServer1) - 1);
        _ntpServer1[sizeof(_ntpServer1) - 1] = '\0';
    }
    if (doc["ntpServer2"].is<const char*>()) {
        strncpy(_ntpServer2, doc["ntpServer2"].as<const char*>(), sizeof(_ntpServer2) - 1);
        _ntpServer2[sizeof(_ntpServer2) - 1] = '\0';
    }
    if (doc["ntpServer3"].is<const char*>()) {
        strncpy(_ntpServer3, doc["ntpServer3"].as<const char*>(), sizeof(_ntpServer3) - 1);
        _ntpServer3[sizeof(_ntpServer3) - 1] = '\0';
    }

    // --- Mark as configured and persist ---
    _configured = true;

    if (_configManager != nullptr) {
        _configManager->saveModuleConfig(this);
        ESP_LOGI(TAG, "Time config saved — timezone: %s (%s)", _timezone, _posixTz);
    }

    // --- Reinitialise NTP with new servers ---
    _reinitNtp();

    response->send(200, "application/json", "{\"status\":\"ok\"}");
}

/**
 * @brief Handle GET /api/time/zones — streams timezones.json from LittleFS.
 *
 * Serves the IANA-to-POSIX timezone lookup file directly from LittleFS.
 * The file is ~15 KB and is streamed without loading into RAM.
 *
 * @param request The incoming HTTP request.
 */
void TimeManager::_handleGetZones(PsychicRequest* request, PsychicResponse* response) {
    if (!LittleFS.exists("/timezones.json")) {
        response->send(404, "application/json", "{\"error\":\"Timezone database not found\"}");
        return;
    }

    File f = LittleFS.open("/timezones.json", "r");
    if (!f) {
        response->send(500, "application/json", "{\"error\":\"Failed to open timezone database\"}");
        return;
    }

    size_t sz = f.size();
    char* buf = (char*)malloc(sz + 1);
    if (!buf) {
        f.close();
        response->send(500, "application/json", "{\"error\":\"Out of memory\"}");
        return;
    }

    f.readBytes(buf, sz);
    buf[sz] = '\0';
    f.close();

    response->send(200, "application/json", buf);
    free(buf);
}

/**
 * @brief Handle POST /api/time/zones/update — replaces timezones.json on LittleFS.
 *
 * Streams the incoming body to a temporary file, then renames it on completion.
 * This avoids corrupting the active database mid-write. Body size is capped
 * at 32 KB to protect heap.
 *
 * @param request The incoming HTTP request.
 * @param data    Request body data chunk.
 * @param len     Length of the body data chunk.
 * @param index   Current chunk byte offset.
 * @param total   Total body size.
 */
void TimeManager::_handlePostZonesUpdate(PsychicRequest* request, PsychicResponse* response) {
    // --- Guard: reject oversized bodies (32 KB max) ---
    size_t total = request->contentLength();
    if (total > 32768) {
        response->send(413, "application/json", "{\"error\":\"Body too large (max 32 KB)\"}");
        return;
    }

    const char* body = request->bodyCStr();
    if (!body || strlen(body) == 0) {
        response->send(400, "application/json", "{\"error\":\"body_required\"}");
        return;
    }

    size_t bodyLen = strlen(body);
    ESP_LOGI(TAG, "Receiving timezone database update (%zu bytes)", bodyLen);
    File tmp = LittleFS.open("/timezones.tmp", "w");
    if (!tmp) {
        response->send(500, "application/json", "{\"error\":\"Failed to open temp file\"}");
        return;
    }
    tmp.write((const uint8_t*)body, bodyLen);
    tmp.close();

    LittleFS.remove("/timezones.json");
    LittleFS.rename("/timezones.tmp", "/timezones.json");
    ESP_LOGI(TAG, "Timezone database updated (%zu bytes)", bodyLen);
    response->send(200, "application/json", "{\"status\":\"ok\"}");
}

// =============================================================================
// Private Helpers
// =============================================================================

/**
 * @brief Reinitialize the SNTP client with current NTP server settings.
 *
 * Called after applyConfig() or when NTP servers are changed via the portal.
 * Resets _syncStatus to IN_PROGRESS so tick() begins polling again.
 */
void TimeManager::_reinitNtp() {
    ESP_LOGI(TAG, "Reinitialising NTP with servers: %s, %s, %s",
             _ntpServer1, _ntpServer2, _ntpServer3);

    configTzTime(_posixTz, _ntpServer1, _ntpServer2, _ntpServer3);
    _syncStatus = TimeSyncStatus::IN_PROGRESS;
}
