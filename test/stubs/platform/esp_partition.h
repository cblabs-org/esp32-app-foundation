#pragma once
#include <cstdint>

#define ESP_PARTITION_TYPE_DATA 0x01
#define ESP_PARTITION_SUBTYPE_DATA_SPIFFS 0x82

typedef struct {
    const char* label;
} esp_partition_t;

inline const esp_partition_t* esp_ota_get_running_partition() {
    static esp_partition_t part = {"native"};
    return &part;
}

inline const esp_partition_t* esp_partition_find_first(uint8_t type, uint8_t subtype, const char* label) {
    static esp_partition_t part = {"config"};
    return &part;
}
