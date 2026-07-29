/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/ConfigManager/ConfigRoot.cpp
 * Description: Implementation of central configuration root path accessors.
 */

#include "ConfigRoot.hpp"
#include <string>

static std::string _mountBase = "/littlefs";
#ifdef ESP_PLATFORM
static std::string _etcPath = "/littlefs/etc";
static std::string _modPath = "/littlefs/mod";
#else
static std::string _etcPath = "test/tmp/etc";
static std::string _modPath = "test/tmp/mod";
#endif

void ConfigRoot::setMountBase(const char* base) {
    if (base) {
        _mountBase = base;
#ifdef ESP_PLATFORM
        _etcPath = _mountBase + "/etc";
        _modPath = _mountBase + "/mod";
#else
        _etcPath = "test/tmp" + _mountBase + "/etc";
        _modPath = "test/tmp" + _mountBase + "/mod";
#endif
    }
}

const char* ConfigRoot::getEtcPath() {
    return _etcPath.c_str();
}

const char* ConfigRoot::getModPath() {
    return _modPath.c_str();
}

void ConfigRoot::getEtcFilePath(const char* filename, char* buf, size_t size) {
    if (!buf || size == 0) return;
    const char* etc = getEtcPath();
    if (filename && filename[0] == '/') {
        snprintf(buf, size, "%s%s", etc, filename);
    } else {
        snprintf(buf, size, "%s/%s", etc, filename ? filename : "");
    }
}

String ConfigRoot::getEtcFilePath(const char* filename) {
    char buf[128];
    getEtcFilePath(filename, buf, sizeof(buf));
    return String(buf);
}

void ConfigRoot::getModFilePath(const char* ns, char* buf, size_t size) {
    if (!buf || size == 0) return;
    const char* mod = getModPath();
    if (ns && strstr(ns, ".json")) {
        if (ns[0] == '/') {
            snprintf(buf, size, "%s%s", mod, ns);
        } else {
            snprintf(buf, size, "%s/%s", mod, ns);
        }
    } else {
        snprintf(buf, size, "%s/%s.json", mod, ns ? ns : "");
    }
}

String ConfigRoot::getModFilePath(const char* ns) {
    char buf[128];
    getModFilePath(ns, buf, sizeof(buf));
    return String(buf);
}
