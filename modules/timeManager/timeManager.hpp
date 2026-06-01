/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/timeManager/timeManager.hpp
 * Description: Manages NTP synchronization, POSIX timezone rules, and non-blocking time tick updates.
 *
 * Exported Functions/Classes:
 * - TimeSyncStatus: Enum representing the current NTP synchronization status.
 * - TimeManager: Class managing time sync and rules.
 *   - begin(): Initializes NTP config and timezone.
 *   - tick(): Non-blocking update routine to be called in main loop.
 *   - setTimezone(): Sets POSIX timezone string.
 *   - getLocalTime(): Gets current local time safely.
 *   - isTimeSet(): Returns true if time has been synchronized.
 */

#pragma once

#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <time.h>
#include <esp_sntp.h>
#else
#include <string>
#include <stdint.h>
#include <time.h>
#endif

#include "../../include/interfaces/ITickable.hpp"

/**
 * @brief Enum representing the current NTP synchronization status
 */
enum class TimeSyncStatus {
    UNSYNCED,
    IN_PROGRESS,
    SYNCED
};

/**
 * @brief Kernel service that manages NTP synchronization and POSIX timezone rules.
 *
 * Implements ITickable so it can be registered in the Application tick dispatch
 * vector alongside other periodic services.
 */
class TimeManager : public ITickable {
public:
    TimeManager() = default;
    ~TimeManager() override = default;

    /**
     * @brief Initializes NTP config and sets the POSIX timezone.
     * @param timezone POSIX timezone string.
     * @param ntpServer1 Primary NTP server.
     * @param ntpServer2 Secondary NTP server.
     * @param ntpServer3 Tertiary NTP server.
     */
    void begin(const char* timezone, const char* ntpServer1 = "pool.ntp.org", const char* ntpServer2 = "time.nist.gov", const char* ntpServer3 = "time.windows.com");

    /**
     * @brief Non-blocking update routine to be called in main loop.
     * Handled NTP state machine and time sync events.
     */
    void tick() override;

    /**
     * @brief Sets POSIX timezone string.
     * @param timezone POSIX timezone string.
     */
    void setTimezone(const char* timezone);

    /**
     * @brief Gets current local time safely.
     * @param timeinfo Pointer to a tm struct to populate.
     * @return true if time is valid and populated, false otherwise.
     */
    bool getLocalTime(struct tm* timeinfo);

    /**
     * @brief Returns true if time has been successfully synchronized.
     * @return true if time is set.
     */
    bool isTimeSet() const;

private:
    TimeSyncStatus _syncStatus = TimeSyncStatus::UNSYNCED; // Current NTP sync status
    unsigned long _lastSyncTime = 0; // Tracking variable for sync intervals
};
