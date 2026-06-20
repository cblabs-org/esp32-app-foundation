# Work Order 2: WeatherModule Logic

**Objective:** Build the `WeatherModule` for the sample app that fetches OpenWeatherMap data and pushes it to Home Assistant.

**Strict Constraints:**
- **Non-blocking Fetch:** Use the `IDataSource` pattern to ensure the JSON fetch does not block the 10ms tick budget.
- **Clean-room Provenance:** Write the parsing logic completely from scratch using `JsonStreamingParser` (do not copy Gadec UK code).
- **Dependency Injection:** Receive `HomeAssistantModule*` via `setHomeAssistant()`. Do not assume a framework getter exists.

**Tasks:**
1. Create `sample-app/src/WeatherModule.hpp` and `.cpp`.
2. Implement `ITickable`, `IConfigurable`, `IWebContributor`.
3. Contribute a **"Weather" top-level tab** (replacing the generic "App" tab) with sub-pills:
   - **Settings**: Latitude, Longitude, Polling Interval.
   - **API Key**: Instructions for obtaining an OpenWeatherMap key + secure input via `KeyManager`.
4. Instantiate `HaSensor` objects for Temperature, Wind Speed, and Weather Condition.
5. Register the sensors with the injected `HomeAssistantModule*`.
6. Fetch JSON data from OpenWeatherMap using the API key securely retrieved from `KeyManager`.
7. Publish states to the `HaSensor` objects to sync with Home Assistant.
8. Broadcast a `weather.updated` event to the `EventBus` for local displays.
