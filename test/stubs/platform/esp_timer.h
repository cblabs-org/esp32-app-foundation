#pragma once
// Minimal esp_timer stub for native test builds
#include <stdint.h>

typedef void* esp_timer_handle_t;
typedef struct {
    void (*callback)(void*);
    void* arg;
    const char* name;
} esp_timer_create_args_t;

#ifndef ESP_OK
#define ESP_OK 0
#endif
inline int esp_timer_create(const esp_timer_create_args_t*, esp_timer_handle_t*) { return ESP_OK; }
inline int esp_timer_start_periodic(esp_timer_handle_t, uint64_t) { return ESP_OK; }
inline int esp_timer_stop(esp_timer_handle_t) { return ESP_OK; }
inline int esp_timer_delete(esp_timer_handle_t) { return ESP_OK; }
