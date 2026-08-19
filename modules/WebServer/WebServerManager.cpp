/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/webServer/webServer.cpp
 * Description: Implementation of WebServerManager — enhanced for WO4 (PR-24) with:
 *              - Dual-server TLS mode (AsyncWebServerSecure on port 443 +
 *                redirect-only HTTP on port 80 when auth is enabled + cert present)
 *              - Heap guard (atomic semaphore + largest-free-block check)
 *              - Enhanced /api/system endpoint (20+ fields)
 *              - GET /api/app (application metadata)
 *              - GET /api/tabs (tab hierarchy for navigation)
 *              - POST /api/system/restart (armed restart)
 *              - POST /api/system/factory-reset (full erase + restart)
 *              - POST body size guard pattern for all POST routes
 *              - PROGMEM static routes for all portal assets
 *
 * Exported Functions/Classes:
 * - WebServerManager: see WebServerManager.hpp
 */

#include "WebServerManager.hpp"
#include "web/serveGzipAsset.hpp"
#include <ArduinoJson.h>

    #include "esp_heap_caps.h"
    #include "esp_task_wdt.h"
    // AuthManager full type needed for isAuthEnabled()/hasCert()/loadCert()
    // in TLS setup. Guarded so native LDF doesn't scan it, but even if it
    // does, the transitive DeviceCrypto include now resolves via relative path.
    #include "AuthManager.hpp"
#include "WifiManager.hpp"
#include "ConfigRoot.hpp"

// Portal PROGMEM assets — hardware only (require Arduino.h for PROGMEM macro)
#include "portal_assets/index_html.hpp"
#include "portal_assets/app_css.hpp"
#include "portal_assets/app_js.hpp"
#include "portal_assets/utils_js.hpp"
#include "portal_assets/system_js.hpp"
#include "portal_assets/wifi_js.hpp"
#include "portal_assets/backup_js.hpp"
#include "portal_assets/keys_js.hpp"
#include "portal_assets/appconfig_js.hpp"
#include "portal_assets/auth_js.hpp"
#include "portal_assets/credits_js.hpp"

#include "platform/LogShim.h"

// ---------------------------------------------------------------------------
// Platform-conditional includes for heap/system APIs
// ---------------------------------------------------------------------------
    #include <Arduino.h>
    #include "esp_system.h"
    #include "esp_chip_info.h"
    #include <dirent.h>
    #include <sys/stat.h>
    #include <unistd.h>
#ifndef NATIVE_BUILD
    #include "esp_heap_caps.h"
    #include "esp_partition.h"
    #include "esp_ota_ops.h"
    #include <LittleFS.h>
#endif
    #include <WiFi.h>
    #include <atomic>
#include <algorithm>

static constexpr const char* TAG = "WebServerManager";

// ---------------------------------------------------------------------------
// Heap guard — protects portal HTML + large responses from concurrent
// memory exhaustion. Only one high-memory request is served at a time.
// ---------------------------------------------------------------------------
static std::atomic<int> activeHighMemRequests{0};
constexpr int  MAX_CONCURRENT_HIGH_MEM      = 1;      // Max concurrent high-memory requests
constexpr int  HEAP_GUARD_MIN_BLOCK_BYTES   = 20000;  // Minimum largest free block (bytes)

/// ADR-15: Default maximum POST/PUT body size for routes that do not
/// declare a custom limit via IWebContributor::getMaxPostBody().
static constexpr size_t DEFAULT_MAX_POST_BODY = 4096;


WebServerManager::WebServerManager() : _httpServer(80) {
}

WebServerManager::~WebServerManager() {
    if (_httpsServer) {
        delete _httpsServer;
        _httpsServer = nullptr;
    }
}

void WebServerManager::setHttpConfig(size_t stackSize, int maxOpenSockets) {
    _httpStackSize = stackSize;
    _httpMaxOpenSockets = maxOpenSockets;
}

