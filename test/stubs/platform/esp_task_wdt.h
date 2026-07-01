#pragma once
#include "esp_system.h" // for esp_err_t if needed

#ifndef ESP_OK
#define ESP_OK 0
#endif

typedef int esp_err_t;

inline esp_err_t esp_task_wdt_add(void* handle) { return ESP_OK; }
inline esp_err_t esp_task_wdt_reset() { return ESP_OK; }
inline esp_err_t esp_task_wdt_delete(void* handle) { return ESP_OK; }
