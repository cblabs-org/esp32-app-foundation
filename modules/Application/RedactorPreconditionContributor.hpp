/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: lib/Redactor/RedactorPreconditionContributor.hpp
 * Description: Built-in IPreconditionContributor implementation that registers
 *              the Redactor Health precondition.
 *
 * Exported Classes:
 * - RedactorPreconditionContributor: Contributes Redactor health precondition
 */

#pragma once

#include "interfaces/IPreconditionContributor.hpp"
#include "interfaces/IPreconditionRegistry.hpp"
#include "Redactor.h"
#include <cstdio>

/**
 * @brief Contributes the Redactor health precondition to IPreconditionRegistry.
 *
 * Evaluates:
 * - isFull() or saturationCount() > 0 => Degraded (FallbackEngaged)
 * - Otherwise => Met
 */
class RedactorPreconditionContributor : public IPreconditionContributor {
public:
    /** @brief Registers the Redactor health precondition with the engine. */
    void registerPreconditions(IPreconditionRegistry& reg) override {
        reg.add(PreconditionNames::REDACTOR_HEALTH, PreconditionRole::Operational,
            [](void*) -> PreconditionStatus {
                if (Redactor::instance().isFull() || Redactor::instance().saturationCount() > 0) {
                    return {false, Cause::FallbackEngaged};
                }
                return {true, Cause::Met};
            },
            [](Cause c, char* buf, size_t len, void*) {
                if (c == Cause::FallbackEngaged) {
                    snprintf(buf, len, "Redactor registry saturated (%zu/%zu secrets, %u rejected)",
                             Redactor::instance().secretCount(), Redactor::MAX_SECRETS,
                             Redactor::instance().saturationCount());
                } else {
                    snprintf(buf, len, "Redactor healthy (%zu/%zu secrets)",
                             Redactor::instance().secretCount(), Redactor::MAX_SECRETS);
                }
            });
    }
};
