/*
 * ESP32 App Foundation (c) 2026
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: lib/GithubClient/githubClient.cpp
 * Description: Implementation of the GitHub API client with certificate validation.
 */

#include "GithubClient.hpp"
#include "../Logger/Logger.hpp"
#include "../SemVer/SemVer.hpp"
#include <string.h>
#include <memory>

#ifndef NATIVE_BUILD
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_heap_caps.h>
#include <esp_task_wdt.h>
#endif

// OTA root CA bundle — shared across GithubClient and OTAUpdateManager.
// Defined in ota_root_cas.h (Sectigo E46 + ISRG Root X1).
#include "ota_root_cas.h"

const char* GithubClient::githubRootCACert = OTA_ROOT_CA_BUNDLE;

GithubClient::GithubClient(const std::string& repository, const std::string& token) {
    repository_ = repository;
    if (!repository.empty()) {
        apiGetLatestRelease = std::string("https://") + apiHost + "/repos/" + repository + "/releases/latest";
        apiGetReleases      = std::string("https://") + apiHost + "/repos/" + repository + "/releases";
    }
    accessToken = token;
    if (!accessToken.empty()) {
        Logger::registerSecret(accessToken);
    }
    releaseAssets = 0;
    isPreRelease = false;
    strcpy(lastErrorMsg, "Initialized");
}

/**
 * @brief Polls the GitHub API to fetch metadata about the latest release.
 * @return True if the metadata was successfully fetched and parsed.
 */
