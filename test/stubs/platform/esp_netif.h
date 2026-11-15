#pragma once

#include "esp_sntp.h"

#ifndef ESP_OK
#define ESP_OK 0
#endif

typedef int esp_err_t;

inline esp_err_t esp_netif_init(void) {
    auto& st = stub_sntp_state();
    st.netifInitialized = true;
    ++st.netifInitCount;
    return ESP_OK;
}