void WebServerManager::begin(const std::vector<IWebContributor*>& contributors,
                             AuthManager* authManager, WifiManager* wifiManager) {
    _authManager = authManager;
    _wifiManager = wifiManager;
    bool apMode = _wifiManager ? _wifiManager->getAPMode() : false;
    ESP_LOGI(TAG, "Initializing WebServerManager (apMode=%d)", apMode);

#ifndef UNIT_TEST
    _httpServer.config.stack_size = _httpStackSize;
    _httpServer.config.max_open_sockets = _httpMaxOpenSockets;
#endif

    // --- TLS mode decision ---
    // AsyncWebServer does NOT support TLS — it has no SSL layer.
    // The cert/key are generated and stored (WO3) ready for the PsychicHttp
    // migration (PR-25) which wraps ESP-IDF's esp_https_server with native HTTPS.
    // Until then, all routes are served on HTTP port 80.
    bool useTls = false;
    if ((!apMode || (authManager && authManager->isApTlsEnabled())) && authManager && authManager->isTlsEnabled() && authManager->hasCert()) {
        uint8_t *certDer = nullptr, *keyDer = nullptr;
        size_t certLen = 0, keyLen = 0;

        if (authManager->loadCert(&certDer, &certLen, &keyDer, &keyLen)) {
            ESP_LOGI(TAG, "TLS enabled with TLS cert present — enabling HTTPS on port 443");
            useTls = true;
            _httpsServer = new (std::nothrow) PsychicHttpsServer();
            if (_httpsServer == nullptr) {
                ESP_LOGE(TAG, "Failed to allocate PsychicHttpsServer, falling back to HTTP");
                useTls = false;
            } else {
#ifndef UNIT_TEST
                _httpsServer->config.stack_size = _httpStackSize;
                _httpsServer->config.max_open_sockets = _httpMaxOpenSockets;
#endif
                _httpsServer->setCertificate(certDer, certLen, keyDer, keyLen);
            }
        } else {
            ESP_LOGE(TAG, "Failed to load cert from storage, falling back to HTTP");
        }
    }

    // Select the active server for route registration
    // TLS mode: routes on HTTPS server; HTTP server becomes redirect-only
    // HTTP-open mode: all routes on the HTTP server (port 80)
    PsychicHttpServer* activeServer = &_httpServer; // default
    if (useTls && _httpsServer) {
        activeServer = _httpsServer;
    }
    _tlsMode = useTls;

    // --- Setup routing for static files served from PROGMEM (hardware only) ---
    // Heap-guarded portal HTML route — the largest single response
    activeServer->on("/", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        // --- Heap guard check ---
        int current = activeHighMemRequests.load();
        if (current >= MAX_CONCURRENT_HIGH_MEM ||
            heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) < HEAP_GUARD_MIN_BLOCK_BYTES) {
            return response->send(503, "application/json", "{\"error\":\"Low memory — retry\"}");
        }
        activeHighMemRequests++;

        esp_err_t err = serveGzipAsset(request, response, index_html_gz, index_html_gz_len, index_html_gz_etag, "text/html");

        activeHighMemRequests--;
        return err;
    });
    
    activeServer->on("/app.css", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        return serveGzipAsset(request, response, app_css_gz, app_css_gz_len, app_css_gz_etag, "text/css");
    });

    activeServer->on("/app.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        return serveGzipAsset(request, response, app_js_gz, app_js_gz_len, app_js_gz_etag, "application/javascript");
    });

    activeServer->on("/system.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        return serveGzipAsset(request, response, system_js_gz, system_js_gz_len, system_js_gz_etag, "application/javascript");
    });

    activeServer->on("/wifi.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        return serveGzipAsset(request, response, wifi_js_gz, wifi_js_gz_len, wifi_js_gz_etag, "application/javascript");
    });

    activeServer->on("/utils.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        return serveGzipAsset(request, response, utils_js_gz, utils_js_gz_len, utils_js_gz_etag, "application/javascript");
    });

    activeServer->on("/backup.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        return serveGzipAsset(request, response, backup_js_gz, backup_js_gz_len, backup_js_gz_etag, "application/javascript");
    });

    activeServer->on("/keys.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        return serveGzipAsset(request, response, keys_js_gz, keys_js_gz_len, keys_js_gz_etag, "application/javascript");
    });

    activeServer->on("/appconfig.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        return serveGzipAsset(request, response, appconfig_js_gz, appconfig_js_gz_len, appconfig_js_gz_etag, "application/javascript");
    });

    activeServer->on("/auth.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        return serveGzipAsset(request, response, auth_js_gz, auth_js_gz_len, auth_js_gz_etag, "application/javascript");
    });

    activeServer->on("/credits.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        return serveGzipAsset(request, response, credits_js_gz, credits_js_gz_len, credits_js_gz_etag, "application/javascript");
    });

    // --- Register all IWebContributor routes and build tabs array ---
    JsonDocument tabsDoc;
    JsonArray tabsArr = tabsDoc.to<JsonArray>();
    
    for (auto* contributor : contributors) {
        if (contributor != nullptr) {
            contributor->registerRoutes(*activeServer);
            const char* tabName = contributor->getTabName();
            if (tabName != nullptr) {
                JsonObject tabObj = tabsArr.add<JsonObject>();
                tabObj["name"] = tabName;
                tabObj["id"] = tabName; // Using name as ID for simplicity
                const char* parentTab = contributor->getParentTab();
                if (parentTab != nullptr) {
                    tabObj["parent"] = parentTab;
                }
                tabObj["order"] = contributor->getTabOrder();
                // Include SVG icon string when the contributor declares one.
                // nullptr return means "use the generic grid icon" (app.js default).
                const char* icon = contributor->getTabIcon();
                if (icon != nullptr) {
                    tabObj["icon"] = icon;
                }
                const char* script = contributor->getTabScript();
                if (script != nullptr && script[0] == '/') {
                    tabObj["script"] = script;
                }
            }
        }
    }

    // Build body-limit lookup from contributor-declared limits.
    // Static so the middleware lambda can capture a pointer to it.
    struct BodyLimitEntry {
        const char* prefix;
        size_t      maxBody;
    };
    static std::vector<BodyLimitEntry> s_bodyLimits;
    s_bodyLimits.clear();
    for (auto* contributor : contributors) {
        if (contributor == nullptr) continue;
        const char* prefix = contributor->getApiPrefix();
        if (prefix != nullptr) {
            s_bodyLimits.push_back({prefix, contributor->getMaxPostBody()});
        }
    }
    // Detect duplicate prefixes — take smallest limit (fail-safe) and warn
    for (size_t i = 0; i < s_bodyLimits.size(); ++i) {
        for (size_t j = i + 1; j < s_bodyLimits.size(); ) {
            if (strcmp(s_bodyLimits[i].prefix, s_bodyLimits[j].prefix) == 0) {
                size_t keep = std::min(s_bodyLimits[i].maxBody, s_bodyLimits[j].maxBody);
                ESP_LOGW(TAG, "Duplicate body-limit prefix '%s' (%u vs %u) — using %u (smallest)",
                         s_bodyLimits[i].prefix,
                         (unsigned)s_bodyLimits[i].maxBody,
                         (unsigned)s_bodyLimits[j].maxBody,
                         (unsigned)keep);
                s_bodyLimits[i].maxBody = keep;
                s_bodyLimits.erase(s_bodyLimits.begin() + j);
            } else {
                ++j;
            }
        }
    }
    // Sort by prefix length descending for longest-prefix-first matching
    std::stable_sort(s_bodyLimits.begin(), s_bodyLimits.end(),
        [](const BodyLimitEntry& a, const BodyLimitEntry& b) {
            return strlen(a.prefix) > strlen(b.prefix);
        });
    
    // Inject the "Status" sub-pill — WebServerManager hosts /api/system
    // but is not itself an IWebContributor, so add it manually.
    JsonObject statusTab = tabsArr.add<JsonObject>();
    statusTab["name"]   = "Status";
    statusTab["id"]     = "Status";
    statusTab["parent"] = "System";
    statusTab["order"]  = 10;

    String tabsJson;
    serializeJson(tabsDoc, tabsJson);

    // --- GET /api/tabs — tab hierarchy for navigation ---
    activeServer->on("/api/tabs", HTTP_GET, [tabsJson](PsychicRequest* request, PsychicResponse* response) {
        return response->send(200, "application/json", tabsJson.c_str());
    });

    // -----------------------------------------------------------------------
    // GET /api/system — Enhanced system information (20+ fields)
    // -----------------------------------------------------------------------
    activeServer->on("/api/system", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        // --- Heap guard for this large response ---
        int current = activeHighMemRequests.load();
        if (current >= MAX_CONCURRENT_HIGH_MEM ||
            heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) < HEAP_GUARD_MIN_BLOCK_BYTES) {
            return response->send(503, "application/json", "{\"error\":\"Low memory — retry\"}");
        }
        activeHighMemRequests++;

        // --- One-shot httpd stack HWM telemetry on first request (#119) ---
