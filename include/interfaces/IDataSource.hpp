/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/interfaces/IDataSource.hpp
 * Description: Pure interface for background data-fetch workers scheduled
 *              by the DataManager. Defines the UpdateStatus result enum and
 *              the PriorityTier scheduling tier enum.
 *
 * Exported Enums:
 * - UpdateStatus: Result code returned by fetchUpdate()
 *   - Success, NoChange, RateLimited, FetchError, ParseError, AuthError
 * - PriorityTier: Scheduling priority tier for DataManager
 *   - High (every ~1 min), Normal (every ~5 min), Low (every ~30 min)
 *
 * Exported Classes:
 * - IDataSource: Abstract base for any module that provides periodic data
 *   - fetchUpdate(): Execute one data-fetch cycle; returns UpdateStatus
 *   - getUpdateIntervalMs(): Report the desired polling interval in milliseconds
 *   - getSourceName(): Human-readable identifier used in logs and EventBus topics
 *   - getPriorityTier(): Return the scheduling priority tier
 */

#pragma once
#include <stdint.h>

/** @brief Result codes for a single IDataSource::fetchUpdate() execution. */
enum class UpdateStatus : uint8_t {
    Success      = 0, ///< Data fetched and parsed successfully
    NoChange     = 1, ///< Server returned 304 or data is unchanged
    RateLimited  = 2, ///< Server returned 429 or local back-off triggered
    FetchError   = 3, ///< Network or TLS connection failure
    ParseError   = 4, ///< Response received but JSON/payload parsing failed
    AuthError    = 5  ///< API key rejected or credentials missing
};

/** @brief Scheduling priority tier for DataManager worker scheduling. */
enum class PriorityTier : uint8_t {
    High   = 0, ///< Refresh approximately every 60 seconds
    Normal = 1, ///< Refresh approximately every 300 seconds (5 mins)
    Low    = 2  ///< Refresh approximately every 1800 seconds (30 mins)
};

/**
 * @brief Contract for background data-fetch workers managed by DataManager.
 *
 * Implementors provide a fetch callback and scheduling metadata. DataManager
 * invokes fetchUpdate() at the interval requested by getUpdateIntervalMs()
 * from within a dedicated FreeRTOS task.
 */
class IDataSource {
public:
    virtual ~IDataSource() = default;

    /**
     * @brief Execute one full data-fetch cycle.
     * @return UpdateStatus result of the attempt.
     */
    virtual UpdateStatus fetchUpdate() = 0;

    /**
     * @brief Return the desired polling interval in milliseconds.
     * @return Milliseconds between successive fetchUpdate() calls.
     */
    virtual uint32_t getUpdateIntervalMs() const = 0;

    /**
     * @brief Return a human-readable name for this data source.
     * @return Null-terminated string identifier used in logs and EventBus topics.
     */
    virtual const char* getSourceName() const = 0;

    /**
     * @brief Return the scheduling priority tier.
     * @return PriorityTier value used by DataManager to group sources.
     */
    virtual PriorityTier getPriorityTier() const = 0;
};
