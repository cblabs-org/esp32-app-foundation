/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/BLERoutingTable.hpp
 * Description: Pure C++ lookup table mapping GATT characteristic handles to their
 *              owning IBLEServiceProvider instances for subscription event dispatch.
 *              Authoritative: RFC #225 rev 2 §6.2, §16.3.
 */

#pragma once

#include <cstdint>
#include <map>
#include "IBLEServiceProvider.hpp"

/**
 * @brief Thread-safe (when mutated only on main thread) lookup table mapping
 *        GATT characteristic attribute handles to owning service providers.
 */
class BLERoutingTable {
public:
    BLERoutingTable() = default;
    ~BLERoutingTable() = default;

    /**
     * @brief Associate a characteristic attribute handle with its owning provider.
     * @param charHandle 16-bit characteristic value attribute handle (> 0).
     * @param provider Pointer to the provider instance.
     * @return true if added/updated, false if handle is 0 or provider is null.
     */
    bool addRoute(uint16_t charHandle, IBLEServiceProvider* provider) {
        if (charHandle == 0 || provider == nullptr) {
            return false;
        }
        _routes[charHandle] = provider;
        return true;
    }

    /**
     * @brief Find the provider that owns the specified characteristic handle.
     * @param charHandle 16-bit characteristic attribute handle.
     * @return Pointer to owning provider, or nullptr if not found.
     */
    IBLEServiceProvider* findProvider(uint16_t charHandle) const {
        auto it = _routes.find(charHandle);
        return (it != _routes.end()) ? it->second : nullptr;
    }

    /**
     * @brief Check if a route exists for the given characteristic handle.
     * @param charHandle 16-bit characteristic attribute handle.
     * @return true if route exists, false otherwise.
     */
    bool hasRoute(uint16_t charHandle) const {
        return _routes.find(charHandle) != _routes.end();
    }

    /**
     * @brief Clear all registered handle routes.
     */
    void clear() {
        _routes.clear();
    }

    /**
     * @brief Current count of registered characteristic handle routes.
     * @return Number of active routes.
     */
    size_t size() const {
        return _routes.size();
    }

private:
    std::map<uint16_t, IBLEServiceProvider*> _routes;
};
