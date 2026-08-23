/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/interfaces/IPrecondition.hpp
 * Description: Types, enums, structs, and declarations for the precondition
 *              framework. Replaces binary config gates with a rich precondition
 *              model supporting cause reporting, severity derivation, edge graphs,
 *              retry policies, circuit breaking, and damping.
 *
 * Exported Enums/Structs/Functions:
 * - Cause: The 12-member cause vocabulary for precondition status
 * - Severity: Derived severity levels representing operational impact
 * - PreconditionRole: Lifecycle role (Operational vs Startup vs Both)
 * - PreconditionStatus: Struct holding met boolean and cause enum
 * - EdgeKind: Dependency relationship strength (Wants, Requires, BindsTo)
 * - Edge: Struct associating a dependency precondition name with EdgeKind
 * - RetryPolicy: Parameters controlling retry delays and fallback escalation
 * - DampingPolicy: Parameters controlling flap penalty accumulation and decay
 * - BreakerState: Circuit breaker states (Closed, Open, HalfOpen)
 * - deriveSeverity(): Free function declaring cause + trust to severity mapping
 * - PreconditionNames: Well-known precondition identifier constants
 */

#pragma once

#include <cstddef>
#include <cstdint>

/**
 * @brief Cause vocabulary explaining why a precondition is met or unmet (CG-3).
 */
enum class Cause : uint8_t {
    Met = 0,          ///< Condition is satisfied.
    NotConfigured,    ///< Nothing to work with. A person must supply something.
    Connecting,       ///< In progress; expected to resolve unaided.
    Rejected,         ///< Config present and refused — bad password, revoked token.
    Unreachable,      ///< Config present and plausible; the peer is not answering.
    RateLimited,      ///< The peer is answering and asking us to slow down.
    Implausible,      ///< An answer arrived, well-formed, and a domain check rejects it.
    DependencyUnmet,  ///< Blocked upstream. Not this precondition's problem.
    FallbackEngaged,  ///< Degraded but serviceable, and still self-healing.
    Unavailable,      ///< Not present in this deployment, by configuration. Not a fault.
    Indeterminate,    ///< Cannot evaluate — a dependency was not wired.
    InternalError     ///< A defect, not a configuration problem.
};

/**
 * @brief Operational severity derived from cause and trust history (CG-4).
 */
enum class Severity : uint8_t {
    Transient = 0,    ///< Keep trying; self-healing in progress.
    NeedsUser,        ///< Requires human intervention to resolve.
    Blocked,          ///< Upstream dependency unmet; upstream owns the report.
    Degraded,         ///< Serviceable in fallback mode, still self-healing.
    Inactive,         ///< Deliberately absent by configuration; not a fault.
    Persistent        ///< Defect or un-wired dependency; requires attention.
};

/**
 * @brief Lifecycle role of a precondition (CG-1).
 */
enum class PreconditionRole : uint8_t {
    Operational = 0,  ///< Features degrade while unmet. Does NOT block boot. (default)
    Startup,          ///< Must be met to leave boot.
    Both              ///< Required at boot and continues to gate operational health.
};

/**
 * @brief Hot-path evaluation status returned by a precondition (CG-2).
 */
struct PreconditionStatus {
    bool  met   = false;             ///< Whether the precondition is satisfied
    Cause cause = Cause::NotConfigured; ///< Specific cause code
};

/**
 * @brief Strength and lifecycle timing of a dependency edge (CG-6).
 */
enum class EdgeKind : uint8_t {
    Wants,      ///< Presentation only: don't offer me while that is unmet.
    Requires,   ///< I cannot be met without it. Evaluated every tick.
    BindsTo     ///< Requires, plus: if it becomes unmet at runtime, so do I.
};

/**
 * @brief Named dependency edge declared by a precondition (CG-6).
 */
struct Edge {
    const char* name;  ///< Identifier of the dependency precondition
    EdgeKind    kind;  ///< Dependency relationship type
};

/**
 * @brief Retry and fallback escalation parameters (CG-9).
 */
struct RetryPolicy {
    uint32_t baseDelayMs            = 5000;    ///< Initial retry delay in milliseconds
    uint32_t ceilingMs              = 300000;  ///< Maximum retry delay in milliseconds (5 min)
    uint8_t  attemptsBeforeFallback = 5;       ///< Retries before escalating to fallback
};

/**
 * @brief Flap damping parameters for unmet status penalty accumulation (CG-12).
 */
struct DampingPolicy {
    uint16_t penaltyPerSecond  = 10;      ///< Penalty accrued per second while unmet
    uint16_t suppressThreshold = 2000;    ///< Score above which precondition is suppressed
    uint16_t reuseThreshold    = 750;     ///< Score below which precondition is unsuppressed
    uint32_t halfLifeMs        = 900000;  ///< Exponential decay half-life in ms (15 min)
};

/**
 * @brief Circuit breaker state for self-healing resources (CG-11).
 */
enum class BreakerState : uint8_t {
    Closed = 0,  ///< Normal operation
    Open,        ///< Retries exhausted; fallback engaged
    HalfOpen     ///< Cooldown elapsed — trial probe in progress
};

inline Severity deriveSeverity(Cause cause, bool everMet = false) {
    switch (cause) {
        case Cause::Met: return Severity::Transient;
        case Cause::Connecting: return Severity::Transient;
        case Cause::Unreachable: return everMet ? Severity::Transient : Severity::NeedsUser;
        case Cause::RateLimited: return Severity::Transient;
        case Cause::Implausible: return everMet ? Severity::Transient : Severity::NeedsUser;
        case Cause::NotConfigured: return Severity::NeedsUser;
        case Cause::Rejected: return Severity::NeedsUser;
        case Cause::DependencyUnmet: return Severity::Blocked;
        case Cause::Indeterminate: return Severity::Persistent;
        case Cause::FallbackEngaged: return Severity::Degraded;
        case Cause::Unavailable: return Severity::Inactive;
        case Cause::InternalError: return Severity::Persistent;
        default: return Severity::Transient;
    }
}

/**
 * @brief Well-known precondition name constants.
 */
namespace PreconditionNames {
    inline constexpr const char* WIFI_STATION = "WiFi Station";
    inline constexpr const char* TIMEZONE     = "Timezone";
    inline constexpr const char* TIME_SYNC    = "Time Sync";
}
