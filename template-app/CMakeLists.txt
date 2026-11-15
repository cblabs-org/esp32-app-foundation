cmake_minimum_required(VERSION 3.16.0)

set(EXCLUDE_COMPONENTS
    # Rainmaker ecosystem (cloud platform — not used)
    espressif__esp_rainmaker
    espressif__rmaker_common
    espressif__esp_insights
    espressif__esp_diagnostics
    espressif__esp_diag_data_store
    espressif__cbor
    espressif__qrcode
    # Industrial protocols (Modbus — not used)
    espressif__esp-modbus
    # Cellular modem (no modem hardware)
    espressif__esp_modem
    # Zigbee (not used)
    espressif__esp-zboss-lib
    espressif__esp-zigbee-lib
    # DSP / Speech / Audio (not used)
    espressif__esp-dsp
    espressif__esp-sr
    chmorgan__esp-libhelix-mp3
    # Crypto (mbedTLS handles all framework crypto needs)
    espressif__libsodium
    # Network provisioning (framework has its own wifiManager)
    espressif__network_provisioning
    # Unused filesystems
    spiffs
    fatfs
    # Console REPL (framework uses MCP over serial)
    console
)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(template-app)

# --- PR-320: Inject LwIP DHCP Option 100/101 hook header ---
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/framework/include/lwip_dhcp_tz_hooks.h")
    idf_component_get_property(lwip_lib lwip COMPONENT_LIB)
    if(lwip_lib)
        target_include_directories(${lwip_lib} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/framework/include")
        target_compile_definitions(${lwip_lib} PRIVATE "ESP_IDF_LWIP_HOOK_FILENAME=\"lwip_dhcp_tz_hooks.h\"")
    endif()
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../include/lwip_dhcp_tz_hooks.h")
    idf_component_get_property(lwip_lib lwip COMPONENT_LIB)
    if(lwip_lib)
        target_include_directories(${lwip_lib} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../include")
        target_compile_definitions(${lwip_lib} PRIVATE "ESP_IDF_LWIP_HOOK_FILENAME=\"lwip_dhcp_tz_hooks.h\"")
    endif()
endif()
