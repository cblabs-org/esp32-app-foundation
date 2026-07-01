#pragma once
#include "esp_system.h" // for esp_err_t

#ifndef ESP_OK
#define ESP_OK 0
#endif

inline int esp_ota_mark_app_valid_cancel_rollback() { return ESP_OK; }
