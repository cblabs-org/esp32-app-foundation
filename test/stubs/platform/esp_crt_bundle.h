#pragma once
#include "esp_system.h"

#ifndef ESP_OK
#define ESP_OK 0
#endif

inline int esp_crt_bundle_attach(void* conf) { return ESP_OK; }
