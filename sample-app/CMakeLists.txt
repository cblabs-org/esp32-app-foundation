cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(weather-station)

# --- PR-320: Inject LwIP DHCP Option 100/101 hook header ---
idf_component_get_property(lwip lwip COMPONENT_LIB)
if(lwip)
    get_filename_component(FRAMEWORK_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/../include" ABSOLUTE)
    target_compile_options(${lwip} PRIVATE "-I${FRAMEWORK_INCLUDE_DIR}")
    target_compile_definitions(${lwip} PRIVATE "-DESP_IDF_LWIP_HOOK_FILENAME=\"lwip_dhcp_tz_hooks.h\"")
endif()

