/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/render/shim/Arduino.h
 * Description: The slice of the Arduino core the legacy board and U8G2's C++
 *              header need on a host: fixed-width types and millis(). U8g2lib.h
 *              includes <Arduino.h> and <Print.h> when ARDUINO is defined, which
 *              is how the firmware sees it. This file declares platform symbols
 *              only and never a U8G2 API, so U8G2's own declarations are the
 *              ones compiled against.
 *
 * Exported Functions:
 * - millis(): Milliseconds since boot (harness-controlled, see HostEnvironment).
 * - yield():  Scheduler yield; U8g2lib.cpp calls it in the screenshot helper. No-op.
 * - harnessBuildVersion(): The firmware version string of the scenario being
 *                          rendered. The build defines BUILD_VERSION as a call to it.
 */

#pragma once

#include <cstddef>
#include <cstdint>

/** @brief Milliseconds since boot; on the host, whatever the scenario set. */
unsigned long millis();

/**
 * @brief The firmware version of the scenario being rendered.
 *
 * Not an Arduino symbol. In the firmware BUILD_VERSION is a string literal
 * injected by inject_version.py; the harness defines BUILD_VERSION as a call to
 * this function instead (CMakeLists.txt), so each scenario can carry its own
 * version while the board sources stay unmodified. Answered from
 * HostEnvironment::buildVersion.
 */
const char* harnessBuildVersion();

/** @brief Arduino scheduler yield; there is nothing to yield to on a host. */
inline void yield() {}
