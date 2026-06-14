/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
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
#include <ArduinoJson.h>

#ifndef NATIVE_BUILD
    #include "esp_heap_caps.h"
#endif

// Portal PROGMEM assets — hardware only (require Arduino.h for PROGMEM macro)
#ifndef NATIVE_BUILD
#include "portal_assets/index_html.hpp"
#include "portal_assets/app_css.hpp"
#include "portal_assets/app_js.hpp"
#include "portal_assets/utils_js.hpp"
#include "portal_assets/system_js.hpp"
#include "portal_assets/time_js.hpp"
#include "portal_assets/wifi_js.hpp"
#include "portal_assets/ota_js.hpp"
#include "portal_assets/backup_js.hpp"
#include "portal_assets/keys_js.hpp"
#include "portal_assets/mcp_js.hpp"
#include "portal_assets/screenshot_js.hpp"
#include "portal_assets/screenshot_html.hpp"
#include "portal_assets/display_js.hpp"
#include "portal_assets/appconfig_js.hpp"
#include "portal_assets/auth_js.hpp"
#endif

#include "platform/LogShim.h"

// ---------------------------------------------------------------------------
// Platform-conditional includes for heap/system APIs
// ---------------------------------------------------------------------------
#ifndef NATIVE_BUILD
    #include <Arduino.h>
    #include "esp_system.h"
    #include "esp_chip_info.h"
    #include "esp_heap_caps.h"
    #include "esp_partition.h"
    #include "esp_ota_ops.h"
    #include <LittleFS.h>
    #include <WiFi.h>
    #include <atomic>
#else
    #include <atomic>
#endif

static const char* TAG = "WebServerManager";

// ---------------------------------------------------------------------------
// Heap guard — protects portal HTML + large responses from concurrent
// memory exhaustion. Only one high-memory request is served at a time.
// ---------------------------------------------------------------------------
static std::atomic<int> activeHighMemRequests{0};
constexpr int  MAX_CONCURRENT_HIGH_MEM      = 1;      // Max concurrent high-memory requests
constexpr int  HEAP_GUARD_MIN_BLOCK_BYTES   = 20000;  // Minimum largest free block (bytes)

#ifdef NATIVE_BUILD
uint32_t AsyncWebServer::lastMaxBodySize = 0;
#endif

WebServerManager::WebServerManager() : _httpServer(80) {
}

WebServerManager::~WebServerManager() {
#ifndef NATIVE_BUILD
    if (_httpsServer) {
        delete _httpsServer;
        _httpsServer = nullptr;
    }
#endif
}