#ifdef ESP_PLATFORM
        static bool hwmLogged = false;
        if (!hwmLogged) {
            TaskHandle_t httpdTask = xTaskGetHandle("httpd");
            if (httpdTask != nullptr) {
                UBaseType_t hwm = uxTaskGetStackHighWaterMark(httpdTask);
                ESP_LOGI(TAG, "httpd stack HWM (first request): %u words (%u bytes free)",
                         (unsigned)hwm, (unsigned)(hwm * 4));
            }
            hwmLogged = true;
        }
#endif

        JsonDocument doc;

        // --- Internal DRAM metrics (always present on all ESP32 variants) ---
        doc["heap_free"]             = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        doc["heap_total"]            = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
        doc["heap_min_free"]         = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
        doc["heap_max_alloc"]        = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);

        // --- PSRAM metrics (returns 0 on boards without PSRAM) ---
        doc["psram_free"]            = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        doc["psram_total"]           = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);

        // --- Combined totals (DEFAULT = internal + PSRAM if available) ---
        doc["heap_default_free"]     = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
        doc["heap_default_total"]    = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);

        // --- Storage metrics (LittleFS) ---
        size_t totalBytes = 0, usedBytes = 0;
#ifndef NATIVE_BUILD
        totalBytes = LittleFS.totalBytes();
        usedBytes = LittleFS.usedBytes();
        if (totalBytes > 0) {
            doc["storage_used"]   = usedBytes;
            doc["storage_total"]  = totalBytes;
        } else {
            doc["storage_used"]   = 0;
            doc["storage_total"]  = 0;
        }
