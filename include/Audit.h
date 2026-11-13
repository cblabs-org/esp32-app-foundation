/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/Audit.h
 * Description: Public audit logging convenience macros and soft-dependency facade.
 *              Provides a two-tier API for audit trail recording:
 *
 *              1. AUDIT(...) / AUDIT_SYNC(...) macros:
 *                 Safe for shared library modules and domain services. When
 *                 WITH_AUDIT is defined, these macros forward directly to
 *                 AuditModule::instance().log() and auditSync(). When WITH_AUDIT
 *                 is NOT defined, they compile to a zero-cost ((void)0) no-op.
 *
 *              2. Direct AuditModule::instance() API:
 *                 For modules or endpoints that strictly require audit functionality.
 *                 Direct inclusion of AuditModule.hpp without WITH_AUDIT produces
 *                 a compile-time error directing callers to include/Audit.h.
 *
 * Exported Macros:
 * - AUDIT(source, action, ...): Asynchronously enqueues an audit record (non-blocking).
 * - AUDIT_SYNC(source, action, ...): Enqueues an audit record and waits for flash write.
 */

#pragma once

#ifndef AUDIT_H
#define AUDIT_H

#ifdef WITH_AUDIT

#include "AuditModule.hpp"

/**
 * @brief Asynchronously enqueue an audit record (non-blocking).
 *
 * Enqueues an audit record to the AuditModule background writer task.
 * If the queue is full or rate limited, the record may be dropped or throttled.
 *
 * @param source Originating subsystem or module (e.g., "auth", "config", "wifi").
 * @param action Specific action performed (e.g., "login", "update", "connect").
 * @param ...    Optional target ID and details strings.
 */
#define AUDIT(source, action, ...)       AuditModule::instance().log(source, action, ##__VA_ARGS__)

/**
 * @brief Synchronously enqueue an audit record and block until flushed to flash storage.
 *
 * @param source Originating subsystem or module (e.g., "auth", "config", "wifi").
 * @param action Specific action performed (e.g., "login", "update", "connect").
 * @param ...    Optional target ID and details strings.
 */
#define AUDIT_SYNC(source, action, ...)  AuditModule::instance().auditSync(source, action, ##__VA_ARGS__)

#else

/**
 * @brief Soft no-op audit macro when WITH_AUDIT is not enabled.
 */
#define AUDIT(source, action, ...)       ((void)0)

/**
 * @brief Soft no-op synchronous audit macro when WITH_AUDIT is not enabled.
 */
#define AUDIT_SYNC(source, action, ...)  ((void)0)

#endif // WITH_AUDIT

#endif // AUDIT_H
