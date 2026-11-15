/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/TimeManager/TimeManager.cpp
 * Description: Implementation of the time manager, handling NTP sync (including
 *              RFC 2132 DHCP Option 42), timezone rules (including RFC 4833
 *              DHCP Options 100/101), config schema v2 migration, EventBus
 *              handoff, periodic re-sync, and REST API endpoints.
 *
 * Exported Functions/Classes:
 * - lwip_dhcp_on_extra_option(): Strong LwIP hook staging RFC 4833 Options 100/101.
 * - lwip_dhcp_tz_take_pending(): Thread-safe spinlock drain of staged TZ options.
 * - TimeManager::begin():        Initializes NTP config (and DHCP SNTP mode) and timezone.
 * - TimeManager::tick():         Drains staged DHCP TZ options, refreshes Option 42 slots,
 *                                and runs the non-blocking NTP state machine.
 * - TimeManager::setTimezone():  Sets POSIX timezone string.
 * - TimeManager::getLocalTime(): Gets current local time safely.
 * - TimeManager::isTimeSet():    Returns true if time has been synchronized.
 * - TimeManager::migrateConfig(): Migrates persisted schema v1 config to v2.
 * - TimeManager::applyConfig():  Apply time config from persisted JSON.
 * - TimeManager::serializeConfig(): Serialize persisted time config to JSON.
 * - TimeManager::resetConfig():  Reset config and transient DHCP state to defaults.
 * - TimeManager::registerRoutes(): Register REST API endpoints.
 * - TimeManager::setEventBus():  Subscribe once to WifiEvents::WIFI_CONNECTED.
 */

#include "TimeManager.hpp"
#include "lwip_dhcp_tz_hooks.h"
#include "EventBus.hpp"
#include "utils/serveGzipAsset.hpp"
#include "ConfigManager.hpp"
#include "platform/LogShim.h"
#include "portal_assets/time_js.hpp"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <esp_netif.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#if defined(ESP_PLATFORM) && !defined(UNIT_TEST)
#include <sdkconfig.h>
#include <lwip/ip_addr.h>
#include <lwip/pbuf.h>
#include <lwip/prot/dhcp.h>
#endif

static constexpr const char* TAG = "TimeManager";

#include <PsychicHttp.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// =============================================================================
// Static Helpers & LwIP RFC 4833 DHCP Timezone Hook
// =============================================================================

namespace {

#ifdef CONFIG_LWIP_SNTP_MAX_SERVERS
static constexpr size_t EFFECTIVE_SNTP_SLOTS =
    (TimeManager::MAX_SNTP_SLOTS < static_cast<size_t>(CONFIG_LWIP_SNTP_MAX_SERVERS))
        ? TimeManager::MAX_SNTP_SLOTS
        : static_cast<size_t>(CONFIG_LWIP_SNTP_MAX_SERVERS);
#else
static constexpr size_t EFFECTIVE_SNTP_SLOTS = 1;
#endif

static portMUX_TYPE s_dhcpTzMux = portMUX_INITIALIZER_UNLOCKED;

struct DhcpTzStaging {
    bool hasPcode = false;
    bool hasTcode = false;
    char pcode[LWIP_DHCP_TZ_MAX_LEN] = {};
    char tcode[LWIP_DHCP_TZ_MAX_LEN] = {};
};

static DhcpTzStaging s_dhcpTzStaging;

/** @brief Copy src into bounded NUL-terminated dst buffer safely. */
static void copyBounded(char* dst, size_t cap, const char* src) {
    if (dst == nullptr || cap == 0) {
        return;
    }
    if (src == nullptr) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, cap - 1);
    dst[cap - 1] = '\0';
}

/** @brief Return true when DHCP state represents an accepted/bound lease context. */
static bool isAcceptedDhcpState(uint8_t state) {
    switch (state) {
        case DHCP_STATE_REQUESTING:
        case DHCP_STATE_RENEWING:
        case DHCP_STATE_REBINDING:
        case DHCP_STATE_REBOOTING:
        case DHCP_STATE_BOUND:
            return true;
        default:
            return false;
    }
}