#else
        doc["storage_used"]   = 0;
        doc["storage_total"]  = 0;
#endif

        // --- Temperature (internal sensor) ---
        float chipTemp = 0.0f;
        if (temperatureRead() != 0) {
            chipTemp = temperatureRead();
        }
        doc["chip_temp"] = chipTemp;

        // --- Uptime ---
        doc["uptime"] = millis() / 1000;

        // --- Firmware / Build info ---
        doc["firmware_version"] = BUILD_VERSION;
        doc["build_serial"]     = __DATE__ " " __TIME__;
        doc["sdk_version"]      = ESP.getSdkVersion();

        // --- Chip info ---
        esp_chip_info_t chipInfo;
        esp_chip_info(&chipInfo);
        doc["chip_model"]    = ESP.getChipModel();
        doc["chip_revision"] = chipInfo.revision;
        doc["chip_cores"]    = chipInfo.cores;
        doc["flash_size"]    = ESP.getFlashChipSize();

        // --- WiFi info ---
        if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
            doc["ap_mode"]   = true;
            doc["wifi_ssid"] = WiFi.softAPSSID();
            doc["wifi_ip"]   = WiFi.softAPIP().toString();
            doc["wifi_mac"]  = WiFi.softAPmacAddress();
            doc["wifi_rssi"] = 0;
        } else {
            doc["ap_mode"]   = false;
            doc["wifi_ssid"] = WiFi.SSID();
            doc["wifi_ip"]   = WiFi.localIP().toString();
            doc["wifi_mac"]  = WiFi.macAddress();
            doc["wifi_rssi"] = WiFi.RSSI();
        }

        // --- Active partition ---