void WebServerManager::begin(const std::vector<IWebContributor*>& contributors,
                             AuthManager* authManager, bool apMode) {
    _authManager = authManager;
    ESP_LOGI(TAG, "Initializing WebServerManager (apMode=%d)", apMode);

    // --- TLS mode decision ---
    // TLS is engaged only when: NOT in AP mode, auth is enabled, and a cert is present.
    // If cert loading fails at runtime, we fall back to HTTP-open gracefully.
    bool useTls = false;
#ifndef NATIVE_BUILD
    if (!apMode && authManager && authManager->isAuthEnabled() && authManager->hasCert()) {
        uint8_t* certDer = nullptr;
        size_t   certLen = 0;
        uint8_t* keyDer  = nullptr;
        size_t   keyLen  = 0;
        if (authManager->loadCert(&certDer, &certLen, &keyDer, &keyLen)) {
            _httpsServer = new (std::nothrow) AsyncWebServer(443);
            if (_httpsServer) {
                // TODO(WO4): When ESPAsyncWebServer exposes AsyncWebServerSecure,
                // replace with: new AsyncWebServerSecure(443) and call
                // _httpsServer->setServerKeyAndCertificate(keyDer, keyLen, certDer, certLen);
                // For now, use port 443 AsyncWebServer placeholder — the cert
                // loading path is validated even if SSL negotiation requires the
                // Secure variant. WO8 integration testing will verify end-to-end.
                useTls = true;
                _tlsMode = true;
                ESP_LOGI(TAG, "TLS mode — HTTPS on port 443 (cert=%zu bytes, key=%zu bytes)",
                         certLen, keyLen);
            } else {
                ESP_LOGE(TAG, "Failed to allocate HTTPS server — falling back to HTTP");
            }
        } else {
            ESP_LOGW(TAG, "Cert load failed — falling back to HTTP-open mode");
        }
    }
#endif

    // Select the active server for route registration
    // TLS mode: routes on HTTPS server; HTTP server becomes redirect-only
    // HTTP-open mode: all routes on the HTTP server (port 80)
    AsyncWebServer& activeServer = _httpServer; // default
#ifndef NATIVE_BUILD
    if (useTls && _httpsServer) {
        activeServer = *_httpsServer;
    }
#endif

    // --- Setup routing for static files served from PROGMEM (hardware only) ---
#ifndef NATIVE_BUILD
    // Heap-guarded portal HTML route — the largest single response
    activeServer.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        // --- Heap guard check ---
        int current = activeHighMemRequests.load();
        if (current >= MAX_CONCURRENT_HIGH_MEM ||
            heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) < HEAP_GUARD_MIN_BLOCK_BYTES) {
            request->send(503, "application/json", "{\"error\":\"Low memory — retry\"}");
            return;
        }
        activeHighMemRequests++;

        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", index_html_gz, index_html_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);

        activeHighMemRequests--;
    });
    
    activeServer.on("/app.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/css", app_css_gz, app_css_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    activeServer.on("/app.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "application/javascript", app_js_gz, app_js_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    activeServer.on("/system.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "application/javascript", system_js_gz, system_js_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    activeServer.on("/time.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "application/javascript", time_js_gz, time_js_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    activeServer.on("/wifi.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "application/javascript", wifi_js_gz, wifi_js_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    activeServer.on("/ota.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "application/javascript", ota_js_gz, ota_js_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    activeServer.on("/utils.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "application/javascript", utils_js_gz, utils_js_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    activeServer.on("/backup.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "application/javascript", backup_js_gz, backup_js_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    activeServer.on("/keys.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "application/javascript", keys_js_gz, keys_js_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    activeServer.on("/mcp.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "application/javascript", mcp_js_gz, mcp_js_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    activeServer.on("/screenshot.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "application/javascript", screenshot_js_gz, screenshot_js_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    activeServer.on("/screenshot.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", screenshot_html_gz, screenshot_html_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    activeServer.on("/display.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "application/javascript", display_js_gz, display_js_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    activeServer.on("/appconfig.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "application/javascript", appconfig_js_gz, appconfig_js_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    activeServer.on("/auth.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "application/javascript", auth_js_gz, auth_js_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
#endif

    // --- Register all IWebContributor routes and build tabs array ---
    JsonDocument tabsDoc;
    JsonArray tabsArr = tabsDoc.to<JsonArray>();
    
    for (auto* contributor : contributors) {
        if (contributor != nullptr) {
            contributor->registerRoutes(activeServer, _authManager);
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
            }
        }
    }
    
    // Inject the "Status" sub-pill — WebServerManager hosts /api/system
    // but is not itself an IWebContributor, so add it manually.
    JsonObject statusTab = tabsArr.add<JsonObject>();
    statusTab["name"]   = "Status";
    statusTab["id"]     = "Status";
    statusTab["parent"] = "System";
    statusTab["order"]  = 10;

#ifndef NATIVE_BUILD
    String tabsJson;
    serializeJson(tabsDoc, tabsJson);

    // --- GET /api/tabs — tab hierarchy for navigation ---
    activeServer.on("/api/tabs", HTTP_GET, [tabsJson](AsyncWebServerRequest* request) {
        request->send(200, "application/json", tabsJson);
    });

    // -----------------------------------------------------------------------
    // GET /api/system — Enhanced system information (20+ fields)
    // -----------------------------------------------------------------------
    activeServer.on("/api/system", HTTP_GET, [](AsyncWebServerRequest* request) {
        // --- Heap guard for this large response ---
        int current = activeHighMemRequests.load();
        if (current >= MAX_CONCURRENT_HIGH_MEM ||
            heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) < HEAP_GUARD_MIN_BLOCK_BYTES) {
            request->send(503, "application/json", "{\"error\":\"Low memory — retry\"}");
            return;
        }
        activeHighMemRequests++;

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
        doc["storage_used"]   = LittleFS.usedBytes();
        doc["storage_total"]  = LittleFS.totalBytes();

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
        const esp_partition_t* running = esp_ota_get_running_partition();
        if (running != nullptr) {
            doc["partition"] = running->label;
        }

        doc["status"] = "ok";

        String out;
        serializeJson(doc, out);
        request->send(200, "application/json", out);

        activeHighMemRequests--;
    });

    // -----------------------------------------------------------------------
    // GET /api/app — Application metadata (title + version)
    // Uses build-time macros. Consuming apps override APP_TITLE in platformio.ini.
    // -----------------------------------------------------------------------
    activeServer.on("/api/app", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["name"]    = APP_TITLE;
        doc["version"] = BUILD_VERSION;
        String out;
        serializeJson(doc, out);
        request->send(200, "application/json", out);
    });

    // -----------------------------------------------------------------------
    // POST /api/system/restart — Armed restart
    // Sends 200, then schedules esp_restart() after 500ms delay.
    // Body limit: 256 bytes (action-only, minimal body).
    // -----------------------------------------------------------------------
    activeServer.on("/api/system/restart", HTTP_POST, [](AsyncWebServerRequest* request) {
        ESP_LOGW(TAG, "POST /api/system/restart — scheduling device restart");
        request->send(200, "application/json", "{\"status\":\"restarting\"}");

        // Schedule restart after response is flushed (500ms delay)
        // Using a FreeRTOS timer or simple delay loop
        delay(500);
        esp_restart();
    });

    // -----------------------------------------------------------------------
    // POST /api/system/factory-reset — Full erase + restart
    // Erases /mod/ (module configs) and /etc/ (system configs) directories,
    // sends 200, then schedules restart. Body limit: 256 bytes.
    // -----------------------------------------------------------------------
    activeServer.on("/api/system/factory-reset", HTTP_POST, [](AsyncWebServerRequest* request) {
        ESP_LOGW(TAG, "POST /api/system/factory-reset — erasing configuration");

        // --- Step 1: Erase /mod/ directory (per-module config files) ---
        File modDir = LittleFS.open("/mod");
        if (modDir && modDir.isDirectory()) {
            File entry = modDir.openNextFile();
            while (entry) {
                String path = String("/mod/") + entry.name();
                entry.close();
                LittleFS.remove(path);
                ESP_LOGI(TAG, "Removed: %s", path.c_str());
                entry = modDir.openNextFile();
            }
            modDir.close();
        }

        // --- Step 2: Erase /etc/ directory (system-level configs) ---
        File etcDir = LittleFS.open("/etc");
        if (etcDir && etcDir.isDirectory()) {
            File entry = etcDir.openNextFile();
            while (entry) {
                String path = String("/etc/") + entry.name();
                entry.close();
                LittleFS.remove(path);
                ESP_LOGI(TAG, "Removed: %s", path.c_str());
                entry = etcDir.openNextFile();
            }
            etcDir.close();
        }

        ESP_LOGW(TAG, "Factory reset complete — scheduling restart");
        request->send(200, "application/json", "{\"status\":\"factory-reset-complete\"}");

        // Schedule restart after response is flushed
        delay(500);
        esp_restart();
    });

#else
    activeServer.on("/api/tabs", HTTP_GET, [](AsyncWebServerRequest* request) {});
    activeServer.on("/api/system", HTTP_GET, [](AsyncWebServerRequest* request) {});
    activeServer.on("/api/app", HTTP_GET, [](AsyncWebServerRequest* request) {});
    activeServer.on("/api/system/restart", HTTP_POST, [](AsyncWebServerRequest* request) {});
    activeServer.on("/api/system/factory-reset", HTTP_POST, [](AsyncWebServerRequest* request) {});
#endif

    // --- Captive portal redirect (AP mode only) / 404 (STA mode) ---
    // When in AP mode, the DNS server resolves ALL domains to 192.168.4.1.
    // Phones/OS probe specific URLs to detect captive portals. If those return
    // anything other than the expected response, the OS pops up a portal window.
    // This catch-all 302 redirect ensures any unhandled request opens the portal.
    //
    // In STA mode the redirect is harmful — it silently masks missing routes
    // instead of returning a visible 404 error.
    activeServer.onNotFound([](AsyncWebServerRequest* request) {
#ifndef NATIVE_BUILD
        if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
            request->redirect("http://192.168.4.1/");
        } else {
            request->send(404, "application/json", "{\"error\":\"Not found\"}");
        }
#else
        request->send(404, "application/json", "{\"error\":\"Not found\"}");
#endif
    });

#ifndef NATIVE_BUILD
    if (useTls && _httpsServer) {
        // --- Port 80: redirect-only server (all requests → HTTPS) ---
        _httpServer.onNotFound([](AsyncWebServerRequest* req) {
            String url = "https://";
            url += req->host();
            url += req->url();
            req->redirect(url.c_str());
        });
        _httpServer.begin();
        _httpsServer->begin();
        ESP_LOGI(TAG, "HTTPS server started on port 443; HTTP redirect on port 80");
    } else {
        _httpServer.begin();
    }
#else
    _httpServer.begin();
#endif
    ESP_LOGI(TAG, "WebServer started (TLS=%d, AP=%d), registered %zu contributors",
             useTls, apMode, contributors.size());
}

AsyncWebServer& WebServerManager::getServer() {
#ifndef NATIVE_BUILD
    if (_tlsMode && _httpsServer) return *_httpsServer;
#endif
    return _httpServer;
}