/** @brief Validate printable ASCII with no control chars, embedded NULs, or outer spaces. */
static bool isValidDhcpTzPayload(const char* buf, uint8_t len) {
    if (buf == nullptr || len == 0 || len >= LWIP_DHCP_TZ_MAX_LEN) {
        return false;
    }
    for (uint8_t i = 0; i < len; ++i) {
        unsigned char ch = static_cast<unsigned char>(buf[i]);
        if (ch < 0x20u || ch > 0x7Eu) {
            return false;
        }
    }
    if (buf[0] == ' ' || buf[len - 1] == ' ') {
        return false;
    }
    return true;
}

} // namespace

/**
 * @brief Strong LwIP hook capturing RFC 4833 Options 100 (PCode) & 101 (TCode).
 *
 * Executes in LwIP TCP/IP task context. Performs zero heap allocations, VFS I/O,
 * EventBus calls, libc TZ mutations, or heavy logging.
 */
extern "C" void lwip_dhcp_on_extra_option(
    struct dhcp* /*dhcp*/, uint8_t state, uint8_t option,
    uint8_t len, struct pbuf* p, uint16_t offset) {
    if (!isAcceptedDhcpState(state)) {
        return;
    }
    if (option != LWIP_DHCP_OPTION_TZ_PCODE && option != LWIP_DHCP_OPTION_TZ_TCODE) {
        return;
    }
    if (p == nullptr || len == 0 || len >= LWIP_DHCP_TZ_MAX_LEN) {
        return;
    }

    char tmp[LWIP_DHCP_TZ_MAX_LEN];
    if (pbuf_copy_partial(p, tmp, len, offset) != len) {
        return;
    }
    tmp[len] = '\0';

    if (!isValidDhcpTzPayload(tmp, len)) {
        return;
    }

    portENTER_CRITICAL(&s_dhcpTzMux);
    if (option == LWIP_DHCP_OPTION_TZ_PCODE) {
        memcpy(s_dhcpTzStaging.pcode, tmp, static_cast<size_t>(len) + 1u);
        s_dhcpTzStaging.hasPcode = true;
    } else {
        memcpy(s_dhcpTzStaging.tcode, tmp, static_cast<size_t>(len) + 1u);
        s_dhcpTzStaging.hasTcode = true;
    }
    portEXIT_CRITICAL(&s_dhcpTzMux);
}

/**
 * @brief Drain pending RFC 4833 DHCP timezone payloads staged by LwIP hook.
 */
extern "C" bool lwip_dhcp_tz_take_pending(
    char* outPcode, size_t pcodeCap, bool* gotPcode,
    char* outTcode, size_t tcodeCap, bool* gotTcode) {
    bool any = false;
    bool localGotPcode = false;
    bool localGotTcode = false;
    char localPcode[LWIP_DHCP_TZ_MAX_LEN] = {};
    char localTcode[LWIP_DHCP_TZ_MAX_LEN] = {};

    portENTER_CRITICAL(&s_dhcpTzMux);
    if (s_dhcpTzStaging.hasPcode) {
        memcpy(localPcode, s_dhcpTzStaging.pcode, sizeof(localPcode));
        s_dhcpTzStaging.hasPcode = false;
        s_dhcpTzStaging.pcode[0] = '\0';
        localGotPcode = true;
        any = true;
    }
    if (s_dhcpTzStaging.hasTcode) {
        memcpy(localTcode, s_dhcpTzStaging.tcode, sizeof(localTcode));
        s_dhcpTzStaging.hasTcode = false;
        s_dhcpTzStaging.tcode[0] = '\0';
        localGotTcode = true;
        any = true;
    }
    portEXIT_CRITICAL(&s_dhcpTzMux);

    if (gotPcode != nullptr) {
        *gotPcode = localGotPcode;
    }
    if (outPcode != nullptr && pcodeCap > 0) {
        if (localGotPcode) {
            copyBounded(outPcode, pcodeCap, localPcode);
        } else {
            outPcode[0] = '\0';
        }
    }

    if (gotTcode != nullptr) {
        *gotTcode = localGotTcode;
    }
    if (outTcode != nullptr && tcodeCap > 0) {
        if (localGotTcode) {
            copyBounded(outTcode, tcodeCap, localTcode);
        } else {
            outTcode[0] = '\0';
        }
    }

    return any;
}

// =============================================================================
// Lifecycle
// =============================================================================

/**
 * @brief Initializes NTP config (including DHCP SNTP mode) and sets the active POSIX timezone.
 *
 * Uses getPosixTz(), which returns the active DHCP override when set or the persisted/default
 * fallback timezone ("GMT0") populated by applyConfig() / resetConfig().
 */
