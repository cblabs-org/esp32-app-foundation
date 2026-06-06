/*
 * ESP32 App Foundation (c) 2026
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: lib/GithubClient/GithubClient.hpp
 * Description: Client to query the GitHub REST API for release information 
 *              using HTTPS and certificate validation.
 *
 * Exported Functions/Classes:
 * - GithubClient: [Class] API client using secure HTTPS and JSON parsing.
 *   - getLatestRelease(): Polls GitHub for release metadata.
 *   - getLatestReleaseForChannel(): Channel-aware release query.
 *   - getLastError(): Diagnostic error retrieval.
 */

#ifndef GITHUB_CLIENT_HPP
#define GITHUB_CLIENT_HPP

#ifndef NATIVE_BUILD
#include <Arduino.h>
#else
#include <string>
#include <cstdint>
#include <cstddef>
#ifndef HEX
#define HEX 16
#endif
// Provide String stub if not already available
#ifndef STRING_STUB_DEFINED
#define STRING_STUB_DEFINED
class String {
public:
    String() {}
    String(const char* s) : _str(s ? s : "") {}
    String(const std::string& s) : _str(s) {}
    String(int value, int base) {
        if (base == 16) {
            char buf[20];
            snprintf(buf, sizeof(buf), "%x", value);
            _str = buf;
        } else {
            _str = std::to_string(value);
        }
    }
    const char* c_str() const { return _str.c_str(); }
    bool isEmpty() const { return _str.empty(); }
    int length() const { return _str.length(); }
    String operator+(const char* s) const { return String(_str + s); }
    String operator+(const String& s) const { return String(_str + s._str); }
    bool operator==(const String& other) const { return _str == other._str; }
    bool operator==(const char* s) const { return _str == s; }
    char operator[](int i) const { return _str[i]; }
    String& operator+=(char c) { _str += c; return *this; }
    String& operator+=(const String& s) { _str += s._str; return *this; }
    friend String operator+(const char* lhs, const String& rhs) {
        return String(lhs) + rhs;
    }
    operator std::string() const { return _str; }
private:
    std::string _str;
};
#endif
#endif

// ---------------------------------------------------------------------------
// Release asset capacity
//
// MAX_OTA_BOARDS is injected at build time by scripts/inject_version.py,
// which reads [platformio] default_envs from platformio.ini.
// That makes platformio.ini the single source of truth: adding a board to
// default_envs automatically propagates to MAX_RELEASE_ASSETS here.
//
//   ASSETS_PER_BOARD = .bin + .bin.sig + .bin.sha256  (3 files per board)
//   SHARED_ASSETS    = manifest.json (+ future shared files)
//
// Native builds do not run inject_version.py, so a safe fallback is provided.
// ---------------------------------------------------------------------------
#ifndef MAX_OTA_BOARDS
#  define MAX_OTA_BOARDS 8   ///< Native-build fallback; hardware builds set this via -D
#endif
static constexpr int ASSETS_PER_BOARD   = 3;
static constexpr int SHARED_ASSETS      = 2;
static constexpr int MAX_RELEASE_ASSETS = (MAX_OTA_BOARDS * ASSETS_PER_BOARD) + SHARED_ASSETS;

/**
 * @brief API client to interact with GitHub Releases securely.
 */
class GithubClient {
private:
    const char* apiHost = "api.github.com";
    String apiGetLatestRelease;
    String apiGetReleases;
    String accessToken;
    String repository_;
    char lastErrorMsg[128];

    // Global Root CA for GitHub API (ISRG Root X1 or DigiCert)
    static const char* githubRootCACert;

public:
    String releaseId;
    String releaseDescription;
    int releaseAssets;
    String releaseAssetURL[MAX_RELEASE_ASSETS];
    String releaseAssetName[MAX_RELEASE_ASSETS];
    bool isPreRelease;  ///< Set by the release query methods

    /**
     * @brief Constructor for the GitHub client.
     * @param repository The repository in "owner/repo" format.
     * @param token Optional GitHub personal access token.
     */
    GithubClient(const String& repository, const String& token = "");

    ~GithubClient() = default;

    /**
     * @brief Polls the GitHub API to fetch metadata about the latest release for the repository.
     *        Performs HTTPS certificate validation.
     * @return True if the metadata was successfully fetched and parsed, otherwise false.
     */
    bool getLatestRelease();

    /**
     * @brief Query releases and return the best available version for the
     *        subscriber's channel using the risk-floor model.
     *
     * Channel determines MINIMUM stability, not an exact track:
     *   stable → stable only (delegates to getLatestRelease())
     *   rc     → rc + stable
     *   beta   → beta + rc + stable
     *   dev    → dev + beta + rc + stable
     *
     * A single /releases request is made; the highest SemVer version among
     * all releases that meet the floor is returned. This means a beta
     * subscriber automatically receives v0.0.7 stable when it is ahead of
     * the latest v0.0.6-beta.x with no manual channel change required.
     *
     * @param channel Channel floor string: "stable", "rc", "beta", or "dev".
     * @return true if any acceptable release was found.
     */
    bool getLatestReleaseForChannel(const char* channel);

    /**
     * @brief Retrieves the last error message generated by the client.
     * @return A string containing the error description (or "SUCCESS").
     */
    const char* getLastError();
};

#endif // GITHUB_CLIENT_HPP
