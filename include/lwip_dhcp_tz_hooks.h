/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/lwip_dhcp_tz_hooks.h
 * Description: Pure-C LwIP DHCP hook definitions and staging handoff declaration
 *              for RFC 4833 timezone discovery (Option 100 PCode / Option 101 TCode).
 *              Injected into ESP-IDF's lwip component via ESP_IDF_LWIP_HOOK_FILENAME.
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @brief RFC 4833 DHCP Option 100: POSIX timezone rule string (PCode). */
#define LWIP_DHCP_OPTION_TZ_PCODE 100

/** @brief RFC 4833 DHCP Option 101: IANA timezone database name (TCode). */
#define LWIP_DHCP_OPTION_TZ_TCODE 101

/** @brief Maximum supported RFC 4833 timezone payload length (excluding NUL). */
#define LWIP_DHCP_TZ_MAX_LEN 64

/*
 * Inject Options 100 & 101 into LwIP's DHCP Parameter Request List (Option 55).
 * Leading comma is mandatory because dhcp.c places DHCP_ADD_EXTRA_REQUEST_OPTIONS
 * directly after #endif inside dhcp_discover_request_options[] with no separator.
 */
#ifndef DHCP_ADD_EXTRA_REQUEST_OPTIONS
#define DHCP_ADD_EXTRA_REQUEST_OPTIONS , LWIP_DHCP_OPTION_TZ_PCODE, LWIP_DHCP_OPTION_TZ_TCODE
#endif

/*
 * Forward-declare struct dhcp at file scope before lwip_default_hooks.h
 * declares lwip_dhcp_on_extra_option(struct dhcp *...) at line 74 (upstream
 * ESP-IDF v5.5.5 places `struct dhcp;` at line 81, which otherwise causes a
 * C17 parameter-scope struct mismatch against lwip_default_hooks.c when
 * CONFIG_LWIP_HOOK_DHCP_EXTRA_OPTION_DEFAULT=y).
 */
struct dhcp;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Drain pending RFC 4833 DHCP timezone payloads staged by the LwIP hook.
 * Implemented in TimeManager.cpp behind a spinlock.
 *
 * @param outPcode Destination buffer for Option 100 (PCode) string (may be NULL).
 * @param pcodeCap Capacity of @p outPcode in bytes (including NUL terminator).
 * @param gotPcode Output flag set true if a PCode payload was staged (may be NULL).
 * @param outTcode Destination buffer for Option 101 (TCode) string (may be NULL).
 * @param tcodeCap Capacity of @p outTcode in bytes (including NUL terminator).
 * @param gotTcode Output flag set true if a TCode payload was staged (may be NULL).
 * @return true if at least one pending RFC 4833 timezone payload was drained.
 */
bool lwip_dhcp_tz_take_pending(char* outPcode, size_t pcodeCap, bool* gotPcode,
                               char* outTcode, size_t tcodeCap, bool* gotTcode);

#ifdef __cplusplus
}
#endif