void TimeManager::begin() {
    _begun = true;

    // Drain any DHCP timezone staged before begin() completed
    _processPendingDhcpTimezone();

    // Configure timezone, DHCP Option 42 mode, and SNTP fallback servers
    _reinitNtp();
}

/**
 * @brief Non-blocking update routine to be called in main loop.
 *
 * Drains EventBus WiFi-connected signal and staged RFC 4833 DHCP timezone
 * payloads, refreshes Option 42 SNTP fallback slots, then drives the NTP
 * synchronization and periodic re-sync state machine.
 */
void TimeManager::tick(uint32_t currentMillis) {
    // Drain EventBus notification flag (set on small EventBus task stack)
    (void)_wifiConnectedPending.exchange(false, std::memory_order_acquire);

    // Drain any RFC 4833 Option 100/101 timezone staged by LwIP hook (O(1) spinlock check)
    _processPendingDhcpTimezone();

    // Refresh Option 42 telemetry and restore static fallbacks in cleared trailing slots
    if (_begun) {
        _refreshDhcpNtpStateAndFallback();
    }

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
// Timezone & Telemetry Helpers
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

/**
 * @brief Return the active DHCP timezone source as a stable lowercase string.
 */
const char* TimeManager::getDhcpTzSourceStr() const {
    switch (_dhcpTzSource) {
        case DhcpTzSource::PCode:
            return "pcode";
        case DhcpTzSource::TCode:
            return "tcode";
        case DhcpTzSource::None:
        default:
            return "none";
    }
}

// =============================================================================
// EventBus Dependency Injection
// =============================================================================

void TimeManager::setEventBus(EventBus* eb) {
    _eventBus = eb;
    if (_eventBus != nullptr && !_eventBusSubscribed) {
        if (_eventBus->subscribe("wifi.connected", _onWifiConnected, this)) {
            _eventBusSubscribed = true;
        }
    }
}

void TimeManager::_onWifiConnected(const char* /*topic*/, const void* /*payload*/, void* ctx) {
    if (ctx != nullptr) {
        static_cast<TimeManager*>(ctx)->_wifiConnectedPending.store(true, std::memory_order_release);
    }
}

// =============================================================================
// IConfigurable Implementation (Schema v2)
// =============================================================================

/**
 * @brief Migrate persisted v1 time configuration to schema v2.
 */
bool TimeManager::migrateConfig(JsonObject& section, int fromVersion) {
    if (fromVersion < 2) {
        if (section["dhcpNtp"].isNull()) {
            section["dhcpNtp"] = true;
        }
        if (section["dhcpTz"].isNull()) {
            const char* tz = section["timezone"].is<const char*>()
                ? section["timezone"].as<const char*>() : "";
            const char* posix = section["posix"].is<const char*>()
                ? section["posix"].as<const char*>() : "";

            bool manualTzConfigured =
                (tz != nullptr && tz[0] != '\0') ||
                (posix != nullptr && posix[0] != '\0' && strcmp(posix, "GMT0") != 0);

            section["dhcpTz"] = !manualTzConfigured;
        }
    }
    return true;
}

/**
 * @brief Apply time configuration from a parsed JSON section.
 *
 * Reads fallback timezone (IANA name + POSIX string), NTP server addresses,
 * and DHCP opt-in flags from the persisted config. Sets _configured = true
 * to signal that persisted config was loaded (used by TimePreconditionContributor).
 *
 * @param section Read-only JSON object containing config keys.
 */
void TimeManager::applyConfig(const JsonObject& section) {
    // --- Timezone ---
    if (section["timezone"].is<const char*>()) {
        copyBounded(_timezone, sizeof(_timezone), section["timezone"].as<const char*>());
    }
    if (section["posix"].is<const char*>()) {
        copyBounded(_posixTz, sizeof(_posixTz), section["posix"].as<const char*>());
    }

    // --- NTP Servers ---
    if (section["ntpServer1"].is<const char*>()) {
        copyBounded(_ntpServer1, sizeof(_ntpServer1), section["ntpServer1"].as<const char*>());
    }
    if (section["ntpServer2"].is<const char*>()) {
        copyBounded(_ntpServer2, sizeof(_ntpServer2), section["ntpServer2"].as<const char*>());
    }
    if (section["ntpServer3"].is<const char*>()) {
        copyBounded(_ntpServer3, sizeof(_ntpServer3), section["ntpServer3"].as<const char*>());
    }

    // --- DHCP Options (schema v2) ---
    if (section["dhcpNtp"].is<bool>()) {
        _dhcpNtpEnabled = section["dhcpNtp"].as<bool>();
    }
    if (section["dhcpTz"].is<bool>()) {
        _dhcpTzEnabled = section["dhcpTz"].as<bool>();
    }

    if (!_dhcpNtpEnabled) {
        _clearDhcpNtpRuntime();
    }
    if (!_dhcpTzEnabled) {
        _clearDhcpTzRuntime();
    }

    // --- Mark as configured ---
    _configured = true;

    // --- Notify ConfigManager of active/configured timezone for _meta envelope ---
    if (_configManager != nullptr) {
        const char* metaTz = getTimezone();
        if (metaTz != nullptr && metaTz[0] != '\0') {
            _configManager->setMetaTimezone(metaTz);
        }
    }

    ESP_LOGI(TAG, "Config applied — timezone: %s (%s), NTP: %s, dhcpNtp=%d, dhcpTz=%d",
             _timezone, _posixTz, _ntpServer1,
             _dhcpNtpEnabled ? 1 : 0, _dhcpTzEnabled ? 1 : 0);
}

/**
 * @brief Serialize persisted time configuration into a JSON section.
 *
 * Writes ONLY persisted fallback timezone, NTP server addresses, and DHCP opt-in
 * flags (never volatile runtime DHCP overrides). ConfigManager appends _v separately.
 *
 * @param section Mutable JSON object to write config into.
 */
void TimeManager::serializeConfig(JsonObject& section) const {
    section["timezone"]   = _timezone;
    section["posix"]      = _posixTz;
    section["ntpServer1"] = _ntpServer1;
    section["ntpServer2"] = _ntpServer2;
    section["ntpServer3"] = _ntpServer3;
    section["dhcpNtp"]    = _dhcpNtpEnabled;
    section["dhcpTz"]     = _dhcpTzEnabled;
}

/**
 * @brief Reset time configuration and transient DHCP runtime state to factory defaults.
 *
 * Clears the IANA timezone name (no user choice yet), seeds the POSIX TZ
 * with the factory default ("GMT0"), resets NTP servers to defaults, enables
 * DHCP NTP and TZ discovery, clears transient DHCP state, and marks as
 * unconfigured. ConfigManager will call saveModuleConfig() afterwards.
 */
void TimeManager::resetConfig() {
    memset(_timezone, 0, sizeof(_timezone));
    memset(_posixTz, 0, sizeof(_posixTz));
    copyBounded(_posixTz, sizeof(_posixTz), "GMT0");
    copyBounded(_ntpServer1, sizeof(_ntpServer1), "pool.ntp.org");
    copyBounded(_ntpServer2, sizeof(_ntpServer2), "time.nist.gov");
    copyBounded(_ntpServer3, sizeof(_ntpServer3), "time.windows.com");
    _configured = false;
    _dhcpNtpEnabled = true;
    _dhcpTzEnabled = true;
    _clearTransientDhcpState();
}

// =============================================================================
// IWebContributor Implementation
// =============================================================================

/**
 * @brief Register REST API route handlers for time management.
 *
 * Routes:
 * - GET  /api/time/config       — persisted fallback + active DHCP runtime telemetry
 * - POST /api/time/config       — update timezone, NTP servers, and DHCP opt-in toggles
 * - GET  /api/time/zones        — stream IANA-to-POSIX lookup from LittleFS
 * - POST /api/time/zones/update — replace timezones.json on LittleFS
 *
 * @param server Reference to the PsychicHttpServer instance.
 */
void TimeManager::registerRoutes(PsychicHttpServer& server) {
    server.on("/time.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        return serveGzipAsset(request, response, time_js_gz, time_js_gz_len, time_js_gz_etag, "application/javascript");
    });

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
 * @brief Handle GET /api/time/config — returns persisted config + active DHCP status.
 *
 * @param request  The incoming HTTP request.
 * @param response The HTTP response object.
 */
void TimeManager::_handleGetConfig(PsychicRequest* /*request*/, PsychicResponse* response) {
    JsonDocument doc;
    doc["timezone"]   = _timezone;
    doc["posix"]      = _posixTz;
    doc["activeTimezone"] = (_dhcpTzActive && _dhcpTzEnabled)
        ? ((_dhcpTimezone[0] != '\0') ? _dhcpTimezone : _dhcpPosixTz)
        : _timezone;
    doc["activePosix"]    = getPosixTz();
    doc["ntpServer1"]     = _ntpServer1;
    doc["ntpServer2"]     = _ntpServer2;
    doc["ntpServer3"]     = _ntpServer3;
    doc["dhcpNtp"]        = _dhcpNtpEnabled;
    doc["dhcpTz"]         = _dhcpTzEnabled;
    doc["dhcpNtpActive"]  = _dhcpNtpActive;

    JsonArray dhcpServers = doc["dhcpNtpServers"].to<JsonArray>();
    for (size_t i = 0; i < _dhcpNtpCount && i < MAX_SNTP_SLOTS; ++i) {
        if (_dhcpNtpServers[i][0] != '\0') {
            dhcpServers.add(_dhcpNtpServers[i]);
        }
    }

    doc["dhcpTzActive"] = _dhcpTzActive;
    doc["dhcpTzSource"] = getDhcpTzSourceStr();
    doc["configured"]   = isConfigured();

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
 * @brief Handle POST /api/time/config — updates timezone, NTP servers, and DHCP toggles.
 *
 * @param request  The incoming HTTP request.
 * @param response The HTTP response object.
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
        copyBounded(_timezone, sizeof(_timezone), doc["timezone"].as<const char*>());
    }
    if (doc["posix"].is<const char*>()) {
        copyBounded(_posixTz, sizeof(_posixTz), doc["posix"].as<const char*>());
    }

    // --- Apply NTP servers (optional) ---
    if (doc["ntpServer1"].is<const char*>()) {
        copyBounded(_ntpServer1, sizeof(_ntpServer1), doc["ntpServer1"].as<const char*>());
    }
    if (doc["ntpServer2"].is<const char*>()) {
        copyBounded(_ntpServer2, sizeof(_ntpServer2), doc["ntpServer2"].as<const char*>());
    }
    if (doc["ntpServer3"].is<const char*>()) {
        copyBounded(_ntpServer3, sizeof(_ntpServer3), doc["ntpServer3"].as<const char*>());
    }

    // --- Apply DHCP opt-in toggles (optional) ---
    if (doc["dhcpNtp"].is<bool>()) {
        _dhcpNtpEnabled = doc["dhcpNtp"].as<bool>();
        if (!_dhcpNtpEnabled) {
            _clearDhcpNtpRuntime();
        }
    }
    if (doc["dhcpTz"].is<bool>()) {
        _dhcpTzEnabled = doc["dhcpTz"].as<bool>();
        if (!_dhcpTzEnabled) {
            _clearDhcpTzRuntime();
        }
    }

    // --- Mark as configured, apply effective TZ, and persist ---
    _configured = true;
    setTimezone(getPosixTz());

    if (_configManager != nullptr) {
        const char* metaTz = getTimezone();
        if (metaTz != nullptr && metaTz[0] != '\0') {
            _configManager->setMetaTimezone(metaTz);
        }
        _configManager->saveModuleConfig(this);
        ESP_LOGI(TAG, "Time config saved — timezone: %s (%s), dhcpNtp=%d, dhcpTz=%d",
                 _timezone, _posixTz, _dhcpNtpEnabled ? 1 : 0, _dhcpTzEnabled ? 1 : 0);
    }

    // --- Reinitialise NTP with current DHCP setting and static servers ---
    _reinitNtp();

    response->send(200, "application/json", "{\"status\":\"ok\"}");
}

