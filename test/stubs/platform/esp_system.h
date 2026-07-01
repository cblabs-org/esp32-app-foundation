#pragma once
#include <cstdint>

inline void esp_restart() {}

class ESPClass {
public:
    const char* getChipModel() { return "ESP32-S3-STUB"; }
    uint32_t getCpuFreqMHz() { return 240; }
    const char* getSdkVersion() { return "v1.0.0-stub"; }
    uint32_t getFlashChipSize() { return 4194304; }
    void restart() {}
};
inline ESPClass ESP;
