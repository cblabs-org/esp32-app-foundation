cmake_minimum_required(VERSION 3.16.0)

# --- PR-10: Exclude unused Arduino Core managed dependencies ---
# The Arduino Core's idf_component.yml declares ~27 managed dependencies.
# Most are unused by this framework. EXCLUDE_COMPONENTS tells the ESP-IDF
# build system to skip their compilation entirely, saving significant flash.
# IMPORTANT: This list MUST appear BEFORE include(project.cmake).

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
project(esp32-app-foundation)

# --- PR-320: Inject LwIP DHCP Option 100/101 hook header ---
idf_component_get_property(lwip lwip COMPONENT_LIB)
if(lwip)
    target_compile_options(${lwip} PRIVATE "-I${CMAKE_CURRENT_LIST_DIR}/include")
    target_compile_definitions(${lwip} PRIVATE "-DESP_IDF_LWIP_HOOK_FILENAME=\"lwip_dhcp_tz_hooks.h\"")
endif()

