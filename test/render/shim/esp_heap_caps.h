/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/render/shim/esp_heap_caps.h
 * Description: Host stand-in for ESP-IDF's esp_heap_caps.h, reduced to the one
 *              call the legacy DiagnosticsBoard makes:
 *              heap_caps_get_free_size(MALLOC_CAP_8BIT). Answers from the
 *              HostEnvironment.
 *
 * Exported Functions:
 * - heap_caps_get_free_size(): Free heap in bytes, as the scenario set it.
 */

#pragma once

#include <cstddef>
#include <cstdint>

#include "HostEnvironment.hpp"

#define MALLOC_CAP_8BIT (1u << 2)  // ESP-IDF capability bit for byte-addressable memory

/** @brief Free heap with the requested capabilities, in bytes. */
inline size_t heap_caps_get_free_size(uint32_t caps) {
    (void)caps;  // Only MALLOC_CAP_8BIT is ever asked for; the scenario sets one value.
    return hostEnvironment().freeHeapBytes;
}
