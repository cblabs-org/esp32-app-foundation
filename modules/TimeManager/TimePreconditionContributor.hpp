/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/TimeManager/TimePreconditionContributor.hpp
 * Description: Built-in IPreconditionContributor implementation that registers
 *              Timezone and Time Sync preconditions (Operational role).
 *
 * Exported Classes:
 * - TimePreconditionContributor: Contributes Timezone and Time Sync preconditions
 *   - registerPreconditions(): Registers Timezone and Time Sync preconditions
 */

#pragma once

#include "interfaces/IPreconditionContributor.hpp"
#include "utils/PreconditionRegistry.hpp"
#include "TimeManager.hpp"
#include <cstdio>

/**
 * @brief Contributes Timezone and Time Sync operational preconditions.
 *
 * Splits the old TimeConfigGate into two Operational preconditions:
 * 1. Timezone: Operational, no edges (settable in AP mode).
 * 2. Time Sync: Operational, Requires WiFi Station (§9.1).
 *
 * Both preconditions are registered as Operational by default. Consumer apps
 * that require time sync before entering RUNNING can override the role to
 * Startup by registering their own TimePreconditionContributor subclass, or
 * by adding a Startup-role precondition that wraps these Operational ones
 * via BindsTo edges. The framework does not enforce Startup here because
 * many applications function correctly without NTP on first boot.
 */
class TimePreconditionContributor : public IPreconditionContributor {
public:
    /**
     * @brief Construct with a reference to the TimeManager to query.
     * @param tm Reference to the TimeManager instance.
     */
    explicit TimePreconditionContributor(const TimeManager& tm) : _timeManager(tm) {}

    /**
     * @brief Register Timezone and Time Sync preconditions with the registry.
     * @param reg Target PreconditionRegistry instance.
     */
    void registerPreconditions(PreconditionRegistry& reg) override {
        // 1. Timezone — Operational, no edges (settable in AP mode)
        reg.add(PreconditionNames::TIMEZONE, PreconditionRole::Operational,
            [this]() -> PreconditionStatus {
                return _timeManager.isConfigured()
                    ? PreconditionStatus{true, Cause::Met}
                    : PreconditionStatus{false, Cause::NotConfigured};
            },
            [](Cause, char* buf, size_t len) {
                snprintf(buf, len, "Set your timezone in the Time settings");
            });

        // 2. Time Sync — Operational, Requires WiFi Station (§9.1)
        static const Edge syncEdges[] = {
            {PreconditionNames::WIFI_STATION, EdgeKind::Requires}
        };
        reg.add(PreconditionNames::TIME_SYNC, PreconditionRole::Operational,
            [this]() -> PreconditionStatus {
                auto status = _timeManager.getSyncStatus();
                if (status == TimeSyncStatus::SYNCED)
                    return {true, Cause::Met};
                if (status == TimeSyncStatus::IN_PROGRESS)
                    return {false, Cause::Connecting};
                return {false, Cause::Unreachable};
            },
            nullptr,  // single-cause — no describe needed (CG-16)
            syncEdges, 1);
    }

private:
    const TimeManager& _timeManager; ///< Reference to TimeManager
};
