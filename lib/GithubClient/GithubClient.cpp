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
 * @brief Query releases and find the latest matching the specified channel.
 *
 * For "stable" (or empty/null), delegates to getLatestRelease().
 * For "beta" or "dev", queries the /releases endpoint and scans for the
 * first pre-release whose tag name contains the channel string.
 *
 * @param channel Channel to filter by ("stable", "beta", "dev")
 * @return true if a matching release was found
 */
bool GithubClient::getLatestReleaseForChannel(const char* channel) {
    // If channel is null, empty, or "stable", delegate to the existing method
    if (channel == nullptr || channel[0] == '\0' || strcmp(channel, "stable") == 0) {
        return getLatestRelease();
    }

    releaseId = "";
    releaseDescription = "";
    releaseAssets = 0;
    isPreRelease = false;

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

    LOG_SECURE("GITHUB", "Requesting releases for channel '%s' from %s", channel, apiGetReleases.c_str());
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
    filter[0]["tag_name"] = true;
    filter[0]["name"] = true;
    filter[0]["prerelease"] = true;
    filter[0]["assets"][0]["name"] = true;
    filter[0]["assets"][0]["url"] = true;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));

    if (error) {
        snprintf(lastErrorMsg, sizeof(lastErrorMsg), "JSON parse failed: %s", error.c_str());
        LOG_SECURE("GITHUB", "JSON Parse Error: %s", error.c_str());
        http.end();
        return false;
    }

    // Scan releases for the first matching pre-release
    JsonArray releases = doc.as<JsonArray>();
    for (JsonObject release : releases) {
        bool isPre = release["prerelease"] | false;
        const char* tagName = release["tag_name"] | "";

        if (isPre && strstr(tagName, channel) != nullptr) {
            releaseId = tagName;
            releaseDescription = release["name"] | "";
            isPreRelease = true;

            JsonArray assetsArray = release["assets"].as<JsonArray>();
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
            LOG_SECURE("GITHUB", "Found matching pre-release: %s", tagName);
            return true;
        }
    }

    http.end();
    snprintf(lastErrorMsg, sizeof(lastErrorMsg), "No matching pre-release for channel '%s'", channel);
    LOG_SECURE("GITHUB", "No matching pre-release for channel '%s'", channel);
    return false;
#else
    // Mock implementation for native testing — simulate a channel match
    releaseId = String("v1.0.0-") + String(channel) + String(".1");
    releaseDescription = "Mock Pre-Release";
    releaseAssets = 1;
    releaseAssetName[0] = "firmware.bin";
    releaseAssetURL[0] = "https://example.com/firmware.bin";
    isPreRelease = true;
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
