#pragma once
#include "Arduino.h"

inline void esp_wifi_restore() {}
inline esp_err_t esp_wifi_set_rssi_threshold(int32_t) { return ESP_OK; }
