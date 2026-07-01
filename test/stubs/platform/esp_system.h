#pragma once
#include <cstdint>

inline void esp_restart() {}

class ESPClass {
public:
    const char* getChipModel() { return "ESP32-S3-STUB"; }
    uint32_t getCpuFreqMHz() { return 240; }
    void restart() {}
};
inline ESPClass ESP;
