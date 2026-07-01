#pragma once
#include <cstddef>
#include <cstdint>

class UpdateClass {
public:
    bool begin(size_t size = 0, int command = 0) { return true; }
    size_t write(uint8_t* data, size_t len) { return len; }
    bool end(bool evenIfRemaining = false) { return true; }
    bool hasError() { return false; }
    const char* errorString() { return ""; }
};
inline UpdateClass Update;
