/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/render/shim/WiFi.h
 * Description: Host stand-in for the ESP32 Arduino WiFi singleton, reduced to
 *              the two calls the legacy DiagnosticsBoard makes:
 *              WiFi.isConnected() and WiFi.localIP().toString().c_str().
 *              Both answer from the HostEnvironment.
 *
 * Exported Classes:
 * - String:    Minimal Arduino String (c_str() only).
 * - IPAddress: Minimal IPAddress (toString() only).
 * - WiFiClass: The WiFi singleton type; `WiFi` is its one instance.
 */

#pragma once

#include <string>
#include <utility>

#include "HostEnvironment.hpp"

/** @brief Minimal Arduino String: owns text, exposes c_str(). */
class String {
public:
    /** @brief Wrap a copy of the given text. */
    explicit String(std::string text) : _text(std::move(text)) {}

    /** @brief The text as a C string, valid while this String lives. */
    const char* c_str() const { return _text.c_str(); }

private:
    std::string _text;  ///< The owned text
};

/** @brief Minimal IPAddress: carries the dotted-quad text the harness chose. */
class IPAddress {
public:
    /** @brief Wrap the dotted-quad text. */
    explicit IPAddress(std::string text) : _text(std::move(text)) {}

    /** @brief The address as dotted-quad text. */
    String toString() const { return String(_text); }

private:
    std::string _text;  ///< Dotted-quad text, e.g. "192.168.1.42"
};

/** @brief The WiFi singleton type, answering from the HostEnvironment. */
class WiFiClass {
public:
    /** @brief True when the scenario says the station is connected. */
    bool isConnected() const { return hostEnvironment().wifiConnected; }

    /** @brief The scenario's station IP address. */
    IPAddress localIP() const { return IPAddress(hostEnvironment().localIP); }
};

/** @brief The one WiFi instance, as on the ESP32 Arduino core. */
inline WiFiClass WiFi;