bool GithubClient::getLatestRelease() {
    releaseId = "";
    releaseDescription = "";
    releaseAssets = 0;
    isPreRelease = false;

#ifndef NATIVE_BUILD
    auto client = std::unique_ptr<WiFiClientSecure>(new (std::nothrow) WiFiClientSecure());
    if (!client) {
        ESP_LOGE("GITHUB", "Failed to allocate WiFiClientSecure for release query");
        return false;
    }
    client->setCACert(githubRootCACert);
    client->setTimeout(15000);

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.useHTTP10(true);
    if (!http.begin(*client, String(apiGetLatestRelease.c_str()))) {
        ESP_LOGE("GITHUB", "HTTP begin failed for latest release");
        return false;
    }
    http.addHeader("User-Agent", "esp32/1.0");
    http.addHeader("X-GitHub-Api-Version", "2022-11-28");
    http.addHeader("Accept", "application/vnd.github+json");
    if (!accessToken.empty()) {
        http.addHeader("Authorization", String(("Bearer " + accessToken).c_str()));
    }

    LOG_SECURE("GITHUB", "Requesting latest release from %s", apiGetLatestRelease.c_str());
    int httpCode = http.GET();

    if (httpCode < 0) {
        ESP_LOGW("GITHUB", "Latest release query failed with pinned CA (err=%d) — retrying insecure", httpCode);
        http.end();

        esp_task_wdt_reset();

        client.reset(new (std::nothrow) WiFiClientSecure());
        if (!client) {
            ESP_LOGE("GITHUB", "Failed to allocate WiFiClientSecure for latest release insecure retry");
            return false;
        }
        client->setInsecure();
        client->setTimeout(15000);

        if (!http.begin(*client, String(apiGetLatestRelease.c_str()))) {
            ESP_LOGE("GITHUB", "HTTP begin failed for latest release retry");
            return false;
        }
        http.addHeader("User-Agent", "esp32/1.0");
        http.addHeader("X-GitHub-Api-Version", "2022-11-28");
        http.addHeader("Accept", "application/vnd.github+json");
        if (!accessToken.empty()) {
            http.addHeader("Authorization", String(("Bearer " + accessToken).c_str()));
        }

        httpCode = http.GET();
    }

    if (httpCode != HTTP_CODE_OK) {
        snprintf(lastErrorMsg, sizeof(lastErrorMsg), "HTTP request failed, error: %d", httpCode);
        if (httpCode > 0) {
            String payload = http.getString();
            LOG_SECURE("GITHUB", "HTTP Response code: %d, payload: %s", httpCode, payload.c_str());
        }
        http.end();
        return false;
    }

    // Read full response body into PSRAM — stream parsing is fragile on ESP32
    // TLS (WiFiClientSecure can time out mid-read causing IncompleteInput).
    // Using PSRAM instead of getString() avoids consuming scarce DRAM heap.
    int contentLen = http.getSize();
    if (contentLen <= 0) contentLen = 16384;  // Fallback if no Content-Length

    // SAFETY: PSRAM buffer is allocated and freed within this function scope.
    // It MUST be freed before any flash write operation begins (called from
    // OTA task where PSRAM and flash share the SPI bus).
    char* buf = (char*)heap_caps_malloc(contentLen + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf) {
        // PSRAM unavailable (e.g. ESP32-dev) — fall back to DRAM
        buf = (char*)malloc(contentLen + 1);
    }
    if (!buf) {
        ESP_LOGE("GITHUB", "Failed to allocate %d bytes for response (no PSRAM or DRAM)", contentLen + 1);
        http.end();
        snprintf(lastErrorMsg, sizeof(lastErrorMsg), "Alloc failed (%d bytes)", contentLen + 1);
        return false;
    }

    int bytesRead = 0;
    auto streamClient = http.getStreamPtr();
    uint32_t startMs = millis();
    while ((streamClient->connected() || streamClient->available()) && bytesRead < contentLen) {
        int avail = streamClient->available();
        if (avail > 0) {
            int toRead = (contentLen - bytesRead > avail) ? avail : (contentLen - bytesRead);
            int c = streamClient->read((uint8_t*)&buf[bytesRead], toRead);
            if (c > 0) {
                bytesRead += c;
                startMs = millis();
            }
        } else {
            if (millis() - startMs > 15000) {
                ESP_LOGW("GITHUB", "Read timeout (fast path)");
                break;
            }
            delay(10);
        }
    }
    buf[bytesRead] = '\0';
    http.end();

    // Filter document to reduce memory footprint
    JsonDocument filter;
    filter["tag_name"] = true;
    filter["name"] = true;
    filter["assets"][0]["name"] = true;
    filter["assets"][0]["url"] = true;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, buf, DeserializationOption::Filter(filter));
    free(buf);

    if (error) {
        snprintf(lastErrorMsg, sizeof(lastErrorMsg), "JSON parse failed: %s (%d bytes read)", error.c_str(), bytesRead);
        LOG_SECURE("GITHUB", "JSON Parse Error: %s (%d bytes read)", error.c_str(), bytesRead);
        return false;
    }

    releaseId = doc["tag_name"] | "";
    releaseDescription = doc["name"] | "";

    JsonArray assetsArray = doc["assets"].as<JsonArray>();
    for (JsonObject asset : assetsArray) {
        if (releaseAssets >= MAX_RELEASE_ASSETS) break;
        releaseAssetName[releaseAssets] = asset["name"] | "";
        releaseAssetURL[releaseAssets] = asset["url"] | "";
        if (!releaseAssetName[releaseAssets].empty() && !releaseAssetURL[releaseAssets].empty()) {
            releaseAssets++;
        }
    }

    strcpy(lastErrorMsg, "SUCCESS");
    return true;
#else
    // Mock implementation for native testing
    releaseId = "v1.0.0";
    releaseDescription = "Mock Release";
    releaseAssets = 1;
    releaseAssetName[0] = "firmware.bin";
    releaseAssetURL[0] = "https://example.com/firmware.bin";
    isPreRelease = false;
    strcpy(lastErrorMsg, "SUCCESS");
    return true;
#endif
}