#ifdef ESP_PLATFORM
        const esp_partition_t* running = esp_ota_get_running_partition();
        if (running != nullptr) {
            doc["ota_running_partition"] = running->label;
        }
#endif

        doc["status"] = "ok";

        String out;
        serializeJson(doc, out);
        esp_err_t err = response->send(200, "application/json", out.c_str());

        activeHighMemRequests--;
        return err;
    });

    // -----------------------------------------------------------------------
    // GET /api/app — Application metadata (title + version)
    // Runtime appName (via Application::setAppName) overrides the compile-time
    // APP_TITLE macro. Consuming apps call app.setAppName("My App") to brand
    // the portal title without modifying platformio.ini build flags.
    // -----------------------------------------------------------------------
    const char* appTitle = _appName ? _appName : APP_TITLE;
    const char* appStylesheet = _appStylesheet;
    activeServer->on("/api/app", HTTP_GET, [appTitle, appStylesheet](PsychicRequest* request, PsychicResponse* response) {
        JsonDocument doc;
        doc["name"]    = appTitle;
        doc["version"] = BUILD_VERSION;
        if (appStylesheet) {
            doc["stylesheet"] = appStylesheet;
        }
        String out;
        serializeJson(doc, out);
        return response->send(200, "application/json", out.c_str());
    });

    // -----------------------------------------------------------------------
    // POST /api/system/restart — Armed restart
    // Sends 200, then schedules esp_restart() after 500ms delay.
    // Body limit: 256 bytes (action-only, minimal body).
    // -----------------------------------------------------------------------
    activeServer->on("/api/system/restart", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        ESP_LOGW(TAG, "POST /api/system/restart — scheduling device restart");
        esp_err_t err = response->send(200, "application/json", "{\"status\":\"restarting\"}");
        _restartRequested = true;
        return err;
    });

    // -----------------------------------------------------------------------
    // POST /api/system/factory-reset — Full erase + restart
    // Erases /mod/ (module configs) and /etc/ (system configs) directories,
    // sends 200, then schedules restart. Body limit: 256 bytes.
    // -----------------------------------------------------------------------
    activeServer->on("/api/system/factory-reset", HTTP_POST, [](PsychicRequest* request, PsychicResponse* response) {
        ESP_LOGW(TAG, "POST /api/system/factory-reset — erasing configuration");

        // --- Using standard POSIX directory reading ---
        DIR* modDir = opendir(ConfigRoot::getModPath());
        if (modDir) {
            struct dirent* ent;
            while ((ent = readdir(modDir)) != NULL) {
                if (ent->d_type == DT_REG) { // is regular file
                    std::string path = std::string(ConfigRoot::getModPath()) + "/" + ent->d_name;
                    unlink(path.c_str());
                    ESP_LOGI(TAG, "Removed: %s", path.c_str());
                }
            }
            closedir(modDir);
        }

        // --- Step 2: Delete config/etc contents (secrets/certs) ---
        DIR* etcDir = opendir(ConfigRoot::getEtcPath());
        if (etcDir) {
            struct dirent* ent;
            while ((ent = readdir(etcDir)) != NULL) {
                if (ent->d_type == DT_REG) { // is regular file
                    std::string path = std::string(ConfigRoot::getEtcPath()) + "/" + ent->d_name;
                    unlink(path.c_str());
                    ESP_LOGI(TAG, "Removed: %s", path.c_str());
                }
            }
            closedir(etcDir);
        }

        ESP_LOGW(TAG, "Factory reset complete — scheduling restart");
        esp_err_t err = response->send(200, "application/json", "{\"status\":\"factory-reset-complete\"}");

        // Schedule restart after response is flushed
        delay(500);
        esp_restart();
        return err;
    });


    // --- Captive portal redirect (AP mode only) / 404 (STA mode) ---
    // When in AP mode, the DNS server resolves ALL domains to 192.168.4.1.
    // Phones/OS probe specific URLs to detect captive portals. If those return
    // anything other than the expected response, the OS pops up a portal window.
    // This catch-all 302 redirect ensures any unhandled request opens the portal.
    //
    // In STA mode the redirect is harmful — it silently masks missing routes
    // instead of returning a visible 404 error.
    activeServer->onNotFound([this](PsychicRequest* request, PsychicResponse* response) {
        if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
            bool useTls = (_authManager && _authManager->isAuthEnabled() && _authManager->hasCert());
            if (useTls) {
                return response->redirect("https://192.168.4.1/");
            } else {
                return response->redirect("http://192.168.4.1/");
            }
        } else {
            return response->send(404, "application/json", "{\"error\":\"Not found\"}");
        }
    });

    if (useTls && _httpsServer) {
        // --- Port 80: redirect-only server (all requests → HTTPS) ---
        _httpServer.onNotFound([](PsychicRequest* req, PsychicResponse* res) {
            String url = "https://";
            url += req->host();
            url += req->url();
            return res->redirect(url.c_str());
        });
        ESP_LOGI(TAG, "HTTPS server configured for port 443; HTTP redirect configured for port 80");
    }
    // -----------------------------------------------------------------------
    // Global Auth Middleware — framework-level portal authentication guard
    // -----------------------------------------------------------------------
    // Registered AFTER all routes so the middleware chain intercepts every
    // request. Non-API paths (static assets, HTML, JS, CSS) pass through.
    // Whitelisted public API paths pass through. /api/mcp paths are excluded
    // because MCPServer handles its own auth via checkMcpAuth().
    // Everything else under /api/ must pass checkPortalAuth().
    //
    // NOTE: /api/system uses EXACT match (==), not prefix match, because
    // /api/system/restart and /api/system/factory-reset are destructive
    // endpoints that MUST require authentication.
    // -----------------------------------------------------------------------
    if (_authManager) {
        AuthManager* authMgr = _authManager;
        activeServer->addMiddleware([authMgr](PsychicRequest* request,
                                              PsychicResponse* response,
                                              PsychicMiddlewareNext next) -> esp_err_t {
            String path = request->uri();

            // --- Pass through: non-API paths (static assets, HTML, etc.) ---
            if (!path.startsWith("/api/")) {
                return next();
            }

            // --- Pass through: whitelisted public API paths ---
            // SECURITY: /api/system uses exact match to prevent
            //           /api/system/restart and /api/system/factory-reset
            //           from bypassing authentication.
            if (path.startsWith("/api/auth/status") ||
                path.startsWith("/api/auth/login")  ||
                path.startsWith("/api/auth/password") ||
                path == "/api/system"                ||
                path.startsWith("/api/tabs")         ||
                path.startsWith("/api/app")          ||
                path.startsWith("/api/mcp")) {
                return next();
            }

            // --- Enforce portal auth on all other /api/ paths ---
            if (!authMgr->checkPortalAuth(request, response)) {
                return ESP_OK;  // 401 already sent by checkPortalAuth()
            }

            return next();
        });
        ESP_LOGI(TAG, "Global auth middleware registered");
    }

    // -----------------------------------------------------------------------
    // Middleware 2: Cache-Control for API responses
    //
    // Browsers may heuristically cache GET responses that lack explicit
    // Cache-Control headers. For dynamic API endpoints this causes stale
    // data — e.g. /api/auth/status cached before login is served after
    // login, missing the mcpToken field. Prevent this globally.
    // -----------------------------------------------------------------------
    activeServer->addMiddleware([](PsychicRequest* request,
                                   PsychicResponse* response,
                                   PsychicMiddlewareNext next) -> esp_err_t {
        String path = request->uri();
        if (path.startsWith("/api/")) {
            response->addHeader("Cache-Control", "no-store");
        }
        return next();
    });

    // -----------------------------------------------------------------------
    // Global POST/PUT Body-Size Middleware — ADR-15 enforcement
    // -----------------------------------------------------------------------
    // Each IWebContributor declares its own max body size via getMaxPostBody()
    // and its API path prefix via getApiPrefix(). The middleware matches the
    // incoming URI against the registry (longest prefix first) and enforces
    // the contributor's declared limit. Unmatched paths get DEFAULT_MAX_POST_BODY.
    //
    // NOTE: PsychicHttp may buffer the body before this middleware runs.
    // The contentLength() check protects against well-behaved clients but
    // cannot prevent OOM from a malicious client lying about Content-Length.
    // This is the same constraint existing per-route guards have.
    // -----------------------------------------------------------------------
    activeServer->addMiddleware([](PsychicRequest* request,
                                   PsychicResponse* response,
                                   PsychicMiddlewareNext next) -> esp_err_t {
        if (request->method() == HTTP_POST || request->method() == HTTP_PUT) {
            String path = request->uri();
            size_t limit = DEFAULT_MAX_POST_BODY;

            // Longest-prefix-first lookup
            for (const auto& entry : s_bodyLimits) {
                if (path.startsWith(entry.prefix)) {
                    limit = entry.maxBody;
                    break;
                }
            }

            if (request->contentLength() > limit) {
                // Build JSON response with the specific limit for debuggability
                char body[64];
                snprintf(body, sizeof(body),
                    "{\"error\":\"Payload too large\",\"max\":%u}",
                    (unsigned)limit);
                response->send(413, "application/json", body);
                return ESP_OK;  // ESP_OK so PsychicHttp flushes the response
            }
        }
        return next();
    });
    ESP_LOGI(TAG, "Global body-size middleware registered (default=%u, %zu overrides)",
             (unsigned)DEFAULT_MAX_POST_BODY, (size_t)s_bodyLimits.size());

    ESP_LOGI(TAG, "WebServer configured (TLS=%d, AP=%d), registered %u contributors",
             useTls, apMode, (unsigned)contributors.size());
}

