#pragma once
#include <string>

/**
 * @brief Pure abstract interface for weather data access.
 *
 * Decouples display boards from the concrete WeatherModule implementation.
 * All methods must be thread-safe (called from the main Arduino task).
 */
class IWeatherDataProvider {
public:
    virtual ~IWeatherDataProvider() = default;

    // Data availability
    virtual bool hasData() const = 0;

    // Numeric getters (atomic-backed in implementations)
    virtual float getTemperature() const = 0;
    virtual float getWindSpeed() const = 0;
    virtual int getHumidity() const = 0;
    virtual int getPressure() const = 0;
    virtual int getVisibility() const = 0;
    virtual int getClouds() const = 0;
    virtual float getFeelsLike() const = 0;
    virtual float getTempMin() const = 0;
    virtual float getTempMax() const = 0;
    virtual int getWindDeg() const = 0;
    virtual uint32_t getSunrise() const = 0;
    virtual uint32_t getSunset() const = 0;

    // String getters (mutex-backed in implementations)
    virtual std::string getCondition() const = 0;
    virtual std::string getIcon() const = 0;
    virtual std::string getCityName() const = 0;
    virtual std::string getCountry() const = 0;
    virtual std::string getUnits() const = 0;
};
