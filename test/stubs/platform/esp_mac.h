#pragma once
#include <cstdint>

#define ESP_MAC_WIFI_STA 0

inline int esp_read_mac(uint8_t* mac, int type) {
    mac[0] = 0xAA; mac[1] = 0xBB; mac[2] = 0xCC;
    mac[3] = 0xDD; mac[4] = 0xEE; mac[5] = 0xFF;
    return 0; // ESP_OK
}
