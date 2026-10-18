# ADR: Home Assistant Device Metadata Flow

## Context

The HA agent review identified that the device registered in Home Assistant as "ESP32 Device (e08cfe32ac80)" with no `sw_version` and a generic "App Foundation" model. This cascaded into ugly entity friendly names like "ESP32 Device (e08cfe32ac80) Temperature".

## Decision

Added `setAppName()` and `setAppVersion()` to `HomeAssistantModule`. The discovery payload's `device` block now reads:

```json
{
  "identifiers": ["e08cfe32ac80"],
  "name": "Weather Station",
  "manufacturer": "CB Labs",
  "model": "Weather Station",
  "sw_version": "0.0.12-dev.25"
}
```

### Wiring in main.cpp

```cpp
haModule.setAppName(app.getAppName());      // "Weather Station"
haModule.setAppVersion(BUILD_VERSION);       // "0.0.12-dev.25"
```

### Fallback behaviour

If `_appName` is empty, falls back to "ESP32 Device (MAC)" / "App Foundation" — so framework-only users without a sample app still get a working device.

## Additional fixes

- `HaSensor`: added `setIcon()` and `ic` field in `buildDiscoveryPayload()`
- `WeatherModule`: renamed "Weather Condition" → "Condition" (avoids redundant "Weather Station Weather Condition")
- `WeatherModule`: added `mdi:weather-partly-cloudy` icon to condition sensor

## Consequences

- User must **delete the old device** in HA → Settings → Devices for re-discovery to pick up the new metadata
- HA auto-generates clean friendly names: "Weather Station Temperature", "Weather Station Condition"
- `sw_version` updates on every firmware flash (tracks `BUILD_VERSION` from `inject_version.py`)
