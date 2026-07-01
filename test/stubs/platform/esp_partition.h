#pragma once
#include <cstdint>

typedef struct {
    const char* label;
} esp_partition_t;

inline const esp_partition_t* esp_ota_get_running_partition() {
    static esp_partition_t part = {"native"};
    return &part;
}
