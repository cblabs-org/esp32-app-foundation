/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/TimeManager/timeManager.cpp
 * Description: Implementation of the time manager, handling NTP sync and timezone configuration.
 *
 * Exported Functions/Classes:
 * - TimeManager::begin(): Initializes NTP config and timezone.
 * - TimeManager::tick(): Non-blocking update routine to be called in main loop.
 * - TimeManager::setTimezone(): Sets POSIX timezone string.
 * - TimeManager::getLocalTime(): Gets current local time safely.
 * - TimeManager::isTimeSet(): Returns true if time has been synchronized.
 */

#include "TimeManager.hpp"
#include "platform/LogShim.h"

static const char* TAG = "TimeManager";

#ifndef NATIVE_BUILD
#include <esp_sntp.h>

/**
 * @brief Initializes NTP config and sets the POSIX timezone.
 * @param timezone POSIX timezone string.
 * @param ntpServer1 Primary NTP server.
 * @param ntpServer2 Secondary NTP server.
 * @param ntpServer3 Tertiary NTP server.
 */
void TimeManager::begin(const char* timezone, const char* ntpServer1, const char* ntpServer2, const char* ntpServer3) {
    ESP_LOGI(TAG, "Configuring NTP servers: %s, %s, %s",
             ntpServer1 ? ntpServer1 : "(null)",
             ntpServer2 ? ntpServer2 : "(null)",
             ntpServer3 ? ntpServer3 : "(null)");

    // --- Step 1: Initialize timezone ---
    // Ensure the timezone is set before starting time synchronization
    setTimezone(timezone);

    // --- Step 2: Configure NTP servers ---
    // Use the core ESP32 standard configTzTime which combines setting timezone and SNTP
    configTzTime(timezone, ntpServer1, ntpServer2, ntpServer3);
    
    _syncStatus = TimeSyncStatus::InProgress;
}

/**
 * @brief Non-blocking update routine to be called in main loop.
 * Handled NTP state machine and time sync events.
 */
void TimeManager::tick() {
    // --- Step 1: Ignore if already synced ---
    // Avoid unnecessary system calls if synchronization is already complete
    if (_syncStatus == TimeSyncStatus::Synced) {
        return;
    }

    // --- Step 2: Poll SNTP Status ---
    // We check the low level ESP-IDF API to see if SNTP completed a sync cycle
    sntp_sync_status_t syncStatus = sntp_get_sync_status();
    
    if (syncStatus == SNTP_SYNC_STATUS_COMPLETED) {
        // Time has been successfully retrieved from NTP servers
        _syncStatus = TimeSyncStatus::Synced;
        ESP_LOGI(TAG, "NTP time synchronization complete");
    }
}

/**
 * @brief Sets POSIX timezone string.
 * @param timezone POSIX timezone string.
 */
void TimeManager::setTimezone(const char* timezone) {
    if (timezone != nullptr) {
        ESP_LOGI(TAG, "Setting timezone: %s", timezone);
        setenv("TZ", timezone, 1); // 1 = overwrite existing TZ variable
        tzset();
    }
}

/**
 * @brief Gets current local time safely.
 * @param timeinfo Pointer to a tm struct to populate.
 * @return true if time is valid and populated, false otherwise.
 */
bool TimeManager::getLocalTime(struct tm* timeinfo) {
    if (timeinfo == nullptr) {
        return false;
    }
    
    if (_syncStatus != TimeSyncStatus::Synced) {
        ESP_LOGD(TAG, "getLocalTime() called but time is not yet synced");
        return false;
    }
    
    time_t now = 0;
    time(&now);
    localtime_r(&now, timeinfo);
    
    return true;
}

/**
 * @brief Returns true if time has been successfully synchronized.
 * @return true if time is set.
 */
bool TimeManager::isTimeSet() const {
    return _syncStatus == TimeSyncStatus::Synced;
}

#else
// Mock implementation for NATIVE_BUILD testing

void TimeManager::begin(const char* timezone, const char* ntpServer1, const char* ntpServer2, const char* ntpServer3) {
    setTimezone(timezone);
    _syncStatus = TimeSyncStatus::Synced;
}

void TimeManager::tick() {
    // No-op for native build
}

void TimeManager::setTimezone(const char* timezone) {
    // Mock setting timezone
}

bool TimeManager::getLocalTime(struct tm* timeinfo) {
    if (timeinfo == nullptr) return false;
    time_t now = time(nullptr);
    // Use standard localtime for mock
    struct tm* ti = localtime(&now);
    if (ti) {
        *timeinfo = *ti;
        return true;
    }
    return false;
}

bool TimeManager::isTimeSet() const {
    return _syncStatus == TimeSyncStatus::Synced;
}

#endif