void WebServerManager::tick(uint32_t currentMillis) {
    if (!_serverStarted && _wifiManager && _wifiManager->isReady()) {
        _httpServer.begin();
        if (_tlsMode && _httpsServer) {
            _httpsServer->begin();
            ESP_LOGI(TAG, "HTTPS server started on port 443; HTTP redirect on port 80");
        } else {
            ESP_LOGI(TAG, "HTTP server started on port 80");
        }
        _serverStarted = true;
    }

    // --- Periodic httpd task stack high-water mark telemetry (#119) ---
    // The httpd task is created internally by PsychicHttp / ESP-IDF with
    // the name "httpd". Log every 60s to track headroom on constrained
    // boards without spamming the console.
#ifdef ESP_PLATFORM
    if (_serverStarted) {
        static uint32_t lastHwmLogMs = 0;
        if (lastHwmLogMs == 0 || (currentMillis - lastHwmLogMs) >= 60000) {
            TaskHandle_t httpdTask = xTaskGetHandle("httpd");
            if (httpdTask != nullptr) {
                UBaseType_t hwm = uxTaskGetStackHighWaterMark(httpdTask);
                ESP_LOGI(TAG, "httpd stack HWM: %u words (%u bytes free)",
                         (unsigned)hwm, (unsigned)(hwm * 4));
            }
            lastHwmLogMs = currentMillis;
        }
    }
#endif

    if (_restartRequested) {
        ESP_LOGI(TAG, "Restart requested. Checking certificate generation status...");
        if (_authManager) {
            uint32_t startMs = millis();
            while (_authManager->isCertGenerating() && (millis() - startMs < 10000)) {
                esp_task_wdt_reset();
                delay(100);
            }
        }
        ESP_LOGI(TAG, "Ending web servers...");
        if (_httpsServer) {
            _httpsServer->end();
        }
        _httpServer.end();
        delay(200);
        ESP_LOGW(TAG, "Calling esp_restart() now");
        esp_restart();
    }
}

PsychicHttpServer& WebServerManager::getServer() {
    if (_tlsMode && _httpsServer) return *_httpsServer;
    return _httpServer;
}