/**
 * @brief Handle GET /api/time/zones — streams timezones.json from LittleFS.
 *
 * Serves the IANA-to-POSIX timezone lookup file directly from LittleFS.
 *
 * @param request  The incoming HTTP request.
 * @param response The HTTP response object.
 */
void TimeManager::_handleGetZones(PsychicRequest* /*request*/, PsychicResponse* response) {
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
 * @param request  The incoming HTTP request.
 * @param response The HTTP response object.
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
// Private Runtime & DHCP Helpers
// =============================================================================

void TimeManager::_clearDhcpTzRuntime() {
    _dhcpTzActive = false;
    _dhcpTzSource = DhcpTzSource::None;
    _dhcpTimezone[0] = '\0';
    _dhcpPosixTz[0] = '\0';
}

void TimeManager::_clearDhcpNtpRuntime() {
    _dhcpNtpActive = false;
    _dhcpNtpCount = 0;
    memset(_dhcpNtpServers, 0, sizeof(_dhcpNtpServers));
}

void TimeManager::_clearTransientDhcpState() {
    _clearDhcpNtpRuntime();
    _clearDhcpTzRuntime();
}

void TimeManager::_processPendingDhcpTimezone() {
    char pcode[LWIP_DHCP_TZ_MAX_LEN] = {};
    char tcode[LWIP_DHCP_TZ_MAX_LEN] = {};
    bool gotPcode = false;
    bool gotTcode = false;

    if (!lwip_dhcp_tz_take_pending(pcode, sizeof(pcode), &gotPcode,
                                   tcode, sizeof(tcode), &gotTcode)) {
        return;
    }

    if (!_dhcpTzEnabled) {
        ESP_LOGD(TAG, "Discarding DHCP timezone option (dhcpTz disabled)");
        return;
    }

    if (gotPcode) {
        // Option 100 (PCode) takes precedence for reliable direct POSIX rules.
        copyBounded(_dhcpPosixTz, sizeof(_dhcpPosixTz), pcode);
        if (gotTcode) {
            copyBounded(_dhcpTimezone, sizeof(_dhcpTimezone), tcode);
        } else {
            _dhcpTimezone[0] = '\0';
        }
        _dhcpTzSource = DhcpTzSource::PCode;
        _dhcpTzActive = true;
        setTimezone(_dhcpPosixTz);

        if (_configManager != nullptr) {
            const char* metaTz = getTimezone();
            if (metaTz[0] != '\0') {
                _configManager->setMetaTimezone(metaTz);
            }
        }

        ESP_LOGI(TAG, "DHCP Option 100 PCode active: '%s' (IANA='%s')",
                 _dhcpPosixTz, _dhcpTimezone);
        return;
    }

    if (gotTcode) {
        char resolvedPosix[64] = {};
        if (_lookupIanaPosixTz(tcode, resolvedPosix, sizeof(resolvedPosix))) {
            copyBounded(_dhcpTimezone, sizeof(_dhcpTimezone), tcode);
            copyBounded(_dhcpPosixTz, sizeof(_dhcpPosixTz), resolvedPosix);
            _dhcpTzSource = DhcpTzSource::TCode;
            _dhcpTzActive = true;
            setTimezone(_dhcpPosixTz);

            if (_configManager != nullptr) {
                const char* metaTz = getTimezone();
                if (metaTz[0] != '\0') {
                    _configManager->setMetaTimezone(metaTz);
                }
            }

            ESP_LOGI(TAG, "DHCP Option 101 TCode active: '%s' -> '%s'",
                     _dhcpTimezone, _dhcpPosixTz);
        } else {
            ESP_LOGW(TAG, "DHCP Option 101 timezone '%s' not found in /timezones.json; keeping fallback",
                     tcode);
        }
    }
}

void TimeManager::_refreshDhcpNtpStateAndFallback() {
    if (!_dhcpNtpEnabled) {
        _clearDhcpNtpRuntime();
        return;
    }

    char discovered[MAX_SNTP_SLOTS][48] = {};
    size_t count = 0;

    for (size_t i = 0; i < EFFECTIVE_SNTP_SLOTS; ++i) {
        const char* srvName = esp_sntp_getservername(static_cast<uint8_t>(i));
        const ip_addr_t* srvIp = esp_sntp_getserver(static_cast<uint8_t>(i));
        if (srvName == nullptr && srvIp != nullptr && !ip_addr_isany(srvIp)) {
            char ipBuf[IPADDR_STRLEN_MAX] = {};
            if (ipaddr_ntoa_r(srvIp, ipBuf, sizeof(ipBuf)) != nullptr && ipBuf[0] != '\0') {
                copyBounded(discovered[count], sizeof(discovered[count]), ipBuf);
                ++count;
            }
        }
    }

    bool changed = (count != _dhcpNtpCount) || ((count > 0) != _dhcpNtpActive);
    if (!changed) {
        for (size_t i = 0; i < count; ++i) {
            if (strcmp(discovered[i], _dhcpNtpServers[i]) != 0) {
                changed = true;
                break;
            }
        }
    }

    _dhcpNtpCount = count;
    _dhcpNtpActive = (count > 0);
    for (size_t i = 0; i < MAX_SNTP_SLOTS; ++i) {
        if (i < count) {
            copyBounded(_dhcpNtpServers[i], sizeof(_dhcpNtpServers[i]), discovered[i]);
        } else {
            _dhcpNtpServers[i][0] = '\0';
        }
    }

    if (changed) {
        if (_dhcpNtpActive) {
            ESP_LOGI(TAG, "DHCP Option 42 active: %zu server(s) (primary=%s)",
                     _dhcpNtpCount, _dhcpNtpServers[0]);
        } else {
            ESP_LOGI(TAG, "DHCP Option 42 cleared; restoring static fallback SNTP servers");
        }
    }

    // LwIP dhcp_set_ntp_servers() clears slots [num .. SNTP_MAX_SERVERS - 1] to NULL.
    // Restore non-empty static fallbacks into cleared trailing slots without overwriting
    // DHCP Option 42 IPs or duplicating an already-present static server name.
    if (_dhcpNtpActive || changed) {
        const char* fallbacks[3] = { _ntpServer1, _ntpServer2, _ntpServer3 };
        auto hasServerName = [&](const char* candidate) -> bool {
            for (size_t k = 0; k < EFFECTIVE_SNTP_SLOTS; ++k) {
                const char* existing = esp_sntp_getservername(static_cast<uint8_t>(k));
                if (existing != nullptr && strcmp(existing, candidate) == 0) {
                    return true;
                }
            }
            return false;
        };

        for (size_t slot = 0; slot < EFFECTIVE_SNTP_SLOTS; ++slot) {
            const char* srvName = esp_sntp_getservername(static_cast<uint8_t>(slot));
            const ip_addr_t* srvIp = esp_sntp_getserver(static_cast<uint8_t>(slot));
            bool slotEmpty = (srvName == nullptr) && (srvIp == nullptr || ip_addr_isany(srvIp));
            if (!slotEmpty) {
                continue;
            }
            for (size_t fIdx = 0; fIdx < 3; ++fIdx) {
                const char* fb = fallbacks[fIdx];
                if (fb[0] != '\0' && !hasServerName(fb)) {
                    esp_sntp_setservername(static_cast<uint8_t>(slot), fb);
                    break;
                }
            }
        }
    }
}

bool TimeManager::_lookupIanaPosixTz(const char* iana, char* outPosix, size_t outCap) {
    if (iana == nullptr || iana[0] == '\0' || outPosix == nullptr || outCap == 0) {
        return false;
    }
    outPosix[0] = '\0';

    if (!LittleFS.exists("/timezones.json")) {
        return false;
    }

    File f = LittleFS.open("/timezones.json", "r");
    if (!f) {
        return false;
    }

    enum class ParseState {
        ExpectOpenBrace,
        ExpectKeyOrClose,
        InKey,
        ExpectColon,
        ExpectValueQuote,
        InValue,
        SkipValueOther,
        ExpectCommaOrClose
    };

    ParseState state = ParseState::ExpectOpenBrace;
    bool escaped = false;
    bool keyMatches = false;
    size_t ianaLen = strlen(iana);
    size_t keyIdx = 0;
    size_t valIdx = 0;
    bool valOverflow = false;

    uint8_t buf[256];
    size_t n = 0;
    while ((n = f.read(buf, sizeof(buf))) > 0) {
        for (size_t i = 0; i < n; ++i) {
            char ch = static_cast<char>(buf[i]);

            switch (state) {
                case ParseState::ExpectOpenBrace:
                    if (ch == '{') {
                        state = ParseState::ExpectKeyOrClose;
                    } else if (!isspace(static_cast<unsigned char>(ch))) {
                        f.close();
                        return false;
                    }
                    break;

                case ParseState::ExpectKeyOrClose:
                    if (ch == '"') {
                        state = ParseState::InKey;
                        escaped = false;
                        keyIdx = 0;
                        keyMatches = true;
                    } else if (ch == '}') {
                        f.close();
                        return false;
                    }
                    break;

                case ParseState::InKey:
                    if (escaped) {
                        escaped = false;
                        if (keyMatches && keyIdx < ianaLen && iana[keyIdx] == ch) {
                            ++keyIdx;
                        } else {
                            keyMatches = false;
                        }
                    } else if (ch == '\\') {
                        escaped = true;
                    } else if (ch == '"') {
                        if (keyIdx != ianaLen) {
                            keyMatches = false;
                        }
                        state = ParseState::ExpectColon;
                    } else {
                        if (keyMatches && keyIdx < ianaLen && iana[keyIdx] == ch) {
                            ++keyIdx;
                        } else {
                            keyMatches = false;
                        }
                    }
                    break;

                case ParseState::ExpectColon:
                    if (ch == ':') {
                        state = ParseState::ExpectValueQuote;
                    } else if (!isspace(static_cast<unsigned char>(ch))) {
                        f.close();
                        return false;
                    }
                    break;

                case ParseState::ExpectValueQuote:
                    if (ch == '"') {
                        state = ParseState::InValue;
                        escaped = false;
                        valIdx = 0;
                        valOverflow = false;
                    } else if (!isspace(static_cast<unsigned char>(ch))) {
                        keyMatches = false;
                        state = ParseState::SkipValueOther;
                    }
                    break;

                case ParseState::InValue:
                    if (escaped) {
                        escaped = false;
                        if (keyMatches) {
                            if (valIdx + 1 < outCap) {
                                outPosix[valIdx++] = ch;
                            } else {
                                valOverflow = true;
                            }
                        }
                    } else if (ch == '\\') {
                        escaped = true;
                    } else if (ch == '"') {
                        if (keyMatches) {
                            if (!valOverflow && valIdx > 0) {
                                outPosix[valIdx] = '\0';
                                f.close();
                                return true;
                            }
                            outPosix[0] = '\0';
                            f.close();
                            return false;
                        }
                        state = ParseState::ExpectCommaOrClose;
                    } else {
                        if (keyMatches) {
                            if (valIdx + 1 < outCap) {
                                outPosix[valIdx++] = ch;
                            } else {
                                valOverflow = true;
                            }
                        }
                    }
                    break;

                case ParseState::SkipValueOther:
                    if (ch == ',') {
                        state = ParseState::ExpectKeyOrClose;
                    } else if (ch == '}') {
                        f.close();
                        return false;
                    }
                    break;

                case ParseState::ExpectCommaOrClose:
                    if (ch == ',') {
                        state = ParseState::ExpectKeyOrClose;
                    } else if (ch == '}') {
                        f.close();
                        return false;
                    }
                    break;
            }
        }
    }

    outPosix[0] = '\0';
    f.close();
    return false;
}

/**
 * @brief Reinitialize the SNTP client with current NTP server and DHCP settings.
 *
 * Ensures esp_netif_init() has initialized LwIP's tcpip_thread/tcpip_mbox before
 * any esp_sntp_* calls, applies the active POSIX timezone, stops existing SNTP if
 * active, configures LwIP DHCP Option 42 server mode ahead of the synchronous
 * esp_sntp_setserver() slot-clearing barrier (so pending tcpip_callback messages
 * drain before configTzTime() runs), and resets _syncStatus to IN_PROGRESS.
 */
void TimeManager::_reinitNtp() {
    (void)esp_netif_init();

    const char* effectivePosix = getPosixTz();
    setTimezone(effectivePosix);

    if (esp_sntp_enabled()) {
        esp_sntp_stop();
    }

#if defined(CONFIG_LWIP_DHCP_GET_NTP_SRV) && CONFIG_LWIP_DHCP_GET_NTP_SRV
    esp_sntp_servermode_dhcp(_dhcpNtpEnabled ? 1 : 0);
#endif

    for (size_t i = 0; i < EFFECTIVE_SNTP_SLOTS; ++i) {
        esp_sntp_setserver(static_cast<uint8_t>(i), nullptr);
    }

    const char* s1 = (_ntpServer1[0] != '\0') ? _ntpServer1 : nullptr;
    const char* s2 = (EFFECTIVE_SNTP_SLOTS >= 2 && _ntpServer2[0] != '\0') ? _ntpServer2 : nullptr;
    const char* s3 = (EFFECTIVE_SNTP_SLOTS >= 3 && _ntpServer3[0] != '\0') ? _ntpServer3 : nullptr;

    ESP_LOGI(TAG, "Configuring NTP (dhcpNtp=%d): %s, %s, %s",
             _dhcpNtpEnabled ? 1 : 0,
             s1 ? s1 : "(none)", s2 ? s2 : "(none)", s3 ? s3 : "(none)");

    configTzTime(effectivePosix, s1, s2, s3);
    _clearDhcpNtpRuntime();
    _syncStatus = TimeSyncStatus::IN_PROGRESS;
}
