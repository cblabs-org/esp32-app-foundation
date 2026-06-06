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

#ifndef NATIVE_BUILD
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#endif

// Sectigo Public Server Authentication Root E46
// (cross-signed by USERTrust ECC Certification Authority)
// Valid until: Jan 18 2038. GitHub migrated from DigiCert to Sectigo in 2026.
const char* GithubClient::githubRootCACert = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDRjCCAsugAwIBAgIQGp6v7G3o4ZtcGTFBto2Q3TAKBggqhkjOPQQDAzCBiDEL\n" \
"MAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNl\n" \
"eSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMT\n" \
"JVVTRVJUcnVzdCBFQ0MgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMjEwMzIy\n" \
"MDAwMDAwWhcNMzgwMTE4MjM1OTU5WjBfMQswCQYDVQQGEwJHQjEYMBYGA1UEChMP\n" \
"U2VjdGlnbyBMaW1pdGVkMTYwNAYDVQQDEy1TZWN0aWdvIFB1YmxpYyBTZXJ2ZXIg\n" \
"QXV0aGVudGljYXRpb24gUm9vdCBFNDYwdjAQBgcqhkjOPQIBBgUrgQQAIgNiAAR2\n" \
"+pmpbiDt+dd34wc7qNs9Xzjoq1WmVk/WSOrsfy2qw7LFeeyZYX8QeccCWvkEN/U0\n" \
"NSt3zn8gj1KjAIns1aeibVvjS5KToID1AZTc8GgHHs3u/iVStSBDHBv+6xnOQ6Oj\n" \
"ggEgMIIBHDAfBgNVHSMEGDAWgBQ64QmG1M8ZwpZ2dEl23OA1xmNjmjAdBgNVHQ4E\n" \
"FgQU0SLaTFnxS18mOKqd1u7rDcP7qWEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB\n" \
"/wQFMAMBAf8wHQYDVR0lBBYwFAYIKwYBBQUHAwEGCCsGAQUFBwMCMBEGA1UdIAQK\n" \
"MAgwBgYEVR0gADBQBgNVHR8ESTBHMEWgQ6BBhj9odHRwOi8vY3JsLnVzZXJ0cnVz\n" \
"dC5jb20vVVNFUlRydXN0RUNDQ2VydGlmaWNhdGlvbkF1dGhvcml0eS5jcmwwNQYI\n" \
"KwYBBQUHAQEEKTAnMCUGCCsGAQUFBzABhhlodHRwOi8vb2NzcC51c2VydHJ1c3Qu\n" \
"Y29tMAoGCCqGSM49BAMDA2kAMGYCMQCMCyBit99vX2ba6xEkDe+YO7vC0twjbkv9\n" \
"PKpqGGuZ61JZryjFsp+DFpEclCVy4noCMQCwvZDXD/m2Ko1HA5Bkmz7YQOFAiNDD\n" \
"49IWa2wdT7R3DtODaSXH/BiXv8fwB9su4tU=\n" \
"-----END CERTIFICATE-----\n";

