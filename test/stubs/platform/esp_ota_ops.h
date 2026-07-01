#pragma once
#include "esp_partition.h"

#include "esp_system.h" // for esp_err_t

#ifndef ESP_OK
#define ESP_OK 0
#endif

inline int esp_ota_mark_app_valid_cancel_rollback() { return ESP_OK; }

inline const esp_partition_t* esp_ota_get_next_update_partition(const esp_partition_t *start_from) {
    static esp_partition_t p;
    return &p;
}

inline int esp_ota_set_boot_partition(const esp_partition_t* partition) { return ESP_OK; }
