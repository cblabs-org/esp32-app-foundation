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
 * @brief Health state of a registered data source.
 *
 * A source that cannot be reached degrades *itself* — it never resets the
 * device. Every state below has a defined exit; see SourceHealth.
 *
 * @note `Unknown` is deliberately distinct from `Quarantined`. A source that has
 *       never succeeded and a source that has failed after succeeding are
 *       different problems for the user: the first usually means a
 *       misconfiguration they can correct, the second an outage they cannot.
 *       Consumers of this state — gates, boot reporters, displays — MUST
 *       distinguish them. Reporting "still trying" for a source that has never
 *       once connected is indistinguishable from silence to the person standing
 *       in front of the device, and is the failure mode that produced #128.
 */
enum class SourceHealthState : uint8_t {
    Unknown     = 0, ///< Registered but never attempted. Exit: first attempt.
    Healthy     = 1, ///< Last attempt succeeded. Exit: a failure → Degraded.
    Degraded    = 2, ///< Failing and backing off; still retrying. Exit: success → Healthy, or ceiling → Quarantined.
    Quarantined = 3, ///< At the backoff ceiling. Still probes at the ceiling interval, so it self-heals. Exit: a successful probe → Healthy.
    Suspended   = 4  ///< Hard failure (AuthError). No automatic retry — credentials will not fix themselves. Exit: resetSourceHealth() or an explicit priority fetch.
};

/**
 * @brief Point-in-time health snapshot for one registered source.
 *
 * Returned by value; safe to read from any task.
 */
struct SourceHealth {
    /// Borrowed from getSourceName(), which the interface requires to have static
    /// lifetime. The snapshot does NOT own it — a snapshot that outlives its
    /// source's registration is holding a pointer the framework no longer
    /// guarantees. Copy the string before handing a snapshot to anything that
    /// serialises asynchronously.
    const char*       name                = nullptr;
    SourceHealthState state               = SourceHealthState::Unknown;
    /// Result of the last completed attempt. Carries no verdict while
    /// state == Unknown (never attempted, or just reset) — it reads as Success
    /// only because that is the zero value, so branch on `state` first.
    UpdateStatus      lastStatus          = UpdateStatus::Success;
    uint16_t          consecutiveFailures = 0;
    uint32_t          lastAttemptMs       = 0;        ///< 0 = never attempted
    uint32_t          lastSuccessMs       = 0;        ///< Only meaningful when everSucceeded is true
    uint32_t          nextAttemptMs       = 0;        ///< Deadline; meaningful only while backoffActive
    uint32_t          currentBackoffMs    = 0;        ///< Active backoff interval; 0 = not backing off
    bool              everSucceeded       = false;    ///< Distinguishes never-connected from lost-connection
    bool              backoffActive       = false;    ///< Whether nextAttemptMs holds a live deadline

    /** @brief True when the source is not currently serving usable data. */
    bool isFailing() const {
        return state == SourceHealthState::Degraded
            || state == SourceHealthState::Quarantined
            || state == SourceHealthState::Suspended;
    }

    /**
     * @brief True when this source has never once succeeded.
     *
     * The case a user can usually act on — a typo'd address, a missing key, a
     * firewall rule. Report it distinctly from a source that has merely gone away.
     */
    bool neverConnected() const { return !everSucceeded; }
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
     *
     * @note Called from the DataManager worker task. This is a shared queue —
     *       a long-running fetch blocks all other registered data sources and
     *       queued jobs until it returns.
     *
     *       IMPORTANT: Implementations MUST set client-level timeouts (e.g.
     *       HTTPClient::setTimeout()) to bound network operations. The framework
     *       enforces a 30-second hard deadline — fetches exceeding 30 s trigger
     *       an abort/reboot. Fetches exceeding 15 s log a warning.
     *
     * @return UpdateStatus indicating the result of the fetch operation.
     */
    virtual UpdateStatus fetchUpdate() = 0;

    /**
     * @brief Return the desired polling interval in milliseconds.
     * @return Milliseconds between successive fetchUpdate() calls.
     */
    virtual uint32_t getUpdateIntervalMs() const = 0;

    /**
     * @brief Return a human-readable name for this data source.
     *
     * @warning The returned pointer MUST have static or application lifetime.
     *          The framework stores it by reference in health snapshots and log
     *          output; a name backed by a member that dies with the source will
     *          dangle. Return a string literal or a static buffer.
     *
     * @return Null-terminated string identifier used in logs and EventBus topics.
     */
    virtual const char* getSourceName() const = 0;

    /**
     * @brief Return the scheduling priority tier.
     * @return PriorityTier value used by DataManager to group sources.
     */
    virtual PriorityTier getPriorityTier() const = 0;

    /**
     * @brief May this source be fetched while the application is CONFIGURING?
     *
     * Scheduled fetching is normally gated on AppState::Running, so a source is
     * only polled once every IConfigGate is satisfied. That is right for ordinary
     * data sources: there is no point fetching weather before WiFi exists.
     *
     * It is exactly wrong for a source that *backs* a gate. Such a source forms a
     * circular dependency with the state machine — the gate blocks RUNNING,
     * RUNNING enables fetching, and fetching is what would satisfy the gate — so
     * it can never be satisfied, on first boot or after a regression. Return true
     * to opt out of the gate and keep this source scheduled in CONFIGURING.
     *
     * @warning A source that returns true is polled before the application is
     *          fully configured. It MUST tolerate absent configuration and
     *          missing dependencies, and must fail fast rather than block.
     *
     * @return false by default (fetch only while RUNNING).
     */
    virtual bool fetchesWhileConfiguring() const { return false; }
};
