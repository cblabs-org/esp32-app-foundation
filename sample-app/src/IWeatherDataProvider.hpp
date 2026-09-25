/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: sample-app/src/IWeatherDataProvider.hpp
 * Description: Pure abstract interface for weather data access.
 *              Decouples display boards and consumer components from the
 *              concrete WeatherModule implementation.
 *
 * Exported Classes:
 * - IWeatherDataProvider: Pure abstract interface for thread-safe weather data access
 *   - hasData():           Returns true once valid weather data has been parsed
 *   - getTemperature():    Returns current temperature
 *   - getWindSpeed():      Returns wind speed
 *   - getHumidity():       Returns relative humidity percentage
 *   - getPressure():       Returns barometric pressure in hPa
 *   - getVisibility():     Returns visibility distance in metres
 *   - getClouds():         Returns cloud coverage percentage
 *   - getFeelsLike():      Returns apparent ("feels like") temperature
 *   - getTempMin():        Returns daily minimum temperature
 *   - getTempMax():        Returns daily maximum temperature
 *   - getWindDeg():        Returns wind direction in degrees
 *   - getSunrise():        Returns sunrise time as UTC epoch
 *   - getSunset():         Returns sunset time as UTC epoch
 *   - getCondition():      Returns weather condition description
 *   - getIcon():           Returns OpenWeatherMap icon code
 *   - getCityName():       Returns location city name
 *   - getCountry():        Returns location country code
 *   - getUnits():          Returns configured temperature units ("metric"/"imperial")
 */

#pragma once

#include <string>
#include <cstdint>

/**
 * @brief Pure abstract interface for weather data access.
 *
 * Decouples display boards and other UI or integration components from the
 * concrete WeatherModule implementation. All methods must be thread-safe
 * when called from the main Arduino task or render loop.
 */
class IWeatherDataProvider {
public:
    virtual ~IWeatherDataProvider() = default;

    // --- Data availability ---

    /** @brief Returns true once the first successful weather data parse has completed. */
    virtual bool hasData() const = 0;

    // --- Numeric getters (atomic-backed in implementations) ---

    /** @brief Returns the current temperature. */
    virtual float getTemperature() const = 0;

    /** @brief Returns the current wind speed. */
    virtual float getWindSpeed() const = 0;

    /** @brief Returns the relative humidity percentage. */
    virtual int getHumidity() const = 0;

    /** @brief Returns the barometric pressure in hPa. */
    virtual int getPressure() const = 0;

    /** @brief Returns the visibility distance in metres. */
    virtual int getVisibility() const = 0;

    /** @brief Returns the cloud coverage percentage. */
    virtual int getClouds() const = 0;

    /** @brief Returns the apparent ("feels like") temperature. */
    virtual float getFeelsLike() const = 0;

    /** @brief Returns the daily minimum temperature. */
    virtual float getTempMin() const = 0;

    /** @brief Returns the daily maximum temperature. */
    virtual float getTempMax() const = 0;

    /** @brief Returns the wind direction in degrees. */
    virtual int getWindDeg() const = 0;

    /** @brief Returns the sunrise time as a UTC epoch timestamp. */
    virtual uint32_t getSunrise() const = 0;

    /** @brief Returns the sunset time as a UTC epoch timestamp. */
    virtual uint32_t getSunset() const = 0;

    // --- String getters (mutex-backed in implementations) ---

    /** @brief Returns the weather condition description (e.g., "Clouds", "Clear"). */
    virtual std::string getCondition() const = 0;

    /** @brief Returns the OpenWeatherMap icon code (e.g., "10d"). */
    virtual std::string getIcon() const = 0;

    /** @brief Returns the location city name. */
    virtual std::string getCityName() const = 0;

    /** @brief Returns the location country code (e.g., "GB", "US"). */
    virtual std::string getCountry() const = 0;

    /** @brief Returns the configured temperature units ("metric" or "imperial"). */
    virtual std::string getUnits() const = 0;
};
