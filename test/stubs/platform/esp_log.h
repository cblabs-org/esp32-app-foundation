#pragma once

#include <cstdio>
#include <cstdarg>

// Log level constants (mirror esp_log.h enum values)
#define ESP_LOG_NONE    0
#define ESP_LOG_ERROR   1
#define ESP_LOG_WARN    2
#define ESP_LOG_INFO    3
#define ESP_LOG_DEBUG   4
#define ESP_LOG_VERBOSE 5

#define ESP_LOGE(tag, fmt, ...) printf("E (%s): " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) printf("W (%s): " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...) printf("I (%s): " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...) printf("D (%s): " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGV(tag, fmt, ...) printf("V (%s): " fmt "\n", tag, ##__VA_ARGS__)

// No-op for native builds — runtime level control is hardware-only
static inline void esp_log_level_set(const char* tag, int level) { (void)tag; (void)level; }

// Native build stubs for log pipeline compatibility
typedef int (*vprintf_like_t)(const char*, va_list);
static inline vprintf_like_t esp_log_set_vprintf(vprintf_like_t func) {
    (void)func;
    return nullptr;
}

