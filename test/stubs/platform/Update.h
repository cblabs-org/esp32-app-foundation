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

    void abort() {}
    bool setMD5(const char * expected_md5) { return true; }
    size_t writeStream(class Stream &data) { return 0; }
};
#define UPDATE_SIZE_UNKNOWN 0xFFFFFFFF

inline UpdateClass Update;
