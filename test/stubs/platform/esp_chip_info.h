#pragma once
#include <cstdint>

typedef struct {
    uint32_t features;
    uint8_t cores;
    uint8_t revision;
} esp_chip_info_t;

inline void esp_chip_info(esp_chip_info_t* out_info) {
    if (out_info) {
        out_info->features = 0;
        out_info->cores = 2;
        out_info->revision = 1;
    }
}