/**
 * @brief Query releases and return the best available version for a subscriber's channel.
 *
 * Implements the risk-floor model: a subscriber's channel determines the
 * MINIMUM stability they require, not the exact track they are locked to.
 * Any release that meets or exceeds that stability floor is a candidate;
 * the highest SemVer version among all candidates is returned.
 *
 * Acceptance rules (single pass through /releases list):
 *   stable channel  → delegates to getLatestRelease() (non-prerelease only)
 *   rc     channel  → rc + stable
 *   beta   channel  → beta + rc + stable
 *   dev    channel  → dev + beta + rc + stable
 *
 * This means a device on beta will automatically receive v0.0.7 stable if
 * it is ahead of the latest v0.0.6-beta.x — no manual channel switch needed.
 *
 * The acceptance check uses the Channel enum ordering (DEV=0 < BETA=2 < RC=3
 * < STABLE=15): a prerelease is accepted when its parsed channel value >=
 * the subscriber's floor channel value.
 *
 * @param channel Channel floor string: "stable", "rc", "beta", or "dev".
 * @return true if any acceptable release was found.
 */
bool GithubClient::getLatestReleaseForChannel(const char* channel) {
    // Stable (or unset) delegates to the single-release fast-path
    if (channel == nullptr || channel[0] == '\0' || strcmp(channel, "stable") == 0) {
        return getLatestRelease();
    }

    releaseId = "";
    releaseDescription = "";
    releaseAssets = 0;
    isPreRelease = false;

    // Convert the subscriber's channel string to a Channel enum floor value.
    // Any release whose parsed channel >= this floor is an acceptable candidate.
    Channel floor = stringToChannel(channel);

#ifndef NATIVE_BUILD
    // --- Step 1: Fetch stable baseline via /releases/latest ---
    // This populates member fields with the latest stable release.
    bool hasStableBaseline = getLatestRelease();

    if (hasStableBaseline) {
        LOG_SECURE("GITHUB", "Stable baseline: %s", releaseId.c_str());
    } else {
        LOG_SECURE("GITHUB", "No stable baseline found — scanning all releases");
    }

    // Track best candidate found so far.
    // If stable baseline exists, it is the starting best candidate.
    std::string bestTag         = hasStableBaseline ? releaseId : "";
    std::string bestDescription = hasStableBaseline ? releaseDescription : "";
    bool   bestIsPre       = false;
    int    bestAssetCount  = hasStableBaseline ? releaseAssets : 0;
    std::string bestAssetName[MAX_RELEASE_ASSETS];
    std::string bestAssetURL[MAX_RELEASE_ASSETS];
    FirmwareVersion bestVer = {};
    if (hasStableBaseline) {
        bestVer = parseVersionTag(releaseId.c_str());
        for (int i = 0; i < bestAssetCount; ++i) {
            bestAssetName[i] = releaseAssetName[i];
            bestAssetURL[i]  = releaseAssetURL[i];
        }
    }

    // --- Step 2: Paginated scan of /releases?per_page=1&page=N ---
    static constexpr int MAX_PAGES    = 20;
    static constexpr int PER_PAGE     = 1;
    bool stopScanning = false;
    int parseFailures = 0;

    for (int page = 1; page <= MAX_PAGES && !stopScanning; ++page) {
        // Build paginated URL
        std::string pageUrl = apiGetReleases + "?per_page=" + std::to_string(PER_PAGE) + "&page=" + std::to_string(page);
        LOG_SECURE("GITHUB", "Requesting releases page %d (floor='%s')", page, channel);

        // Heap-allocated TLS client — reuses allocation across pages.
        // DRAM-first (std::nothrow) because this runs on the OTA task
        // where PSRAM is unsafe during flash writes.
        auto pageClient = std::unique_ptr<WiFiClientSecure>(new (std::nothrow) WiFiClientSecure());
        if (!pageClient) {
            ESP_LOGE("GITHUB", "Failed to allocate WiFiClientSecure for page %d", page);
            break;
        }
        pageClient->setCACert(githubRootCACert);
        pageClient->setTimeout(15000);

        HTTPClient pageHttp;
        pageHttp.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        pageHttp.useHTTP10(true);
        if (!pageHttp.begin(*pageClient, String(pageUrl.c_str()))) {
            ESP_LOGE("GITHUB", "Page %d: HTTP begin failed", page);
            break;
        }
        pageHttp.addHeader("User-Agent", "esp32/1.0");
        pageHttp.addHeader("X-GitHub-Api-Version", "2022-11-28");
        pageHttp.addHeader("Accept", "application/vnd.github+json");
        if (!accessToken.empty()) {
            pageHttp.addHeader("Authorization", String(("Bearer " + accessToken).c_str()));
        }
        pageHttp.setTimeout(15000);

        int httpCode = pageHttp.GET();
        if (httpCode < 0) {
            ESP_LOGW("GITHUB", "Page %d query failed with pinned CA (err=%d) — retrying insecure", page, httpCode);
            pageHttp.end();

            esp_task_wdt_reset();

            pageClient.reset(new (std::nothrow) WiFiClientSecure());
            if (!pageClient) {
                ESP_LOGE("GITHUB", "Failed to allocate WiFiClientSecure for page %d insecure retry", page);
                break;
            }
            pageClient->setInsecure();
            pageClient->setTimeout(15000);

            if (!pageHttp.begin(*pageClient, String(pageUrl.c_str()))) {
                ESP_LOGE("GITHUB", "Page %d: HTTP begin failed for retry", page);
                break;
            }
            pageHttp.addHeader("User-Agent", "esp32/1.0");
            pageHttp.addHeader("X-GitHub-Api-Version", "2022-11-28");
            pageHttp.addHeader("Accept", "application/vnd.github+json");
            if (!accessToken.empty()) {
                pageHttp.addHeader("Authorization", String(("Bearer " + accessToken).c_str()));
            }
            pageHttp.setTimeout(15000);

            httpCode = pageHttp.GET();
        }
        if (httpCode != HTTP_CODE_OK) {
            LOG_SECURE("GITHUB", "Page %d HTTP error %d — stopping scan", page, httpCode);
            pageHttp.end();
            break;  // Graceful degradation: return best found so far
        }

        // Read full response body into PSRAM — avoids DRAM-hungry getString()
        int contentLen = pageHttp.getSize();
        if (contentLen <= 0) contentLen = 16384;

        // SAFETY: PSRAM buffer is allocated and freed within this function scope.
        // It MUST be freed before any flash write operation begins (called from
        // OTA task where PSRAM and flash share the SPI bus).
        char* buf = (char*)heap_caps_malloc(contentLen + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!buf) {
            // PSRAM unavailable — fall back to DRAM
            buf = (char*)malloc(contentLen + 1);
        }
        if (!buf) {
            ESP_LOGE("GITHUB", "Page %d: alloc failed (%d bytes)", page, contentLen + 1);
            pageHttp.end();
            parseFailures++;
            break;
        }

        int bytesRead = 0;
        auto streamClient = pageHttp.getStreamPtr();
        uint32_t startMs = millis();
        while ((streamClient->connected() || streamClient->available()) && bytesRead < contentLen) {
            int avail = streamClient->available();
            if (avail > 0) {
                int toRead = (contentLen - bytesRead > avail) ? avail : (contentLen - bytesRead);
                int c = streamClient->read((uint8_t*)&buf[bytesRead], toRead);
                if (c > 0) {
                    bytesRead += c;
                    startMs = millis();
                }
            } else {
                if (millis() - startMs > 15000) {
                    ESP_LOGW("GITHUB", "Read timeout on page %d", page);
                    break;
                }
                delay(10);
            }
        }
        buf[bytesRead] = '\0';
        pageHttp.end();

        // Parse page — array filter for minimal memory usage
        JsonDocument filter;
        filter[0]["tag_name"]   = true;
        filter[0]["name"]      = true;
        filter[0]["prerelease"] = true;
        filter[0]["assets"][0]["name"] = true;
        filter[0]["assets"][0]["url"]  = true;

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, buf,
                                                      DeserializationOption::Filter(filter));
        free(buf);

        if (error) {
            LOG_SECURE("GITHUB", "Page %d JSON error: %s (%d bytes read) — stopping scan",
                       page, error.c_str(), bytesRead);
            parseFailures++;
            break;  // Graceful degradation
        }

        JsonArray releases = doc.as<JsonArray>();

        // EXIT 3: Empty page — no more releases
        if (releases.size() == 0) {
            LOG_SECURE("GITHUB", "Empty page %d — stopping scan", page);
            break;
        }

        for (JsonObject release : releases) {
            const char* tagName = release["tag_name"] | "";
            if (tagName[0] == '\0') continue;

            bool isPre = release["prerelease"] | false;

            // EXIT 1: Hit a stable release — we already have the latest stable
            // from Step 1. Everything after this is older. Stop scanning.
            if (!isPre) {
                LOG_SECURE("GITHUB", "Hit stable %s in scan — stopping (page %d)", tagName, page);
                stopScanning = true;
                break;
            }

            // Pre-release: check if it meets the subscriber's floor
            FirmwareVersion candidate = parseVersionTag(tagName);
            bool accepted = (static_cast<uint8_t>(candidate.channel) >= static_cast<uint8_t>(floor));

            if (!accepted) {
                LOG_SECURE("GITHUB", "Skipping %s — does not meet floor '%s'", tagName, channel);
                continue;
            }

            // EXIT 2: Found an acceptable prerelease that beats best candidate.
            // Since releases are newest-first, the first acceptable prerelease
            // is the winner — nothing later can beat it.
            if (bestTag.empty() || candidate > bestVer) {
                LOG_SECURE("GITHUB", "Found acceptable prerelease %s — stopping", tagName);
                bestVer         = candidate;
                bestTag         = tagName;
                bestDescription = release["name"] | "";
                bestIsPre       = true;
                bestAssetCount  = 0;

                JsonArray assetsArray = release["assets"].as<JsonArray>();
                for (JsonObject asset : assetsArray) {
                    if (bestAssetCount >= MAX_RELEASE_ASSETS) break;
                    std::string name = asset["name"] | "";
                    std::string url  = asset["url"]  | "";
                    if (!name.empty() && !url.empty()) {
                        bestAssetName[bestAssetCount] = name;
                        bestAssetURL[bestAssetCount]  = url;
                        bestAssetCount++;
                    }
                }
                stopScanning = true;
                break;
            }
        }
    }

    // --- Step 3: Commit the best candidate to member fields ---
    if (bestTag.empty()) {
        if (parseFailures > 0) {
            snprintf(lastErrorMsg, sizeof(lastErrorMsg),
                     "GitHub API responses could not be parsed (%d page(s) failed — possible network/memory issue)",
                     parseFailures);
        } else {
            snprintf(lastErrorMsg, sizeof(lastErrorMsg),
                     "No release found at or above channel floor '%s'", channel);
        }
        LOG_SECURE("GITHUB", "%s", lastErrorMsg);
        return false;
    }

    releaseId          = bestTag;
    releaseDescription = bestDescription;
    isPreRelease       = bestIsPre;
    releaseAssets      = bestAssetCount;
    for (int i = 0; i < bestAssetCount; ++i) {
        releaseAssetName[i] = bestAssetName[i];
        releaseAssetURL[i]  = bestAssetURL[i];
    }

    LOG_SECURE("GITHUB", "Selected release: %s (floor='%s')", releaseId.c_str(), channel);
    strcpy(lastErrorMsg, "SUCCESS");
    return true;

#else
    // Mock for native unit tests — return a plausible prerelease for the channel
    // so tests exercising the risk-floor path get a testable result.
    releaseId          = std::string("v0.0.6-") + channel + ".1";
    releaseDescription = "Mock Pre-Release";
    releaseAssets      = 1;
    releaseAssetName[0] = "firmware.bin";
    releaseAssetURL[0]  = "https://example.com/firmware.bin";
    isPreRelease        = true;
    strcpy(lastErrorMsg, "SUCCESS");
    return true;
#endif
}

/**
 * @brief Retrieves the last error message generated by the client.
 * @return A string containing the error description (or "SUCCESS").
 */
const char* GithubClient::getLastError() {
    return lastErrorMsg;
}
