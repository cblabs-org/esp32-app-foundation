/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/render/shim/HostEnvironment.cpp
 * Description: Storage for the process-wide HostEnvironment, plus the one
 *              platform function (millis) that is not header-only.
 *
 * Exported Functions:
 * - hostEnvironment(): Returns the process-wide HostEnvironment.
 * - millis():          Arduino millis(), answered from the HostEnvironment.
 */

#include "HostEnvironment.hpp"
#include "Arduino.h"

/** @brief Return the one HostEnvironment instance (function-local static, no init-order hazard). */
HostEnvironment& hostEnvironment() {
    static HostEnvironment environment;
    return environment;
}

/** @brief Arduino millis(): the harness-controlled clock, not real time. */
unsigned long millis() {
    return hostEnvironment().millis;
}
