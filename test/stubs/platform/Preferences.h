#pragma once
#include <map>
#include <string>
#include <vector>
#include <cstdint>

class Preferences {
public:
    uint32_t getUInt(const char* key, uint32_t defaultValue = 0) { return defaultValue; }
    size_t putUInt(const char* key, uint32_t value) { return 4; }

    bool begin(const char* name, bool readOnly = false) { return true; }
    void end() {}
    size_t putBytes(const char* key, const void* value, size_t len) {
        std::vector<uint8_t> data((const uint8_t*)value, (const uint8_t*)value + len);
        _map[key] = data;
        return len;
    }
    size_t getBytes(const char* key, void* buf, size_t maxLen) {
        if (_map.find(key) != _map.end()) {
            size_t copyLen = maxLen < _map[key].size() ? maxLen : _map[key].size();
            memcpy(buf, _map[key].data(), copyLen);
            return copyLen;
        }
        return 0;
    }
    size_t getBytesLength(const char* key) {
        if (_map.find(key) != _map.end()) {
            return _map[key].size();
        }
        return 0;
    }
    size_t putString(const char* key, const char* value) {
        return putBytes(key, value, strlen(value) + 1);
    }
    size_t getString(const char* key, char* value, size_t maxLen) {
        return getBytes(key, value, maxLen);
    }
    bool isKey(const char* key) {
        return _map.find(key) != _map.end();
    }


private:
    std::map<std::string, std::vector<uint8_t>> _map;
};