GithubClient::GithubClient(const String& repository, const String& token) {
    repository_ = repository;
    if (repository.length() > 0) {
        apiGetLatestRelease = "https://" + String(apiHost) + "/repos/" + repository + "/releases/latest";
        apiGetReleases      = "https://" + String(apiHost) + "/repos/" + repository + "/releases";
    }
    accessToken = token;
    if (accessToken.length() > 0) {
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
    WiFiClientSecure client;
    client.setCACert(githubRootCACert);
    client.setTimeout(15000);

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.begin(client, apiGetLatestRelease);
    http.addHeader("User-Agent", "esp32/1.0");
    http.addHeader("X-GitHub-Api-Version", "2022-11-28");
    http.addHeader("Accept", "application/vnd.github+json");
    if (accessToken.length() > 0) {
        http.addHeader("Authorization", "Bearer " + accessToken);
    }

    LOG_SECURE("GITHUB", "Requesting latest release from %s", apiGetLatestRelease.c_str());
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        snprintf(lastErrorMsg, sizeof(lastErrorMsg), "HTTP request failed, error: %d", httpCode);
        if (httpCode > 0) {
            String payload = http.getString();
            LOG_SECURE("GITHUB", "HTTP Response code: %d, payload: %s", httpCode, payload.c_str());
        }
        http.end();
        return false;
    }

    // Filter document to reduce memory footprint
    JsonDocument filter;
    filter["tag_name"] = true;
    filter["name"] = true;
    filter["assets"][0]["name"] = true;
    filter["assets"][0]["url"] = true;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));

    if (error) {
        snprintf(lastErrorMsg, sizeof(lastErrorMsg), "JSON parse failed: %s", error.c_str());
        LOG_SECURE("GITHUB", "JSON Parse Error: %s", error.c_str());
        http.end();
        return false;
    }

    releaseId = doc["tag_name"] | "";
    releaseDescription = doc["name"] | "";

    JsonArray assetsArray = doc["assets"].as<JsonArray>();
    for (JsonObject asset : assetsArray) {
        if (releaseAssets >= MAX_RELEASE_ASSETS) break;
        releaseAssetName[releaseAssets] = asset["name"] | "";
        releaseAssetURL[releaseAssets] = asset["url"] | "";
        if (releaseAssetName[releaseAssets].length() > 0 && releaseAssetURL[releaseAssets].length() > 0) {
            releaseAssets++;
        }
    }

    http.end();
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
    WiFiClientSecure client;
    client.setCACert(githubRootCACert);
    client.setTimeout(10000);

    HTTPClient http;
    http.begin(client, apiGetReleases);
    http.addHeader("User-Agent", "esp32/1.0");
    http.addHeader("X-GitHub-Api-Version", "2022-11-28");
    http.addHeader("Accept", "application/vnd.github+json");
    if (accessToken.length() > 0) {
        http.addHeader("Authorization", "Bearer " + accessToken);
    }

    LOG_SECURE("GITHUB", "Requesting releases (floor='%s') from %s", channel, apiGetReleases.c_str());
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        snprintf(lastErrorMsg, sizeof(lastErrorMsg), "HTTP request failed, error: %d", httpCode);
        if (httpCode > 0) {
            String payload = http.getString();
            LOG_SECURE("GITHUB", "HTTP Response code: %d, payload: %s", httpCode, payload.c_str());
        }
        http.end();
        return false;
    }

    JsonDocument filter;
    filter[0]["tag_name"] = true;
    filter[0]["name"] = true;
    filter[0]["prerelease"] = true;
    filter[0]["assets"][0]["name"] = true;
    filter[0]["assets"][0]["url"] = true;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
    http.end();

    if (error) {
        snprintf(lastErrorMsg, sizeof(lastErrorMsg), "JSON parse failed: %s", error.c_str());
        LOG_SECURE("GITHUB", "JSON Parse Error: %s", error.c_str());
        return false;
    }

    // --- Single-pass scan: find the highest SemVer release within the floor ---
    // Track best candidate in local state; commit to member fields only at end.
    String bestTag;
    String bestDescription;
    String bestAssetName[MAX_RELEASE_ASSETS];
    String bestAssetURL[MAX_RELEASE_ASSETS];
    int    bestAssetCount = 0;
    bool   bestIsPre      = false;
    FirmwareVersion bestVer = {};  // zero-initialised — effectively -infinity

    JsonArray releases = doc.as<JsonArray>();
    for (JsonObject release : releases) {
        bool isPre          = release["prerelease"] | false;
        const char* tagName = release["tag_name"] | "";
        if (tagName[0] == '\0') continue;

        // Determine whether this release meets the subscriber's floor.
        bool accepted = false;
        if (!isPre) {
            // GitHub non-prerelease == stable: always accepted by non-stable subscribers.
            accepted = true;
        } else {
            // Pre-release: parse the tag and compare its channel against the floor.
            FirmwareVersion candidate = parseVersionTag(tagName);
            accepted = (static_cast<uint8_t>(candidate.channel) >= static_cast<uint8_t>(floor));
        }

        if (!accepted) continue;

        FirmwareVersion candidate = parseVersionTag(tagName);
        if (bestTag.isEmpty() || candidate > bestVer) {
            bestVer         = candidate;
            bestTag         = tagName;
            bestDescription = release["name"] | "";
            bestIsPre       = isPre;
            bestAssetCount  = 0;

            JsonArray assetsArray = release["assets"].as<JsonArray>();
            for (JsonObject asset : assetsArray) {
                if (bestAssetCount >= MAX_RELEASE_ASSETS) break;
                String name = asset["name"] | "";
                String url  = asset["url"]  | "";
                if (name.length() > 0 && url.length() > 0) {
                    bestAssetName[bestAssetCount] = name;
                    bestAssetURL[bestAssetCount]  = url;
                    bestAssetCount++;
                }
            }
            LOG_SECURE("GITHUB", "New best candidate: %s (isPre=%d)", tagName, (int)isPre);
        }
    }

    if (bestTag.isEmpty()) {
        snprintf(lastErrorMsg, sizeof(lastErrorMsg),
                 "No release found at or above channel floor '%s'", channel);
        LOG_SECURE("GITHUB", "%s", lastErrorMsg);
        return false;
    }

    // Commit the winner to member fields
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
    releaseId          = String("v0.0.6-") + String(channel) + String(".1");
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
