/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/interfaces/iDataSource.hpp
 * Description: Pure interface for background data-fetch workers scheduled
 *              by the DataManager. Defines the UpdateStatus result enum and
 *              the PriorityTier scheduling tier enum.
 *
 * Exported Enums:
 * - UpdateStatus: Result code returned by fetchUpdate()
 *   - SUCCESS, NO_CHANGE, RATE_LIMITED, FETCH_ERROR, PARSE_ERROR, AUTH_ERROR
 * - PriorityTier: Scheduling priority tier for DataManager
 *   - HIGH (every ~1 min), MEDIUM (every ~5 min), LOW (every ~30 min)
 *
 * Exported Classes:
 * - iDataSource: Abstract base for any module that provides periodic data
 *   - fetchUpdate(): Execute one data-fetch cycle; returns UpdateStatus
 *   - getUpdateIntervalMs(): Report the desired polling interval in milliseconds
 *   - getSourceName(): Human-readable identifier used in logs and EventBus topics
 *   - getPriorityTier(): Return the scheduling priority tier
 */

#pragma once
#include <stdint.h>

/** @brief Result codes for a single iDataSource::fetchUpdate() execution. */
enum class UpdateStatus : uint8_t {
    SUCCESS      = 0, ///< Data fetched and parsed successfully
    NO_CHANGE    = 1, ///< Server returned 304 or data is unchanged
    RATE_LIMITED = 2, ///< Server returned 429 or local back-off triggered
    FETCH_ERROR  = 3, ///< Network or TLS connection failure
    PARSE_ERROR  = 4, ///< Response received but JSON/payload parsing failed
    AUTH_ERROR   = 5  ///< API key rejected or credentials missing
};

/** @brief Scheduling priority tier for DataManager worker scheduling. */
enum class PriorityTier : uint8_t {
    HIGH   = 0, ///< Refresh approximately every 60 seconds
    MEDIUM = 1, ///< Refresh approximately every 5 minutes
    LOW    = 2  ///< Refresh approximately every 30 minutes
};

/**
 * @brief Contract for background data-fetch workers managed by DataManager.
 *
 * Implementors provide a fetch callback and scheduling metadata. DataManager
 * invokes fetchUpdate() at the interval requested by getUpdateIntervalMs()
 * from within a dedicated FreeRTOS task.
 */
class iDataSource {
public:
    virtual ~iDataSource() = default;

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
